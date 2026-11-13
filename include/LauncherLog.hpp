#pragma once
#include <Siv3D.hpp>
#include <filesystem>
#include <fstream>
#include <ctime>

// PCごとのローカルテキストログ。ログ失敗はゲームの進行を妨げない。
// 追加の .cpp は不要（ヘッダーのみで実装）。
namespace LauncherLog
{
    struct State
    {
        std::filesystem::path path;
        std::string deviceId = "UNSET";
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

    inline void Initialize(const s3d::FilePath& moduleDirectory, const s3d::String& deviceId)
    {
        try
        {
            auto& state = GetState();
            state.deviceId = deviceId.toUTF8();
            const auto directory = std::filesystem::path(moduleDirectory.toWstr()) / L"logs";
            std::filesystem::create_directories(directory);
            state.path = directory / L"launcher.log";
        }
        catch (...)
        {
            GetState().path.clear();
        }
    }

    inline void Write(const char* event, const s3d::String& details = U"") noexcept
    {
        try
        {
            const auto& state = GetState();
            if (state.path.empty()) return;
            std::ofstream stream(state.path, std::ios::binary | std::ios::app);
            if (not stream) return;
            stream << Timestamp() << " | " << state.deviceId << " | "
                   << event << " | " << details.toUTF8() << '\n';
        }
        catch (...) {} // ログ保存失敗で展示を停止しない
    }
}
