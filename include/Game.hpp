#pragma once
#include <Siv3D.hpp>

// 1ゲーム分の設定。JSON 読み込みは Game.cpp に隔離する。
struct Game
{
	String gameId; // 保存先に使用する一意の識別子
	String title;

	FilePath path;

	FilePath directory; // launcher_info.json のあるフォルダ

	bool isWebApp = false;

	Texture texture;

	String desc;

	String staff;

	String tools;

	String regularKeyGuide;
	String exitKeyGuide;
	String legacyEnd;
	JSON regularKeysJson;
	JSON exitKeysJson;

	bool useMouse = false;

	bool useKeyboard = false;

	bool useGamepad = false;

	int32 priority = 0;
};


// ============================================================
// ゲーム情報の読み込み
// ============================================================


Array<Game> LoadGames();
