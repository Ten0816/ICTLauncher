
#include "..\include\GameRunner.hpp"
#include "..\include\LauncherLog.hpp"

#include <exception>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cwchar>

namespace
{
	// ========================================================
	// 定数
	// ========================================================

	constexpr uint64 BrowserWindowTimeoutMs = 15000;
	constexpr uint64 ExeWindowTimeoutMs = 10000;

	// ========================================================
	// Windowsの代表的な例外終了コード
	// ========================================================

	bool IsLikelyCrashCode(const uint32 code)
	{
		switch (code)
		{
		case 0xC0000005: // Access violation
		case 0xC000001D: // Illegal instruction
		case 0xC0000094: // Integer divide by zero
		case 0xC00000FD: // Stack overflow
		case 0xC0000409: // Stack buffer overrun / fail fast
			return true;

		default:
			return false;
		}
	}

	// ========================================================
	// EXE終了コードのログ記録
	// ========================================================

	template <class T>
	void WriteExitCode(
		const String& gameId,
		const T& result)
	{
		if constexpr (requires { result.has_value(); })
		{
			if (result.has_value())
			{
				WriteExitCode(gameId, *result);
			}
			else
			{
				LauncherLog::Write(
					"GAME_EXIT",
					U"gameId=" + gameId
					+ U" type=exe"
					+ U" code=unknown"
					+ U" result=unknown"
				);
			}
		}
		else
		{
			const uint32 rawCode =
				static_cast<uint32>(result);

			const String codeText =
				Format(static_cast<int64>(result));

			String status = U"nonzero";

			if (rawCode == 0)
			{
				status = U"normal";
			}
			else if (IsLikelyCrashCode(rawCode))
			{
				status = U"crash_suspected";
			}

			LauncherLog::Write(
				"GAME_EXIT",
				U"gameId=" + gameId
				+ U" type=exe"
				+ U" code=" + codeText
				+ U" result=" + status
			);
		}
	}

#if SIV3D_PLATFORM(WINDOWS)

	// ========================================================
	// Launcherを前面に復帰させる
	// ========================================================

	void RestoreLauncherToForeground(const String& gameId)
	{
		Window::Restore();

		const HWND hwnd = static_cast<HWND>(
			Platform::Windows::Window::GetHWND()
		);

		if (not hwnd || not ::IsWindow(hwnd))
		{
			LauncherLog::Write(
				"LAUNCHER_RESTORE_FAILED",
				U"gameId=" + gameId
				+ U" reason=invalid_window_handle"
			);

			return;
		}

		const BOOL moved = ::SetWindowPos(
			hwnd,
			HWND_TOP,
			0, 0, 0, 0,
			SWP_NOMOVE
			| SWP_NOSIZE
			| SWP_SHOWWINDOW
		);

		const BOOL activated =
			::SetForegroundWindow(hwnd);

		const bool isForeground =
			(::GetForegroundWindow() == hwnd);

		if (isForeground)
		{
			LauncherLog::Write(
				"LAUNCHER_RESTORED",
				U"gameId=" + gameId
				+ U" result=foreground"
			);
		}
		else
		{
			LauncherLog::Write(
				"LAUNCHER_RESTORE_WARNING",
				U"gameId=" + gameId
				+ U" moved=" + Format(moved != FALSE)
				+ U" activated="
				+ Format(activated != FALSE)
				+ U" result=not_foreground"
			);
		}
	}

	// ========================================================
	// 既存のトップレベルウィンドウ一覧
	// ========================================================

	BOOL CALLBACK CollectExeWindowsProc(
		HWND hwnd,
		LPARAM param)
	{
		auto* windows =
			reinterpret_cast<std::vector<HWND>*>(param);

		if (
			::IsWindowVisible(hwnd)
			&& ::GetWindow(hwnd, GW_OWNER) == nullptr
		)
		{
			windows->push_back(hwnd);
		}

		return TRUE;
	}

	std::vector<HWND> GetExistingExeWindows()
	{
		std::vector<HWND> windows;

		::EnumWindows(
			CollectExeWindowsProc,
			reinterpret_cast<LPARAM>(&windows)
		);

		return windows;
	}

	// ========================================================
	// Windows形式の絶対パスを取得
	// ========================================================

	std::wstring GetAbsoluteWindowsPath(
		const std::wstring& path)
	{
		if (path.empty())
		{
			return L"";
		}

		const DWORD required =
			::GetFullPathNameW(
				path.c_str(),
				0,
				nullptr,
				nullptr
			);

		if (required == 0)
		{
			return L"";
		}

		std::vector<wchar_t> buffer(required);

		const DWORD written =
			::GetFullPathNameW(
				path.c_str(),
				static_cast<DWORD>(buffer.size()),
				buffer.data(),
				nullptr
			);

		if (
			written == 0
			|| written >= buffer.size()
		)
		{
			return L"";
		}

		return std::wstring(
			buffer.data(),
			written
		);
	}

	// ========================================================
	// ウィンドウを所有するEXEのパスを取得
	// ========================================================

	std::wstring GetWindowProcessPath(HWND hwnd)
	{
		DWORD pid = 0;

		::GetWindowThreadProcessId(
			hwnd,
			&pid
		);

		if (pid == 0)
		{
			return L"";
		}

		HANDLE process = ::OpenProcess(
			PROCESS_QUERY_LIMITED_INFORMATION,
			FALSE,
			pid
		);

		if (not process)
		{
			return L"";
		}

		// スタックではなくヒープ上に確保する
		std::vector<wchar_t> buffer(32768);

		DWORD length =
			static_cast<DWORD>(buffer.size());

		const BOOL success =
			::QueryFullProcessImageNameW(
				process,
				0,
				buffer.data(),
				&length
			);

		::CloseHandle(process);

		if (not success)
		{
			return L"";
		}

		return std::wstring(
			buffer.data(),
			length
		);
	}

	// ========================================================
	// 起動対象のEXEウィンドウか確認
	// ========================================================

	bool IsMatchingExeWindow(
		HWND hwnd,
		const FilePath& expectedPath)
	{
		if (
			not ::IsWindow(hwnd)
			|| not ::IsWindowVisible(hwnd)
			|| ::GetWindow(hwnd, GW_OWNER) != nullptr
		)
		{
			return false;
		}

		const std::wstring actualPath =
			GetWindowProcessPath(hwnd);

		if (actualPath.empty())
		{
			return false;
		}

		const std::wstring expectedAbsolutePath =
			GetAbsoluteWindowsPath(
				expectedPath.toWstr()
			);

		if (expectedAbsolutePath.empty())
		{
			return false;
		}

		// Windowsパスの大文字・小文字は区別しない
		return (
			_wcsicmp(
				actualPath.c_str(),
				expectedAbsolutePath.c_str()
			) == 0
		);
	}

	// ========================================================
	// 新しく出現したEXEゲームウィンドウを探す
	// ========================================================

	HWND FindExeGameWindow(
		const FilePath& exePath,
		const std::vector<HWND>& existingWindows)
	{
		const auto windows =
			GetExistingExeWindows();

		for (HWND hwnd : windows)
		{
			// 起動前から存在したウィンドウを除外
			if (
				std::find(
					existingWindows.begin(),
					existingWindows.end(),
					hwnd
				) != existingWindows.end()
			)
			{
				continue;
			}

			if (IsMatchingExeWindow(hwnd, exePath))
			{
				return hwnd;
			}
		}

		return nullptr;
	}

	// ========================================================
	// ゲームウィンドウの前面化を要求
	// ========================================================

	bool ActivateGameWindow(
		HWND hwnd,
		const String& gameId,
		StringView type)
	{
		if (not hwnd || not ::IsWindow(hwnd))
		{
			return false;
		}

		if (::IsIconic(hwnd))
		{
			::ShowWindow(
				hwnd,
				SW_RESTORE
			);
		}
		else
		{
			::ShowWindow(
				hwnd,
				SW_SHOW
			);
		}

		const BOOL moved =
			::SetWindowPos(
				hwnd,
				HWND_TOP,
				0, 0, 0, 0,
				SWP_NOMOVE
				| SWP_NOSIZE
				| SWP_SHOWWINDOW
			);

		const BOOL activated =
			::SetForegroundWindow(hwnd);

		const bool isForeground =
			(::GetForegroundWindow() == hwnd);

		LauncherLog::Write(
			"GAME_WINDOW_ACTIVATION",
			U"gameId=" + gameId
			+ U" type=" + String(type)
			+ U" moved=" + Format(moved != FALSE)
			+ U" requested=" + Format(activated != FALSE)
			+ U" foreground=" + Format(isForeground)
		);

		return isForeground;
	}

#else

	// ========================================================
	// 非Windows環境
	// ========================================================

	void RestoreLauncherToForeground(const String&)
	{
		Window::Restore();
	}

#endif
}

// ============================================================
// コンストラクタ
// ============================================================

GameRunner::GameRunner()
{
#if SIV3D_PLATFORM(WINDOWS)
	browser_ = Config::FindBrowser();
#endif
}

// ============================================================
// デストラクタ
// ============================================================

GameRunner::~GameRunner()
{
#if SIV3D_PLATFORM(WINDOWS)
	browserProcess_.Close();
#endif
}

// ============================================================
// ゲーム起動
// ============================================================

bool GameRunner::Start(const Game& game)
{
	if (active_)
	{
		LauncherLog::Write(
			"GAME_START_REJECTED",
			U"already active: " + game.gameId
		);

		return false;
	}

	recoveryRequested_ = false;
	currentGameId_ = game.gameId;

	const String context =
		U"gameId=" + game.gameId
		+ U" title=" + game.title
		+ U" path=" + game.path;

	// ========================================================
	// ブラウザゲーム
	// ========================================================

	if (game.isWebApp)
	{
#if SIV3D_PLATFORM(WINDOWS)

		if (browser_.type == Config::BrowserType::None)
		{
			LauncherLog::Write(
				"GAME_START_FAILED",
				context + U" reason=browser_not_found"
			);

			System::MessageBoxOK(
				U"EdgeまたはChromeが見つかりません。"
			);

			return false;
		}

		if (not Config::LaunchBrowser(
			browser_,
			game.path,
			browserProcess_
		))
		{
			LauncherLog::Write(
				"GAME_START_FAILED",
				context + U" reason=browser_launch_failed"
			);

			System::MessageBoxOK(
				U"ブラウザを起動できませんでした。"
			);

			return false;
		}

		browserDetectionStartedAt_ =
			::GetTickCount64();

		active_ = true;

		LauncherLog::Write(
			"GAME_START",
			context + U" type=browser"
		);

		return true;

#else

		LauncherLog::Write(
			"GAME_START_FAILED",
			context + U" reason=unsupported_platform"
		);

		return false;

#endif
	}

	// ========================================================
	// EXEゲーム
	// ========================================================

	if (not FileSystem::IsFile(game.path))
	{
		LauncherLog::Write(
			"GAME_START_FAILED",
			context + U" reason=exe_not_found"
		);

		System::MessageBoxOK(
			U"ゲームの実行ファイルが見つかりません。\n"
			+ game.path
		);

		return false;
	}

	try
	{
#if SIV3D_PLATFORM(WINDOWS)

		// 起動前のウィンドウを記録
		existingExeWindows_ =
			GetExistingExeWindows();

		currentExePath_ = game.path;

		waitingForExeWindow_ = false;
		exeWindowDetectionStartedAt_ = 0;

#endif

		process_ = ChildProcess{
			game.path
		};

		if (not static_cast<bool>(*process_))
		{
			process_.reset();
			active_ = false;

#if SIV3D_PLATFORM(WINDOWS)
			existingExeWindows_.clear();
			currentExePath_.clear();
			waitingForExeWindow_ = false;
#endif

			LauncherLog::Write(
				"GAME_START_FAILED",
				context
				+ U" reason=process_creation_failed"
			);

			System::MessageBoxOK(
				U"ゲームの起動に失敗しました。"
			);

			return false;
		}

		active_ = true;

#if SIV3D_PLATFORM(WINDOWS)

		// 起動後の新規ウィンドウ検出を開始
		waitingForExeWindow_ = true;

		exeWindowDetectionStartedAt_ =
			::GetTickCount64();

#endif

		LauncherLog::Write(
			"GAME_START",
			context + U" type=exe"
		);

		return true;
	}
	catch (const std::exception& e)
	{
		process_.reset();
		active_ = false;

#if SIV3D_PLATFORM(WINDOWS)
		existingExeWindows_.clear();
		currentExePath_.clear();
		waitingForExeWindow_ = false;
		exeWindowDetectionStartedAt_ = 0;
#endif

		LauncherLog::Write(
			"GAME_START_FAILED",
			context
			+ U" reason=exception "
			+ Unicode::FromUTF8(e.what())
		);

		System::MessageBoxOK(
			U"ゲームの起動に失敗しました。"
		);

		return false;
	}
}

// ============================================================
// 管理者による復旧要求
// ============================================================

bool GameRunner::RequestForceRecovery()
{
	if (not active_)
	{
		return false;
	}

	if (recoveryRequested_)
	{
		return true;
	}

	// ========================================================
	// EXEゲーム
	// ========================================================

	if (process_)
	{
		if (not process_->isRunning())
		{
			return false;
		}

		try
		{
			process_->terminate();

			recoveryRequested_ = true;

			LauncherLog::Write(
				"GAME_RECOVERY_REQUESTED",
				U"gameId=" + currentGameId_
				+ U" type=exe"
			);

			return true;
		}
		catch (const std::exception& e)
		{
			LauncherLog::Write(
				"GAME_RECOVERY_FAILED",
				U"gameId=" + currentGameId_
				+ U" type=exe reason="
				+ Unicode::FromUTF8(e.what())
			);

			return false;
		}
	}

#if SIV3D_PLATFORM(WINDOWS)

	// ========================================================
	// ブラウザゲーム
	// ========================================================

	if (
		not browserProcess_.windowHandle
		|| not Config::IsTrackedWindowOpen(
			browserProcess_
		)
	)
	{
		LauncherLog::Write(
			"GAME_RECOVERY_FAILED",
			U"gameId=" + currentGameId_
			+ U" type=browser"
			+ U" reason=tracked_window_unavailable"
		);

		return false;
	}

	if (
		not ::PostMessageW(
			browserProcess_.windowHandle,
			WM_CLOSE,
			0,
			0
		)
	)
	{
		LauncherLog::Write(
			"GAME_RECOVERY_FAILED",
			U"gameId=" + currentGameId_
			+ U" type=browser"
			+ U" reason=post_wm_close_failed"
			+ U" win32Error="
			+ Format(::GetLastError())
		);

		return false;
	}

	recoveryRequested_ = true;

	LauncherLog::Write(
		"GAME_RECOVERY_REQUESTED",
		U"gameId=" + currentGameId_
		+ U" type=browser"
	);

	return true;

#else

	return false;

#endif
}

// ============================================================
// ゲーム実行状態の更新
// ============================================================

GameRunStatus GameRunner::Update()
{
	if (not active_)
	{
		return GameRunStatus::Idle;
	}

	// ========================================================
	// EXEゲームの終了監視
	// ========================================================

	if (process_)
	{
		if (process_->isRunning())
		{
#if SIV3D_PLATFORM(WINDOWS)

			// ================================================
			// EXEゲームのウィンドウ検出と前面化
			// ================================================

			if (waitingForExeWindow_)
			{
				const HWND hwnd =
					FindExeGameWindow(
						currentExePath_,
						existingExeWindows_
					);

				if (hwnd)
				{
					const bool activated = ActivateGameWindow(
						hwnd,
						currentGameId_,
						U"exe"
					);

					if (activated)
					{
						Window::Minimize();
					}

					// 前面化の要求は一度だけ
					waitingForExeWindow_ = false;

					existingExeWindows_.clear();
					currentExePath_.clear();

					exeWindowDetectionStartedAt_ = 0;
				}
				else
				{
					const uint64 elapsed =
						::GetTickCount64()
						- exeWindowDetectionStartedAt_;

					if (elapsed >= ExeWindowTimeoutMs)
					{
						waitingForExeWindow_ = false;

						existingExeWindows_.clear();
						currentExePath_.clear();

						exeWindowDetectionStartedAt_ = 0;

						LauncherLog::Write(
							"GAME_WINDOW_ACTIVATION_TIMEOUT",
							U"gameId=" + currentGameId_
							+ U" type=exe"
							+ U" elapsedMs="
							+ Format(elapsed)
						);
					}
				}
			}

#endif

			return GameRunStatus::Running;
		}

		// ====================================================
		// EXE終了処理
		// ====================================================

		const auto exitCode =
			process_->getExitCode();

		WriteExitCode(
			currentGameId_,
			exitCode
		);

#if SIV3D_PLATFORM(WINDOWS)

		waitingForExeWindow_ = false;
		existingExeWindows_.clear();
		currentExePath_.clear();

		exeWindowDetectionStartedAt_ = 0;

#endif

		process_.reset();
		active_ = false;

		RestoreLauncherToForeground(
			currentGameId_
		);

		if (recoveryRequested_)
		{
			LauncherLog::Write(
				"GAME_FORCE_TERMINATED",
				U"gameId=" + currentGameId_
				+ U" type=exe"
			);

			recoveryRequested_ = false;

			// 管理者復旧後はアンケートを表示しない
			return GameRunStatus::LaunchFailed;
		}

		return GameRunStatus::Finished;
	}

#if SIV3D_PLATFORM(WINDOWS)

	// ========================================================
	// ブラウザウィンドウ検出
	// ========================================================

	if (browserProcess_.waitingForWindow)
	{
		browserProcess_.windowHandle =
			Config::FindNewBrowserWindow(
				browserProcess_
			);

		if (browserProcess_.windowHandle)
		{
			HWND hwnd =
				browserProcess_.windowHandle;

			::GetWindowThreadProcessId(
				hwnd,
				&browserProcess_.windowProcessId
			);

			browserProcess_.waitingForWindow = false;

			LauncherLog::Write(
				"GAME_BROWSER_WINDOW_FOUND",
				U"gameId=" + currentGameId_
			);

			// ブラウザは検出時に一度だけ前面化する
			if (
				Config::IsTrackedWindowOpen(
					browserProcess_
				)
			)
			{
				const bool activated = ActivateGameWindow(
					hwnd,
					currentGameId_,
					U"browser"
				);

				if (activated)
				{
					Window::Minimize();
				}
			}
		}
	}

	// ========================================================
	// ブラウザ検出タイムアウト
	// ========================================================

	if (not browserProcess_.windowHandle)
	{
		const uint64 elapsed =
			::GetTickCount64()
			- browserDetectionStartedAt_;

		if (elapsed >= BrowserWindowTimeoutMs)
		{
			LauncherLog::Write(
				"GAME_RUNTIME_FAILED",
				U"gameId=" + currentGameId_
				+ U" reason=browser_window_timeout"
				+ U" elapsedMs=" + Format(elapsed)
			);

			browserProcess_.Close();

			active_ = false;
			recoveryRequested_ = false;
			browserDetectionStartedAt_ = 0;

			RestoreLauncherToForeground(
				currentGameId_
			);

			System::MessageBoxOK(
				U"ゲーム用ブラウザウィンドウを"
				U"検出できませんでした。"
			);

			return GameRunStatus::LaunchFailed;
		}

		return GameRunStatus::Running;
	}

	// ========================================================
	// ブラウザゲームの終了監視
	// ========================================================

	if (
		not Config::IsTrackedWindowOpen(
			browserProcess_
		)
	)
	{
		browserProcess_.Close();

		active_ = false;
		browserDetectionStartedAt_ = 0;

		RestoreLauncherToForeground(
			currentGameId_
		);

		if (recoveryRequested_)
		{
			LauncherLog::Write(
				"GAME_RECOVERY_COMPLETED",
				U"gameId=" + currentGameId_
				+ U" type=browser"
			);

			recoveryRequested_ = false;

			// 復旧後のアンケートは表示しない
			return GameRunStatus::LaunchFailed;
		}

		LauncherLog::Write(
			"GAME_EXIT",
			U"gameId=" + currentGameId_
			+ U" type=browser"
			+ U" code=unavailable"
		);

		return GameRunStatus::Finished;
	}

	return GameRunStatus::Running;

#else

	active_ = false;
	recoveryRequested_ = false;

	RestoreLauncherToForeground(
		currentGameId_
	);

	return GameRunStatus::LaunchFailed;

#endif
}
