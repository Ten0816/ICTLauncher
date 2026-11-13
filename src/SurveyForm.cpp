#include "..\include\SurveyForm.hpp"
#include "..\include\Survey.hpp"
#include "..\include\UI.hpp"

void SurveyForm::Reset()
{
	answerFun_ = 0;
	answerUsability_ = 0;
	answerDifficulty_ = 0;
	answerAge_ = 0;
	answerGood_ = TextAreaEditState{};
	answerImprovement_ = TextAreaEditState{};
	surveyError_.clear();
}

SurveyFormAction SurveyForm::Draw(const Game& surveyGame, const String& deviceId)
{
	const Font& body = FontAsset(U"Survey.Body");
	const Font& surveySmallFont = FontAsset(U"Survey.Small");
	const ColorF ink = UI::TextColor;

	FontAsset(U"Survey.Title")(U"ゲームアンケート")
		.draw(30, Vec2{ 40, 18 }, ink);
	body(U"対象：" + surveyGame.title)
		.draw(18, Arg::topRight = Vec2{ 1230, 27 }, ink);

	const RectF notice{ 40, 75, 1200, 95 };
	notice.rounded(8).draw(ColorF{ 0.93, 0.96, 1.0 });
	body(U"アンケートへのご協力をお願いします")
		.draw(18, Vec2{ 55, 81 }, ink);
	surveySmallFont(U"回答はゲームの評価・分析・改善にのみ使用します。お名前や連絡先は収集しません。")
		.draw(15, Vec2{ 55, 112 }, ink);
	surveySmallFont(U"自由記述には個人情報を入力しないでください。回答は任意ですが、送信時はQ1～Q3への回答が必要です。")
		.draw(15, Vec2{ 55, 139 }, ink);

	const RectF left{ 40, 185, 580, 365 };
	const RectF right{ 635, 185, 605, 365 };
	left.rounded(8).draw(Palette::White);
	right.rounded(8).draw(Palette::White);


	body(U"Q1. このゲームは楽しめましたか？ *")
		.draw(18, Vec2{ 55, 198 }, ink);
	for (int32 n = 1; n <= 5; ++n)
	{
		if (UI::DrawChoiceButton(RectF{ 60 + (n - 1) * 105.0, 235, 90, 37 },
			Format(n), answerFun_ == n)) answerFun_ = n;
	}
	surveySmallFont(U"1：楽しめなかった                      5：とても楽しめた")
		.draw(15, Vec2{ 60, 278 }, ink);

	body(U"Q2. 操作方法は分かりやすかったですか？ *")
		.draw(18, Vec2{ 55, 316 }, ink);
	for (int32 n = 1; n <= 5; ++n)
	{
		if (UI::DrawChoiceButton(RectF{ 60 + (n - 1) * 105.0, 351, 90, 37 },
			Format(n), answerUsability_ == n)) answerUsability_ = n;
	}
	surveySmallFont(U"1：分かりにくい                        5：分かりやすい")
		.draw(15, Vec2{ 60, 393 }, ink);

	body(U"Q3. ゲームの難易度はどうでしたか？ *")
		.draw(18, Vec2{ 55, 433 }, ink);
	const Array<String> difficultyLabels{ U"簡単", U"ちょうどよい", U"難しい" };
	for (int32 n = 1; n <= 3; ++n)
	{
		if (UI::DrawChoiceButton(RectF{ 60 + (n - 1) * 180.0, 478, 165, 43 },
			difficultyLabels[n - 1], answerDifficulty_ == n)) answerDifficulty_ = n;
	}

	body(U"Q4. 良かったところ（任意）").draw(18, Vec2{ 650, 193 }, ink);
	SimpleGUI::TextArea(answerGood_, Vec2{ 650, 224 }, SizeF{ 575, 84 });
	body(U"Q5. 改善してほしいところ（任意）").draw(18, Vec2{ 650, 317 }, ink);
	SimpleGUI::TextArea(answerImprovement_, Vec2{ 650, 348 }, SizeF{ 575, 84 });

	body(U"Q6. あなたの年代を教えてください。（任意）")
		.draw(18, Vec2{ 650, 439 }, ink);
	const Array<String> ageLabels{ U"10代", U"20代", U"30代", U"40代", U"50代", U"60代以上" };
	for (int32 n = 1; n <= 6; ++n)
	{
		const int32 col = (n - 1) % 3;
		const int32 row = (n - 1) / 3;
		if (UI::DrawChoiceButton(RectF{ 650 + col * 192.0, 475 + row * 39.0, 177, 32 },
			ageLabels[n - 1], answerAge_ == n))
		{
			answerAge_ = (answerAge_ == n) ? 0 : n;
		}
	}

	surveySmallFont(U"* は必須項目です").draw(15, Vec2{ 42, 581 }, ink);
	if (!surveyError_.isEmpty())
		surveySmallFont(surveyError_).draw(15, Vec2{ 42, 607 }, ColorF{ 0.85, 0.15, 0.15 });

	if (UI::DrawChoiceButton(RectF{ 870, 570, 160, 50 }, U"キャンセル", false))
	{
		return SurveyFormAction::CancelRequested;
	}
	if (UI::DrawChoiceButton(RectF{ 1040, 570, 200, 50 }, U"送信する", true))
	{
		if (!answerFun_ || !answerUsability_ || !answerDifficulty_)
		{
			surveyError_ = U"Q1～Q3のすべてに回答してください。";
		}
		else if (Survey::Save(surveyGame, answerFun_, answerUsability_,
			answerDifficulty_, answerGood_.text, answerImprovement_.text, answerAge_, deviceId))
		{
			return SurveyFormAction::Submitted;
		}
		else
		{
			surveyError_ = U"保存できませんでした。保存先の書き込み権限を確認してください。";
		}
	}

	return SurveyFormAction::None;
}

void SurveyForm::DrawCompleted()
{
	FontAsset(U"Survey.Title")(U"ご回答ありがとうございました！")
		.drawAt(30, Vec2{ 640, 265 }, UI::TextColor);
	FontAsset(U"Survey.Body")(U"3秒後にゲーム選択画面へ戻ります")
		.drawAt(18, Vec2{ 640, 335 }, UI::TextColor);
}
