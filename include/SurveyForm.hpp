#pragma once
#include "Game.hpp"

// 入力値をフォームに閉じ込める。保存方式の詳細は Survey.cpp が担当。
enum class SurveyFormAction { None, CancelRequested, Submitted };

class SurveyForm
{
public:
	void Reset();
	SurveyFormAction Draw(const Game& game, const String& deviceId);
	static void DrawCompleted();

private:
	int32 answerFun_ = 0;
	int32 answerUsability_ = 0;
	int32 answerDifficulty_ = 0;
	int32 answerAge_ = 0;
	TextAreaEditState answerGood_;
	TextAreaEditState answerImprovement_;
	String surveyError_;
};
