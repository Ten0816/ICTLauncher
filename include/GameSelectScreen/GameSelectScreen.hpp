#pragma once

#include "../Game.hpp"
#include "GameCarousel.hpp"
#include "GameInformationPanel.hpp"
#include "LauncherHeader.hpp"

#include <cstddef>

// ゲーム選択画面全体を管理する。
// 外部へは画面上の操作を返す。
enum class GameSelectAction
{
	None,
	PlayRequested,
	SurveyRequested
};

class GameSelectScreen
{
public:
	GameSelectAction UpdateAndDraw(
		const Array<Game>& games,
		const Texture& launcherLogo
	);

	size_t SelectedIndex() const
	{
		return carousel_.SelectedIndex();
	}

	void Select(size_t index)
	{
		if (carousel_.SelectedIndex() != index)
		{
			carousel_.Select(index);
			informationPanel_.ResetScroll();
		}
	}

	void CollapseDescription()
	{
		informationPanel_.Collapse();
	}

private:
	// 選択中のゲーム画像
	void DrawGameArtwork(const Game& game) const;

	// ゲーム起動ボタン
	void DrawPlayButton() const;

	// 操作デバイス案内
	void DrawControls(const Game& game) const;

	// ゲーム一覧
	GameCarousel carousel_;

	// ゲームの詳細情報
	GameInformationPanel informationPanel_;

	// ロゴ帯・操作案内・カウンター
	LauncherHeader header_;

	// ゲーム切り替え検出用
	size_t previousSelectedIndex_ =
		static_cast<size_t>(-1);
};
