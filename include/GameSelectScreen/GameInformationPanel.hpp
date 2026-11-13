#pragma once

#include "../Game.hpp"

// ゲームのタイトル・説明文・制作情報を管理する
class GameInformationPanel
{
public:
	// 詳細欄の入力とアニメーションを更新
	void Update(const Game& game);

	// タイトル・説明文・アンケートボタンの描画
	// アンケートボタンが押された場合はtrue
	bool DrawDescription(const Game& game) const;

	// 制作スタッフ・開発ツールの描画
	void DrawCredits(const Game& game) const;

	// 詳細欄のスクロール位置を初期化
	void ResetScroll();

	// 詳細欄を閉じる
	void Collapse();

private:
	bool isDescriptionExpanded_ = false;

	double descriptionHeight_ = 185.0;
	double descriptionHeightVelocity_ = 0.0;

	double descriptionScrollY_ = 0.0;
	double descriptionScrollTargetY_ = 0.0;
	double descriptionScrollVelocity_ = 0.0;
};

