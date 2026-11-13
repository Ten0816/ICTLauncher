
#include "../../include/GameSelectScreen/GameCarousel.hpp"
#include "../../include/UI.hpp"

namespace
{
	constexpr double TileSideMargin = 50.0;
	constexpr double NavigationButtonRadius = 19.0;

	// ========================================================
	// 左右のナビゲーションボタン
	// ========================================================

	Circle LeftNavigationButton()
	{
		return Circle{
			Vec2{ 25.0, UI::BaseTilePos.y },
			NavigationButtonRadius
		};
	}

	Circle RightNavigationButton()
	{
		return Circle{
			Vec2{
				UI::GameListArea.rightX() - 25.0,
				UI::BaseTilePos.y
			},
			NavigationButtonRadius
		};
	}
}

// ============================================================
// 更新処理
// ============================================================

void GameCarousel::Update(const Array<Game>& games)
{
	if (games.isEmpty())
	{
		selectGameIndex_ = 0;
		UpdateTileScroll(0);
		return;
	}

	if (selectGameIndex_ >= games.size())
	{
		selectGameIndex_ = games.size() - 1;
	}

	UpdateSelection(games);
	UpdateTileScroll(games.size());
}

// ============================================================
// ゲーム選択
// ============================================================

void GameCarousel::UpdateSelection(
	const Array<Game>& games)
{
	if (games.isEmpty())
	{
		selectGameIndex_ = 0;
		return;
	}

	if (selectGameIndex_ >= games.size())
	{
		selectGameIndex_ = games.size() - 1;
	}

	// ========================================================
	// マウス選択
	// ========================================================

	if (UI::GameListArea.mouseOver())
	{
		for (auto i : step(games.size()))
		{
			const Vec2 center =
				UI::BaseTilePos.movedBy(
					tileOffsetX_
					+ i * UI::TileSize,
					0
				);

			const RectF tile{
				Arg::center = center,
				UI::TileSize - 20
			};

			if (tile.leftClicked())
			{
				selectGameIndex_ = i;
				break;
			}
		}
	}

	// ========================================================
	// キーボード選択
	// ========================================================

	if (KeyLeft.down())
	{
		if (selectGameIndex_ > 0)
		{
			--selectGameIndex_;
		}
	}
	else if (KeyRight.down())
	{
		if (selectGameIndex_ + 1 < games.size())
		{
			++selectGameIndex_;
		}
	}

	// ========================================================
	// ホイール選択
	// ========================================================

	if (UI::GameListArea.mouseOver())
	{
		const double wheel = Mouse::Wheel();

		if (wheel > 0.0)
		{
			if (selectGameIndex_ > 0)
			{
				--selectGameIndex_;
			}
		}
		else if (wheel < 0.0)
		{
			if (selectGameIndex_ + 1 < games.size())
			{
				++selectGameIndex_;
			}
		}
	}

	// ========================================================
	// 左右の矢印ボタンによるゲーム選択
	// ========================================================

	if (
		selectGameIndex_ > 0
		&& LeftNavigationButton().leftClicked()
	)
	{
		--selectGameIndex_;
	}
	else if (
		selectGameIndex_ + 1 < games.size()
		&& RightNavigationButton().leftClicked()
	)
	{
		++selectGameIndex_;
	}
}

// ============================================================
// ゲームタイルの横スクロール
// ============================================================

void GameCarousel::UpdateTileScroll(
	const size_t gameCount)
{
	if (gameCount == 0)
	{
		tileOffsetX_ = 0.0;
		targetTileOffsetX_ = 0.0;
		tileOffsetXVelocity_ = 0.0;
		return;
	}

	constexpr double TileMargin = 10.0;

	// 選択枠の拡張幅（DrawTilesのstretched(6)に対応）
	constexpr double SelectionFrameMargin = 8.0;

	const double tileWidth =
		UI::TileSize - TileMargin * 2.0;

	// 青い選択枠と安全余白を含めて計算
	const double halfWidth =
		tileWidth / 2.0 + SelectionFrameMargin;

	const double lastTileRight =
		UI::BaseTilePos.x
		+ (gameCount - 1) * UI::TileSize
		+ halfWidth;

	const double minOffset = Min(
		0.0,
		UI::GameListArea.rightX()
		- TileSideMargin
		- lastTileRight
	);

	constexpr double maxOffset = 0.0;

	const double selectedCenterX =
		UI::BaseTilePos.x
		+ selectGameIndex_ * UI::TileSize
		+ targetTileOffsetX_;

	const double selectedLeft =
		selectedCenterX - halfWidth;

	const double selectedRight =
		selectedCenterX + halfWidth;

	const double visibleLeft =
		UI::GameListArea.x + TileSideMargin;

	const double visibleRight =
		UI::GameListArea.rightX() - TileSideMargin;

	if (selectedLeft < visibleLeft)
	{
		targetTileOffsetX_ +=
			visibleLeft - selectedLeft;
	}
	else if (selectedRight > visibleRight)
	{
		targetTileOffsetX_ -=
			selectedRight - visibleRight;
	}

	targetTileOffsetX_ = Clamp(
		targetTileOffsetX_,
		minOffset,
		maxOffset
	);

	tileOffsetX_ = Math::SmoothDamp(
		tileOffsetX_,
		targetTileOffsetX_,
		tileOffsetXVelocity_,
		0.1
	);

	if (
		Abs(tileOffsetX_ - targetTileOffsetX_) < 0.1
	)
	{
		tileOffsetX_ = targetTileOffsetX_;
		tileOffsetXVelocity_ = 0.0;
	}
}

// ============================================================
// ゲームタイルの描画
// ============================================================

void GameCarousel::DrawTiles(
	const Array<Game>& games) const
{
	// ========================================================
	// 論理座標から描画ピクセルへの変換
	// ========================================================

	const double offsetX =
		(Scene::Width() - UI::RenderSize.x) / 2.0;

	const double offsetY =
		(Scene::Height() - UI::RenderSize.y) / 2.0;

	// ========================================================
	// クリッピング範囲
	// ========================================================

	const Rect clipRect{
		static_cast<int32>(
			offsetX
			+ (UI::GameListArea.x + TileSideMargin)
			* UI::RenderScale
		),
		static_cast<int32>(
			offsetY
			+ UI::GameListArea.y * UI::RenderScale
		),
		static_cast<int32>(
			(UI::GameListArea.w - TileSideMargin * 2.0)
			* UI::RenderScale
		),
		static_cast<int32>(
			UI::GameListArea.h * UI::RenderScale
		)
	};

	Graphics2D::SetScissorRect(clipRect);

	{
		RasterizerState rs = RasterizerState::Default2D;
		rs.scissorEnable = true;

		const ScopedRenderStates2D clipState{ rs };

		// ====================================================
		// 各ゲームタイルの描画
		// ====================================================

		for (auto [i, g] : Indexed(games))
		{
			const double rawX =
				UI::BaseTilePos.x
				+ tileOffsetX_
				+ i * UI::TileSize;

			// 実描画ピクセルに合わせて座標を丸める
			const double snappedX =
				Math::Round(rawX * UI::RenderScale)
				/ UI::RenderScale;

			const Vec2 center{
				snappedX,
				UI::BaseTilePos.y
			};

			const RectF tile{
				Arg::center = center,
				UI::TileSize - 20
			};

			// 完全に表示範囲外なら描画を省略
			if (
				tile.rightX() < UI::GameListArea.x
				|| tile.x > UI::GameListArea.rightX()
			)
			{
				continue;
			}

			// =================================================
			// 選択中のタイル
			// =================================================

			if (selectGameIndex_ == i)
			{
				// 選択中のタイル背景
				tile.stretched(6)
					.draw(ColorF{ 0.88, 0.95, 1.0 });

				// 常時表示する青い選択枠
				tile.stretched(6)
					.drawFrame(
						4.0,
						0.0,
						ColorF{ 0.05, 0.55, 0.95 }
					);
			}

			// =================================================
			// ゲーム画像
			// =================================================

			if (g.texture)
			{
				const double scale = Min(
					tile.w / g.texture.width(),
					tile.h / g.texture.height()
				);

				const ScopedRenderStates2D sampler{
					SamplerState::ClampNearest
				};

				g.texture
					.scaled(scale)
					.drawAt(center);
			}

			// =================================================
			// マウスカーソル
			// =================================================

			if (
				UI::GameListArea.mouseOver()
				&& tile.mouseOver()
			)
			{
				Cursor::RequestStyle(
					CursorStyle::Hand
				);
			}
		}
	}

	// ========================================================
	// シザー矩形を元に戻す
	// ========================================================

	Graphics2D::SetScissorRect(
		Rect{
			0,
			0,
			Scene::Width(),
			Scene::Height()
		}
	);
}

// ============================================================
// 左右ナビゲーション矢印の描画
// ============================================================

void GameCarousel::DrawNavigationArrows(
	const size_t gameCount) const
{
	if (gameCount <= 1)
	{
		return;
	}

	const Circle leftButton =
		LeftNavigationButton();

	const Circle rightButton =
		RightNavigationButton();

	const bool canGoLeft =
		(selectGameIndex_ > 0);

	const bool canGoRight =
		(selectGameIndex_ + 1 < gameCount);

	// ========================================================
	// 矢印ボタンの描画
	// ========================================================

	const auto drawArrow = [](
		const Circle& button,
		const bool enabled,
		const bool pointsRight)
		{
			const bool hovered =
				enabled && button.mouseOver();

			const ColorF background =
				not enabled
				? ColorF{ 0.90, 0.92, 0.95 }
				: hovered
				? ColorF{ 0.78, 0.91, 1.0 }
			: ColorF{ 1.0 };

			const ColorF arrowColor =
				enabled
				? ColorF{ 0.05, 0.55, 0.95 }
			: ColorF{ 0.65, 0.70, 0.75 };

			button.draw(background);

			button.drawFrame(
				2.0,
				0.0,
				arrowColor
			);

			const Vec2 center = button.center;

			// ====================================================
			// 右向き矢印
			// ====================================================

			if (pointsRight)
			{
				Line{
					center.movedBy(-4, -8),
					center.movedBy(4, 0)
				}.draw(3.0, arrowColor);

				Line{
					center.movedBy(4, 0),
					center.movedBy(-4, 8)
				}.draw(3.0, arrowColor);
			}
			// ====================================================
			// 左向き矢印
			// ====================================================
			else
			{
				Line{
					center.movedBy(4, -8),
					center.movedBy(-4, 0)
				}.draw(3.0, arrowColor);

				Line{
					center.movedBy(-4, 0),
					center.movedBy(4, 8)
				}.draw(3.0, arrowColor);
			}

			if (hovered)
			{
				Cursor::RequestStyle(
					CursorStyle::Hand
				);
			}
		};

	drawArrow(leftButton, canGoLeft, false);
	drawArrow(rightButton, canGoRight, true);
}
