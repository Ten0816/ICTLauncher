#pragma once

#include "../Game.hpp"
#include <cstddef>

// ゲーム一覧の選択・横スクロール・描画を管理する
class GameCarousel
{
public:
	// 入力処理とスクロール位置の更新
	void Update(const Array<Game>& games);

	// ゲーム一覧の描画
	void DrawTiles(const Array<Game>& games) const;

	// 左右の切り替えボタン
	void DrawNavigationArrows(size_t gameCount) const;

	[[nodiscard]]
	size_t SelectedIndex() const
	{
		return selectGameIndex_;
	}

	void Select(size_t index)
	{
		selectGameIndex_ = index;
	}

private:
	void UpdateSelection(const Array<Game>& games);
	void UpdateTileScroll(size_t gameCount);

	// 選択中のゲーム
	size_t selectGameIndex_ = 0;

	// 横スクロールの状態
	double tileOffsetX_ = 0.0;
	double targetTileOffsetX_ = 0.0;
	double tileOffsetXVelocity_ = 0.0;
};
