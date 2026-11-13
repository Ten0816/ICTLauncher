
#include "../../include/GameSelectScreen/LauncherHeader.hpp"
#include "../../include/UI.hpp"

namespace
{
	constexpr double LogoHeight = 36.0;
	constexpr double LogoSpacing = 90.0;
	constexpr double LogoScrollSpeed = 80.0;
	constexpr int32 LogoBandHeight = 50;
}

// ============================================================
// ヘッダー全体の描画
// ============================================================

void LauncherHeader::Draw(
	const Texture& launcherLogo,
	const size_t selectedIndex,
	const size_t gameCount)
{
	DrawLogoBand(launcherLogo);
	DrawCommonGuide();
	DrawGameCounter(selectedIndex, gameCount);
}

// ============================================================
// ロゴ帯
// ============================================================

void LauncherHeader::DrawLogoBand(
	const Texture& launcherLogo)
{
	const Rect logoArea{
		0,
		0,
		UI::WindowSize.x,
		LogoBandHeight
	};

	logoArea.draw(ColorF{ 1.0 });

	RectF{
		0,
		LogoBandHeight - 2.0,
		static_cast<double>(UI::WindowSize.x),
		2.0
	}.draw(ColorF{ 0.20, 0.55, 0.85 });

	if (not launcherLogo)
	{
		return;
	}

	const double scale =
		LogoHeight / launcherLogo.height();

	const double logoWidth =
		launcherLogo.width() * scale;

	const double interval =
		logoWidth + LogoSpacing;

	logoScrollX_ +=
		LogoScrollSpeed * Scene::DeltaTime();

	if (logoScrollX_ >= interval)
	{
		logoScrollX_ = Math::Fmod(
			logoScrollX_,
			interval
		);
	}

	const Rect scaledLogoArea{
		0,
		0,
		static_cast<int32>(UI::RenderSize.x),
		static_cast<int32>(
			LogoBandHeight * UI::RenderScale
		)
	};

	const ScopedViewport2D viewport{
		scaledLogoArea
	};

	const int32 logoCount =
		static_cast<int32>(
			UI::WindowSize.x / interval
		) + 3;

	for (int32 i = -1; i < logoCount; ++i)
	{
		const double x =
			i * interval - logoScrollX_;

		launcherLogo
			.scaled(scale)
			.draw(
				x,
				(LogoBandHeight - LogoHeight) / 2.0
			);
	}
}

// ============================================================
// 操作案内
// ============================================================

void LauncherHeader::DrawCommonGuide() const
{
	UI::LauncherGuideArea.draw(
		UI::LauncherGuideBackground
	);

	Line{
		0, 80,
		UI::WindowSize.x, 80
	}.draw(
		1.0,
		ColorF{ 0.65, 0.78, 0.90 }
	);

	const String guide =
		U"画像をクリックしてゲームを選択"
		U"   ｜   "
		U"あそぶボタンで起動"
		U"   ｜   "
		U"左右キーでゲームを切り替え";

	// 右端にカウンター用の領域を確保
	const RectF guideTextArea{
		UI::LauncherGuideArea.x + 15,
		UI::LauncherGuideArea.y,
		UI::LauncherGuideArea.w - 155,
		UI::LauncherGuideArea.h
	};

	FontAsset(U"Launcher.Guide")(guide)
		.drawAt(
			15,
			guideTextArea.center(),
			UI::LauncherGuideTextColor
		);
}

// ============================================================
// ゲーム数カウンター
// ============================================================

void LauncherHeader::DrawGameCounter(
	const size_t selectedIndex,
	const size_t gameCount) const
{
	if (gameCount == 0)
	{
		return;
	}

	const RectF counterArea{
		UI::LauncherGuideArea.rightX() - 115,
		UI::LauncherGuideArea.y + 2,
		95,
		UI::LauncherGuideArea.h - 4
	};

	counterArea
		.rounded(13)
		.draw(ColorF{ 1.0, 0.95 })
		.drawFrame(
			1.0,
			0.0,
			ColorF{ 0.65, 0.78, 0.90 }
		);

	const String counterText =
		Format(selectedIndex + 1)
		+ U" / "
		+ Format(gameCount);

	FontAsset(U"Game.Small")(counterText)
		.drawAt(
			16,
			counterArea.center(),
			UI::TextColor
		);
}
