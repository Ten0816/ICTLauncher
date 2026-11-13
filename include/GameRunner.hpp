#pragma once

#include "Game.hpp"
#include <Siv3D.hpp>
#include <vector>
#include <memory>

#if SIV3D_PLATFORM(WINDOWS)
#include "Config.hpp"
#endif

// ゲーム実行状態
enum class GameRunStatus
{
	Idle,
	Running,
	Finished,
	Crashed,
	ForceRecovered,
	LaunchFailed
};

class LauncherChildProcess;

class GameRunner
{
public:
	GameRunner();
	~GameRunner();

	GameRunner(const GameRunner&) = delete;
	GameRunner& operator=(const GameRunner&) = delete;

	bool Start(const Game& game);
	GameRunStatus Update();
	bool RequestForceRecovery();

	bool IsActive() const
	{
		return active_;
	}

private:
	std::unique_ptr<LauncherChildProcess> process_;
	bool active_ = false;
	bool recoveryRequested_ = false;
	String currentGameId_;

#if SIV3D_PLATFORM(WINDOWS)
	Config::BrowserInfo browser_;
	Config::BrowserProcess browserProcess_;
	uint64 browserDetectionStartedAt_ = 0;
	uint64 browserRecoveryRequestedAt_ = 0;

	std::vector<HWND> existingExeWindows_;
	FilePath currentExePath_;
	uint64 exeWindowDetectionStartedAt_ = 0;
	bool waitingForExeWindow_ = false;
#endif
};
