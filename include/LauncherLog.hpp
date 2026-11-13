#pragma once
#include <Siv3D.hpp>

#include <filesystem>
#include <fstream>
#include <ctime>
#include <string>
#include <system_error>
#include <mutex>
#include <cstdint>

// PCごとのローカルテキストログ。
// ログの書き込み・ローテーションに失敗してもLauncherの進行を妨げない。
// このヘッダーだけで実装し、既存の呼び出し側に変更は不要。
namespace LauncherLog
{
	// 現行ファイル5 MiB、退避ログ3世代（launcher.log.1～.3）。
	inline constexpr std::uintmax_t MaxLogSizeBytes = 5ull * 1024ull * 1024ull;
	inline constexpr int BackupGenerations = 3;

	struct State
	{
		std::filesystem::path path;
		std::string deviceId = "UNSET";
		std::mutex mutex;
	};

	inline State& GetState()
	{
		static State state;
		return state;
	}

	inline std::string Timestamp()
	{
		const std::time_t now = std::time(nullptr);
		std::tm local{};
#if SIV3D_PLATFORM(WINDOWS)
		if (localtime_s(&local, &now) != 0) return "unknown-time";
#else
		if (localtime_r(&now, &local) == nullptr) return "unknown-time";
#endif
		char buffer[32]{};
		if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local) == 0)
			return "unknown-time";
		return buffer;
	}

	// launcher.log.1, launcher.log.2 ... という名前を作る。
	inline std::filesystem::path BackupPath(const std::filesystem::path& logPath, int generation)
	{
		std::filesystem::path result = logPath;
		result += "." + std::to_string(generation);
		return result;
	}

	// 必要なときだけローテーションする。
	// renameに失敗した場合、既存のログは消さずに追記を続ける。
	// （このとき容量上限は一時的に超える可能性がある。）
	inline void RotateIfNeeded(const std::filesystem::path& logPath,
							   std::size_t nextLineBytes) noexcept
	{
		try
		{
			namespace fs = std::filesystem;
			std::error_code ec;
			const bool exists = fs::exists(logPath, ec);
			if (ec || !exists) return;

			const std::uintmax_t currentSize = fs::file_size(logPath, ec);
			if (ec) return;

			// 次の書き込みでも上限以内なら何もしない。
			if (currentSize <= MaxLogSizeBytes
				&& nextLineBytes <= (MaxLogSizeBytes - currentSize))
			{
				return;
			}

			// 空のログのローテーションは不要。
			if (currentSize == 0) return;

			// 古いバックアップを最初に削除する。
			fs::remove(BackupPath(logPath, BackupGenerations), ec);
			if (ec) return;

			// .2 -> .3, .1 -> .2 の順に退避する。
			for (int generation = BackupGenerations - 1; generation >= 1; --generation)
			{
				const fs::path source = BackupPath(logPath, generation);
				ec.clear();
				if (!fs::exists(source, ec))
				{
					if (ec) return;
					continue;
				}

				fs::rename(source, BackupPath(logPath, generation + 1), ec);
				if (ec) return;
			}

			// 現行ログ -> .1
			fs::rename(logPath, BackupPath(logPath, 1), ec);
		}
		catch (...) {} // ログ処理でLauncherを停止させない。
	}

	inline void Initialize(const s3d::FilePath& moduleDirectory,
						   const s3d::String& deviceId)
	{
		auto& state = GetState();
		std::lock_guard<std::mutex> lock(state.mutex);

		try
		{
			state.deviceId = deviceId.toUTF8();
			const auto directory =
				std::filesystem::path(moduleDirectory.toWstr()) / L"logs";
			std::filesystem::create_directories(directory);
			state.path = directory / L"launcher.log";
		}
		catch (...)
		{
			state.path.clear();
		}
	}

	inline void Write(const char* event,
					  const s3d::String& details = U"") noexcept
	{
		try
		{
			auto& state = GetState();
			std::lock_guard<std::mutex> lock(state.mutex);
			if (state.path.empty()) return;

			// 既存の「日時 | PC識別子 | イベント | 詳細」形式を維持する。
			const std::string line = Timestamp() + " | "
				+ state.deviceId + " | "
				+ (event ? event : "") + " | "
				+ details.toUTF8() + '\n';

			RotateIfNeeded(state.path, line.size());

			std::ofstream stream(state.path, std::ios::binary | std::ios::app);
			if (!stream) return;
			stream.write(line.data(), static_cast<std::streamsize>(line.size()));
		}
		catch (...) {} // ログ保存失敗で展示を停止しない。
	}
}
