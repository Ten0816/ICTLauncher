#include "..\include\Config.hpp"

#if SIV3D_PLATFORM(WINDOWS)
#include <Shlwapi.h>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <cwchar>
#pragma comment(lib, "Shlwapi.lib")



// ============================================================
// 起動設定
// ============================================================

namespace Config
{
	// --------------------------------------------------------
	// 環境変数の取得
	// --------------------------------------------------------

	std::wstring GetEnvironmentVariableString(
		const wchar_t* name)
	{
		const DWORD length =
			::GetEnvironmentVariableW(name, nullptr, 0);

		if (length == 0)
		{
			return L"";
		}

		std::wstring value(length, L'\0');

		const DWORD written =
			::GetEnvironmentVariableW(
				name,
				value.data(),
				length
			);

		if (written == 0)
		{
			return L"";
		}

		value.resize(written);

		return value;
	}


	// --------------------------------------------------------
	// ブラウザの自動探索
	// --------------------------------------------------------

	BrowserInfo FindBrowser()
	{
		const std::wstring programFiles =
			GetEnvironmentVariableString(L"ProgramFiles");

		const std::wstring programFilesX86 =
			GetEnvironmentVariableString(L"ProgramFiles(x86)");

		const std::wstring localAppData =
			GetEnvironmentVariableString(L"LOCALAPPDATA");

		struct Candidate
		{
			std::wstring path;
			BrowserType type;
		};

		const std::vector<Candidate> candidates =
		{
			{
				programFiles
				+ L"\\Microsoft\\Edge\\Application\\msedge.exe",
				BrowserType::Edge
			},
			{
				programFilesX86
				+ L"\\Microsoft\\Edge\\Application\\msedge.exe",
				BrowserType::Edge
			},
			{
				programFiles
				+ L"\\Google\\Chrome\\Application\\chrome.exe",
				BrowserType::Chrome
			},
			{
				programFilesX86
				+ L"\\Google\\Chrome\\Application\\chrome.exe",
				BrowserType::Chrome
			},
			{
				localAppData
				+ L"\\Google\\Chrome\\Application\\chrome.exe",
				BrowserType::Chrome
			}
		};

		for (const auto& candidate : candidates)
		{
			if (std::filesystem::is_regular_file(candidate.path))
			{
				return {
					Unicode::FromWstring(candidate.path),
					candidate.type
				};
			}
		}

		return {};
	}


	// --------------------------------------------------------
	// URL判定
	// --------------------------------------------------------

	bool IsURL(StringView path)
	{
		return (
			path.starts_with(U"http://")
			|| path.starts_with(U"https://")
		);
	}


	// --------------------------------------------------------
	// HTML判定
	// --------------------------------------------------------

	bool IsHTML(const FilePath& path)
	{
		const String extension =
			FileSystem::Extension(path).lowercased();

		return (
			extension == U"html"
			|| extension == U"htm"
		);
	}


	// --------------------------------------------------------
	// HTMLファイルのURL変換
	// --------------------------------------------------------

	String ToBrowserURL(const FilePath& path)
	{
		if (IsURL(path))
		{
			return path;
		}

		// WindowsのファイルパスをURLに変換
		std::vector<wchar_t> buffer(32768);

		DWORD length =
			static_cast<DWORD>(buffer.size());

		const HRESULT result = ::UrlCreateFromPathW(
			path.toWstr().c_str(),
			buffer.data(),
			&length,
			0
		);

		if (FAILED(result))
		{
			return U"";
		}

		return Unicode::FromWstring(buffer.data());
	}


	// --------------------------------------------------------
	// ブラウザ専用プロファイル
	// --------------------------------------------------------

	FilePath GetBrowserProfileDirectory()
	{
		const std::wstring localAppData =
			GetEnvironmentVariableString(L"LOCALAPPDATA");

		if (localAppData.empty())
		{
			return U"";
		}

		const std::filesystem::path profilePath =
			std::filesystem::path(localAppData)
			/ L"ICTLauncher"
			/ L"BrowserProfile";

		std::error_code ec;

		std::filesystem::create_directories(
			profilePath,
			ec
		);

		if (ec)
		{
			return U"";
		}

		return Unicode::FromWstring(
			profilePath.wstring()
		);
	}


	// --------------------------------------------------------
	// ブラウザウィンドウ監視
	// --------------------------------------------------------

	void BrowserProcess::Close()
	{
		if (processHandle)
		{
			::CloseHandle(processHandle);
			processHandle = nullptr;
		}
		processId = 0;
		windowHandle = nullptr;
		windowProcessId = 0;
		existingWindows.clear();
		waitingForWindow = false;
		launchTick = 0;
		executablePath.clear();
	}


	BOOL CALLBACK CollectWindowsProc(HWND hwnd, LPARAM param)
	{
		auto* windows = reinterpret_cast<std::vector<HWND>*>(param);
		if (::IsWindowVisible(hwnd)
			&& ::GetWindow(hwnd, GW_OWNER) == nullptr)
		{
			windows->push_back(hwnd);
		}
		return TRUE;
	}

	std::vector<HWND> GetTopLevelWindows()
	{
		std::vector<HWND> windows;
		::EnumWindows(CollectWindowsProc,
			reinterpret_cast<LPARAM>(&windows));
		return windows;
	}

	bool IsBrowserWindow(HWND hwnd, const FilePath& expectedExecutable)
	{
		wchar_t className[256]{};
		if (!::GetClassNameW(hwnd, className, 256)
			|| std::wstring(className) != L"Chrome_WidgetWin_1")
		{
			return false;
		}

		DWORD pid = 0;
		::GetWindowThreadProcessId(hwnd, &pid);
		if (!pid)
		{
			return false;
		}

		HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
			FALSE, pid);
		if (!handle)
		{
			return false;
		}

		std::vector<wchar_t> exePath(32768);

		DWORD length = static_cast<DWORD>(exePath.size());

		const BOOL ok = ::QueryFullProcessImageNameW(
			handle, 0, exePath.data(), &length
		);
		::CloseHandle(handle);
		if (!ok)
		{
			return false;
		}

		return (_wcsicmp(
			exePath.data(),
			expectedExecutable.toWstr().c_str()
		) == 0);
	}

	HWND FindNewBrowserWindow(const BrowserProcess& browser)
	{
		for (HWND hwnd : GetTopLevelWindows())
		{
			if (std::find(browser.existingWindows.begin(),
				browser.existingWindows.end(), hwnd)
				!= browser.existingWindows.end())
			{
				continue;
			}
			if (IsBrowserWindow(hwnd, browser.executablePath))
			{
				return hwnd;
			}
		}
		return nullptr;
	}

	bool IsTrackedWindowOpen(const BrowserProcess& browser)
	{
		if (!browser.windowHandle || !::IsWindow(browser.windowHandle))
		{
			return false;
		}
		DWORD pid = 0;
		::GetWindowThreadProcessId(browser.windowHandle, &pid);
		return pid == browser.windowProcessId;
	}

	// --------------------------------------------------------
	// ブラウザの起動
	// --------------------------------------------------------

	bool LaunchBrowser(
		const BrowserInfo& browser,
		const FilePath& target,
		BrowserProcess& result)
	{
		if (browser.type == BrowserType::None)
		{
			return false;
		}

		const FilePath profileDirectory =
			GetBrowserProfileDirectory();

		if (profileDirectory.isEmpty())
		{
			return false;
		}

		// ローカルHTMLファイルの存在確認
		if (not IsURL(target) && not FileSystem::Exists(target))
		{
			System::MessageBoxOK(
				U"HTMLファイルが見つかりません。\n"
				U"パス: " + target
			);

			return false;
		}

		// URL変換
		const String url = ToBrowserURL(target);

		if (url.isEmpty())
		{
			System::MessageBoxOK(
				U"HTMLファイルのURL変換に失敗しました。\n"
				U"パス: " + target
			);

			return false;
		}

		// ブラウザの起動コマンド
		const std::wstring command =
			L"\"" + browser.path.toWstr() + L"\""
			+ L" --user-data-dir=\""
			+ profileDirectory.toWstr()
			+ L"\""
			+ L" --no-first-run"
			+ L" --no-default-browser-check"
			+ L" --start-fullscreen"
			+ L" --app=\"" + url.toWstr() + L"\"";

		std::vector<wchar_t> commandBuffer(
			command.begin(),
			command.end()
		);

		commandBuffer.push_back(L'\0');

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);

		PROCESS_INFORMATION processInfo{};

		// 起動前のウィンドウを記録
		result.existingWindows = GetTopLevelWindows();

		result.windowHandle = nullptr;
		result.waitingForWindow = true;
		result.launchTick = ::GetTickCount64();
		result.executablePath = browser.path;

		const std::filesystem::path browserDirectory =
			std::filesystem::path{
				browser.path.toWstr()
		}.parent_path();


		const BOOL success =
			::CreateProcessW(
				browser.path.toWstr().c_str(),
				commandBuffer.data(),
				nullptr,
				nullptr,
				FALSE,
				0,
				nullptr,
				browserDirectory.empty()
					? nullptr
					: browserDirectory.c_str(),
				&startupInfo,
				&processInfo
			);


		if (not success)
		{
			result.Close();
			return false;
		}

		::CloseHandle(processInfo.hThread);

		result.processHandle =
			processInfo.hProcess;

		result.processId =
			processInfo.dwProcessId;

		return true;
	}


}

#endif
