#pragma once
#include "Game.hpp"

// 1回答を App/感想文/<gameId>/*.json に保存する。
namespace Survey
{
	bool Save(const Game& game, int32 fun, int32 usability, int32 difficulty,
		const String& good, const String& improvement, int32 age,
		const String& deviceId);
}
