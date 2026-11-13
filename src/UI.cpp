#include "..\include\UI.hpp"

namespace UI
{
	// フォントは実描画解像度に合わせて高精細に生成し、
	// 描画時に論理ピクセルサイズを指定する。
	void RegisterFonts()
	{
		FontAsset::Register(U"Game.Title", FontMethod::Bitmap, 63, Typeface::Heavy);
		FontAsset::Register(U"Game.Desc", FontMethod::Bitmap, 39);
		FontAsset::Register(U"Game.Small", FontMethod::Bitmap, 24);
		FontAsset::Register(U"Game.Play", FontMethod::Bitmap, 45, Typeface::Heavy);
		FontAsset::Register(U"Launcher.Guide", FontMethod::Bitmap, 23);
		FontAsset::Register(U"Survey.Title", FontMethod::Bitmap, 45, Typeface::Bold);
		FontAsset::Register(U"Survey.Body", FontMethod::Bitmap, 27);
		FontAsset::Register(U"Survey.Small", FontMethod::Bitmap, 23);
	}


	// アンケート内の選択肢・操作ボタンを共通化する。
	// true はこのフレームでクリックされたことを表す。
	bool DrawChoiceButton(const RectF& rect, const String& label, bool selected)
	{
		const ColorF fill = selected ? PlayButtonColor
			: (rect.mouseOver() ? ColorF{ 0.87, 0.93, 1.0 } : ColorF{ 0.94 });
		rect.rounded(6).draw(fill);
		FontAsset(U"Survey.Small")(label).drawAt(
			15, rect.center(), selected ? ColorF{ 1.0 } : TextColor);
		if (rect.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		return rect.leftClicked();
	}

	// 背面の操作を抑止するため、呼び出し元は確認中に描画後 continue する。
	ConfirmationResult DrawConfirmation(bool openingSurvey)
	{
		RectF{ 0, 0, WindowSize.x, WindowSize.y }.draw(ColorF{ 0.10, 0.13, 0.17 });
		const RectF dialog{ 365, 185, 550, 270 };
		dialog.rounded(14).draw(ColorF{ 1.0 });
		dialog.rounded(14).drawFrame(2, ColorF{ 0.75, 0.83, 0.91 });

		FontAsset(U"Survey.Title")(
			openingSurvey ? U"アンケートの確認" : U"アンケートを終了しますか？")
			.draw(30, Vec2{ 405, 212 }, TextColor);
		FontAsset(U"Survey.Body")(
			openingSurvey ? U"選択中のゲームについてのアンケートを開きます。"
						  : U"入力した回答は保存されません。")
			.draw(18, Vec2{ 405, 290 }, TextColor);
		FontAsset(U"Survey.Body")(
			openingSurvey ? U"回答しますか？" : U"本当に終了しますか？")
			.draw(18, Vec2{ 405, 322 }, TextColor);

		const RectF backButton{ 535, 380, 155, 50 };
		const RectF acceptButton{ 705, 380, 175, 50 };
		backButton.rounded(7).draw(backButton.mouseOver()
			? ColorF{ 0.86, 0.90, 0.95 } : ColorF{ 0.93 });
		acceptButton.rounded(7).draw(acceptButton.mouseOver()
			? ColorF{ 0.12, 0.74, 1.0 } : PlayButtonColor);
		FontAsset(U"Survey.Body")(U"戻る")
			.drawAt(18, backButton.center(), TextColor);
		FontAsset(U"Survey.Body")(openingSurvey ? U"回答する" : U"終了する")
			.drawAt(18, acceptButton.center(), ColorF{ 1.0 });

		if (backButton.mouseOver() || acceptButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		if (backButton.leftClicked() || KeyEscape.down())
		{
			return ConfirmationResult::Back;
		}
		if (acceptButton.leftClicked() || KeyEnter.down())
		{
			return ConfirmationResult::Accept;
		}
		return ConfirmationResult::Pending;
	}

	// ゲーム終了後に感想の記入をお願いする画面。
	ConfirmationResult DrawPostGameSurveyPrompt(const String& title)
	{
		RectF{ 0, 0, WindowSize.x, WindowSize.y }.draw(BackgroundColor);
		const RectF panel{ 265, 160, 750, 330 };
		panel.rounded(14).draw(ColorF{ 1.0 });
		panel.rounded(14).drawFrame(2, ColorF{ 0.72, 0.82, 0.93 });

		FontAsset(U"Survey.Title")(U"プレイありがとうございました！")
			.drawAt(29, Vec2{ 640, 213 }, TextColor);
		FontAsset(U"Survey.Body")(title)
			.drawAt(21, Vec2{ 640, 265 }, TextColor);
		FontAsset(U"Survey.Body")(U"よろしければ、ゲームの感想をお聞かせください。")
			.drawAt(20, Vec2{ 640, 324 }, TextColor);
		FontAsset(U"Survey.Small")(U"回答は任意です。ゲームの改善に役立てます。")
			.drawAt(17, Vec2{ 640, 357 }, TextColor);

		const RectF skip{ 390, 407, 210, 55 };
		const RectF answer{ 625, 407, 265, 55 };
		skip.rounded(8).draw(skip.mouseOver() ? ColorF{ 0.86, 0.90, 0.95 } : ColorF{ 0.93 });
		answer.rounded(8).draw(answer.mouseOver() ? ColorF{ 0.12, 0.74, 1.0 } : PlayButtonColor);
		FontAsset(U"Survey.Body")(U"今は回答しない")
			.drawAt(19, skip.center(), TextColor);
		FontAsset(U"Survey.Body")(U"感想を書く")
			.drawAt(20, answer.center(), ColorF{ 1.0 });

		if (skip.mouseOver() || answer.mouseOver()) Cursor::RequestStyle(CursorStyle::Hand);
		if (skip.leftClicked() || KeyEscape.down()) return ConfirmationResult::Back;
		if (answer.leftClicked() || KeyEnter.down()) return ConfirmationResult::Accept;
		return ConfirmationResult::Pending;
	}

}
