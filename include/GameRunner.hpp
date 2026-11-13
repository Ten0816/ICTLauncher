
#pragma once

#include "Game.hpp"
#include <Siv3D.hpp>
#include <vector>

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


class GameRunner
{
public:
	GameRunner();
	~GameRunner();

	GameRunner(const GameRunner&) = delete;
	GameRunner& operator=(const GameRunner&) = delete;

	// ゲーム起動
	bool Start(const Game& game);

	// 起動中のゲームを監視
	GameRunStatus Update();

	// 管理者による復旧要求
	bool RequestForceRecovery();

	bool IsActive() const
	{
		return active_;
	}

private:
	// EXEプロセス
	Optional<ChildProcess> process_;

	// 共通状態
	bool active_ = false;
	bool recoveryRequested_ = false;

	String currentGameId_;

#if SIV3D_PLATFORM(WINDOWS)

	// ========================================================
	// ブラウザゲーム
	// ========================================================

	Config::BrowserInfo browser_;
	Config::BrowserProcess browserProcess_;

	uint64 browserDetectionStartedAt_ = 0;

	// ========================================================
	// EXEゲームのウィンドウ前面化
	// ========================================================

	// ゲーム起動前から存在したウィンドウ
	std::vector<HWND> existingExeWindows_;

	// 起動対象のEXEパス
	FilePath currentExePath_;

	// ウィンドウ検出開始時刻
	uint64 exeWindowDetectionStartedAt_ = 0;

	// ウィンドウを検出するまでtrue
	bool waitingForExeWindow_ = false;

#endif
};
