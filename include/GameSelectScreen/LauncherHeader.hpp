#pragma once

#include "../Game.hpp"
#include <cstddef>

// Launcher上部のロゴ帯・操作案内・ゲーム数を描画する
class LauncherHeader
{
public:
	void Draw(
		const Texture& launcherLogo,
		size_t selectedIndex,
		size_t gameCount
	);

private:
	void DrawLogoBand(const Texture& launcherLogo);
	void DrawCommonGuide() const;
	void DrawGameCounter(
		size_t selectedIndex,
		size_t gameCount
	) const;

	// ロゴ帯のスクロール状態
	double logoScrollX_ = 0.0;
};
