
#include "../../include/GameSelectScreen/GameSelectScreen.hpp"
#include "../../include/UI.hpp"

// ============================================================
// メイン更新・描画
// ============================================================

GameSelectAction GameSelectScreen::UpdateAndDraw(
	const Array<Game>& games,
	const Texture& launcherLogo)
{
	if (games.isEmpty())
	{
		return GameSelectAction::None;
	}

	// ========================================================
	// ゲーム一覧の更新
	// ========================================================

	carousel_.Update(games);

	const size_t selectedIndex =
		carousel_.SelectedIndex();

	// ゲーム切り替え時に詳細欄のスクロールを初期化
	if (previousSelectedIndex_ != selectedIndex)
	{
		informationPanel_.ResetScroll();
		previousSelectedIndex_ = selectedIndex;
	}

	const Game& game = games[selectedIndex];

	// ========================================================
	// 詳細欄の更新
	// ========================================================

	informationPanel_.Update(game);

	// ========================================================
	// 起動ボタン
	// ========================================================

	if (UI::PlayButton.leftClicked())
	{
		return GameSelectAction::PlayRequested;
	}

	// ========================================================
	// 描画
	// ========================================================

	// ロゴ帯・操作案内・ゲーム数カウンター
	header_.Draw(
		launcherLogo,
		selectedIndex,
		games.size()
	);

	// 選択中のゲーム画像
	DrawGameArtwork(game);

	// ゲーム一覧
	carousel_.DrawTiles(games);
	carousel_.DrawNavigationArrows(games.size());

	// 制作情報
	informationPanel_.DrawCredits(game);

	// 起動ボタン・操作デバイス
	DrawPlayButton();
	DrawControls(game);

	// タイトル・説明文・アンケートボタン
	if (informationPanel_.DrawDescription(game))
	{
		return GameSelectAction::SurveyRequested;
	}

	return GameSelectAction::None;
}

// ============================================================
// 選択中のゲーム画像
// ============================================================

void GameSelectScreen::DrawGameArtwork(
	const Game& game) const
{
	UI::LogoArea
		.rounded(UI::InfoAreaRound)
		.draw(ColorF{ 1.0 });

	if (not game.texture)
	{
		return;
	}

	const double scale = Min(
		UI::LogoArea.w / game.texture.width(),
		UI::LogoArea.h / game.texture.height()
	);

	const ScopedRenderStates2D sampler{
		SamplerState::ClampNearest
	};

	game.texture
		.scaled(scale)
		.drawAt(UI::LogoArea.center());
}

// ============================================================
// あそぶボタン
// ============================================================

void GameSelectScreen::DrawPlayButton() const
{
	const ColorF buttonColor =
		UI::PlayButton.mouseOver()
		? ColorF{ 0.2, 0.75, 1.0 }
	: UI::PlayButtonColor;

	UI::PlayButton
		.rounded(UI::InfoAreaRound)
		.draw(buttonColor);

	const Transformer2D t(
		Mat3x2::Scale(
			0.95
			+ Periodic::Sine0_1(1.2s) * 0.05,
			UI::PlayButton.center()
		)
	);

	TextureAsset(U"Icon.Play")
		.drawAt(
			UI::PlayButton.center()
				.movedBy(-60, 0)
		);

	FontAsset(U"Game.Play")(U"あそぶ")
		.draw(
			30,
			Arg::leftCenter =
				UI::PlayButton.center()
					.movedBy(-25, 0)
		);

	if (UI::PlayButton.mouseOver())
	{
		Cursor::RequestStyle(
			CursorStyle::Hand
		);
	}
}

// ============================================================
// 操作デバイス
// ============================================================

void GameSelectScreen::DrawControls(
	const Game& game) const
{
	UI::ControlArea
		.rounded(UI::InfoAreaRound)
		.draw(
			UI::ControlArea.mouseOver()
			? UI::InfoAreaMouseOverColor
			: ColorF{ 1.0 }
		);

	String control = U"操作\n";

	control += game.useMouse
		? U"・マウス\n"
		: U"";

	control += game.useKeyboard
		? U"・キーボード\n"
		: U"";

	control += game.useGamepad
		? U"・ゲームパッド\n"
		: U"";

	FontAsset(U"Game.Small")(control)
		.draw(
			16,
			UI::ControlArea.pos.movedBy(30, 20),
			UI::TextColor
		);
}
