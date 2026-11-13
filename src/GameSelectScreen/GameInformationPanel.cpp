#include "../../include/GameSelectScreen/GameInformationPanel.hpp"
#include "../../include/UI.hpp"

namespace
{
	constexpr double ExpandedDescriptionHeight = 260.0;

	constexpr double DescriptionLineHeight = 37.0;
	constexpr double DescriptionTopMargin = 60.0;
	constexpr double DescriptionBottomMargin = 15.0;

	// ========================================================
	// 文字サイズの自動調整
	// ========================================================

	double GetFittedFontSize(
		const Font& font,
		const String& text,
		const double maxWidth,
		const double preferredSize,
		const double minimumSize)
	{
		double fontSize = preferredSize;

		while (fontSize > minimumSize)
		{
			if (font(text).region(fontSize).w <= maxWidth)
			{
				break;
			}

			fontSize -= 1.0;
		}

		return Max(fontSize, minimumSize);
	}

	// ========================================================
	// 省略文字列
	// ========================================================

	String FitTextWithEllipsis(
		const Font& font,
		const String& text,
		const double fontSize,
		const double maxWidth)
	{
		if (maxWidth <= 0.0)
		{
			return U"";
		}

		if (font(text).region(fontSize).w <= maxWidth)
		{
			return text;
		}

		const String ellipsis = U"…";

		if (font(ellipsis).region(fontSize).w > maxWidth)
		{
			return U"";
		}

		String result = text;

		while (not result.isEmpty())
		{
			result.pop_back();

			const String candidate = result + ellipsis;

			if (font(candidate).region(fontSize).w <= maxWidth)
			{
				return candidate;
			}
		}

		return ellipsis;
	}

	// ========================================================
	// テキストを行単位に折り返す
	// ========================================================

	Array<String> WrapText(
		const Font& font,
		const String& text,
		const double fontSize,
		const double maxWidth)
	{
		Array<String> lines;
		String current;

		for (const char32 ch : text)
		{
			if (ch == U'\r')
			{
				continue;
			}

			if (ch == U'\n')
			{
				lines << current;
				current.clear();
				continue;
			}

			String candidate = current;
			candidate += ch;

			if (
				not current.isEmpty()
				&& font(candidate).region(fontSize).w > maxWidth
			)
			{
				lines << current;
				current.clear();
			}

			current += ch;
		}

		if (not current.isEmpty() || lines.isEmpty())
		{
			lines << current;
		}

		return lines;
	}

	// ========================================================
	// 詳細表示用の行データ
	// ========================================================

	struct DescriptionLine
	{
		String text;
		bool isTitle = false;
	};

	Array<DescriptionLine> BuildDescriptionLines(
		const Game& game,
		const double textWidth)
	{
		Array<DescriptionLine> lines;

		const Font& titleFont =
			FontAsset(U"Game.Title");

		const Font& descFont =
			FontAsset(U"Game.Desc");

		// タイトル全文
		const Array<String> titleLines =
			WrapText(
				titleFont,
				game.title,
				28.0,
				textWidth
			);

		for (const String& line : titleLines)
		{
			lines << DescriptionLine{
				line,
				true
			};
		}

		// タイトルと説明文の間隔
		lines << DescriptionLine{ U"", false };

		// 説明文全文
		const Array<String> descLines =
			WrapText(
				descFont,
				game.desc,
				26.0,
				textWidth
			);

		for (const String& line : descLines)
		{
			lines << DescriptionLine{
				line,
				false
			};
		}

		// 制作スタッフ・開発ツール
		const auto appendSection = [&](
			const String& heading,
			const String& content)
			{
				if (content.isEmpty())
				{
					return;
				}

				lines << DescriptionLine{ U"", false };

				lines << DescriptionLine{
					heading,
					true
				};

				const Array<String> wrappedLines =
					WrapText(
						descFont,
						content,
						26.0,
						textWidth
					);

				for (const String& line : wrappedLines)
				{
					lines << DescriptionLine{
						line,
						false
					};
				}
			};

		appendSection(
			U"制作スタッフ",
			game.staff
		);

		appendSection(
			U"開発ツール",
			game.tools
		);

		return lines;
	}
}

// ============================================================
// スクロール位置の初期化
// ============================================================

void GameInformationPanel::ResetScroll()
{
	descriptionScrollY_ = 0.0;
	descriptionScrollTargetY_ = 0.0;
	descriptionScrollVelocity_ = 0.0;
}

// ============================================================
// 詳細欄を閉じる
// ============================================================

void GameInformationPanel::Collapse()
{
	isDescriptionExpanded_ = false;
	ResetScroll();
}

// ============================================================
// 詳細欄の更新
// ============================================================

void GameInformationPanel::Update(
	const Game& game)
{
	const RectF currentInfoArea{
		UI::InfoArea.x,
		UI::InfoArea.y,
		UI::InfoArea.w,
		descriptionHeight_
	};

	const RectF surveyButton{
		currentInfoArea.rightX() - 255,
		currentInfoArea.y + 12,
		135,
		33
	};

	// ========================================================
	// 開閉処理
	// ========================================================

	if (
		currentInfoArea.leftClicked()
		&& not surveyButton.mouseOver()
	)
	{
		isDescriptionExpanded_ =
			not isDescriptionExpanded_;

		ResetScroll();
	}

	// ========================================================
	// 高さのアニメーション
	// ========================================================

	const double targetHeight =
		isDescriptionExpanded_
		? ExpandedDescriptionHeight
		: UI::InfoArea.h;

	descriptionHeight_ = Math::SmoothDamp(
		descriptionHeight_,
		targetHeight,
		descriptionHeightVelocity_,
		0.15
	);

	if (not isDescriptionExpanded_)
	{
		return;
	}

	// ========================================================
	// スクロール領域
	// ========================================================

	const RectF scrollArea{
		UI::InfoArea.x + 30,
		UI::InfoArea.y + DescriptionTopMargin,
		Max(0.0, UI::InfoArea.w - 75),
		Max(
			0.0,
			descriptionHeight_
				- DescriptionTopMargin
				- DescriptionBottomMargin
		)
	};

	// ========================================================
	// 全文の高さを計算
	// ========================================================

	const Array<DescriptionLine> lines =
		BuildDescriptionLines(
			game,
			scrollArea.w
		);

	const double contentHeight =
		lines.size() * DescriptionLineHeight;

	const double maxScrollY =
		Max(
			0.0,
			contentHeight - scrollArea.h
		);

	// ========================================================
	// ホイール入力
	// ========================================================

	if (scrollArea.mouseOver())
	{
		const double wheel = Mouse::Wheel();

		constexpr double ScrollSpeed = 70.0;

		descriptionScrollTargetY_ +=
			wheel * ScrollSpeed;
	}

	descriptionScrollTargetY_ = Clamp(
		descriptionScrollTargetY_,
		0.0,
		maxScrollY
	);

	// ========================================================
	// スムーズスクロール
	// ========================================================

	descriptionScrollY_ = Math::SmoothDamp(
		descriptionScrollY_,
		descriptionScrollTargetY_,
		descriptionScrollVelocity_,
		0.12
	);

	descriptionScrollY_ = Clamp(
		descriptionScrollY_,
		0.0,
		maxScrollY
	);

	if (
		Abs(
			descriptionScrollY_
			- descriptionScrollTargetY_
		) < 0.1
	)
	{
		descriptionScrollY_ =
			descriptionScrollTargetY_;

		descriptionScrollVelocity_ = 0.0;
	}
}

// ============================================================
// 制作スタッフ・開発ツール
// ============================================================

void GameInformationPanel::DrawCredits(
	const Game& game) const
{
	UI::StaffArea
		.rounded(UI::InfoAreaRound)
		.draw(
			UI::StaffArea.mouseOver()
			? UI::InfoAreaMouseOverColor
			: ColorF{ 1.0 }
		);

	constexpr double MarginX = 30.0;
	constexpr double FontSize = 16.0;

	const double textWidth = Max(
		0.0,
		UI::StaffArea.w - MarginX * 2.0
	);

	const RectF staffArea{
		UI::StaffArea.x + MarginX,
		UI::StaffArea.y + 10,
		textWidth,
		24
	};

	const RectF toolsArea{
		UI::StaffArea.x + MarginX,
		UI::StaffArea.y + 35,
		textWidth,
		24
	};

	const Font& font =
		FontAsset(U"Game.Small");

	// スタッフ名
	const String staffText =
		FitTextWithEllipsis(
			font,
			game.staff,
			FontSize,
			staffArea.w
		);

	font(staffText).draw(
		FontSize,
		staffArea.pos,
		UI::TextColor
	);

	// 開発ツール
	const String toolsText =
		U"開発ツール: " + game.tools;

	const String displayTools =
		FitTextWithEllipsis(
			font,
			toolsText,
			FontSize,
			toolsArea.w
		);

	font(displayTools).draw(
		FontSize,
		toolsArea.pos,
		UI::TextColor
	);
}

// ============================================================
// タイトル・説明文
// ============================================================

bool GameInformationPanel::DrawDescription(
	const Game& game) const
{
	const RectF infoArea{
		UI::InfoArea.x,
		UI::InfoArea.y,
		UI::InfoArea.w,
		descriptionHeight_
	};

	// ========================================================
	// 背景
	// ========================================================

	infoArea
		.rounded(UI::InfoAreaRound)
		.draw(
			infoArea.mouseOver()
			? UI::InfoAreaMouseOverColor
			: ColorF{ 1.0 }
		);

	const Font& titleFont =
		FontAsset(U"Game.Title");

	const Font& descFont =
		FontAsset(U"Game.Desc");

	// ========================================================
	// 通常表示
	// ========================================================

	if (not isDescriptionExpanded_)
	{
		// 右上のボタンとの重なりを避ける
		const RectF titleArea{
			infoArea.x + 30,
			infoArea.y + 15,
			Max(0.0, infoArea.w - 330),
			75
		};

		// 1行で収まる場合は大きく表示
		const double singleLineSize =
			GetFittedFontSize(
				titleFont,
				game.title,
				titleArea.w,
				42.0,
				30.0
			);

		const bool fitsSingleLine =
			not game.title.includes(U'\n')
			&& titleFont(game.title)
			.region(singleLineSize).w
			<= titleArea.w;

		double descY = infoArea.y + 85;

		if (fitsSingleLine)
		{
			titleFont(game.title).draw(
				singleLineSize,
				titleArea.pos,
				UI::TextColor
			);
		}
		else
		{
			// 長いタイトルは最大2行
			constexpr double TitleFontSize = 28.0;
			constexpr double TitleLineHeight = 35.0;

			const Array<String> titleLines =
				WrapText(
					titleFont,
					game.title,
					TitleFontSize,
					titleArea.w
				);

			const size_t visibleLines =
				Min<size_t>(2, titleLines.size());

			for (size_t i = 0; i < visibleLines; ++i)
			{
				String line = titleLines[i];

				// 3行目以降がある場合、2行目を省略
				if (
					i == visibleLines - 1
					&& titleLines.size() > visibleLines
				)
				{
					line += U"…";
				}

				line = FitTextWithEllipsis(
					titleFont,
					line,
					TitleFontSize,
					titleArea.w
				);

				titleFont(line).draw(
					TitleFontSize,
					Vec2{
						titleArea.x,
						titleArea.y
							+ i * TitleLineHeight
					},
					UI::TextColor
				);
			}

			// 2行タイトルに合わせて説明文を下げる
			descY = infoArea.y + 100;
		}

		// ====================================================
		// 説明文
		// ====================================================

		constexpr double DescFontSize = 24.0;
		constexpr double DescLineHeight = 32.0;
		constexpr double BottomMargin = 12.0;

		const double descWidth =
			Max(0.0, infoArea.w - 60);

		const double descHeight =
			Max(
				0.0,
				infoArea.bottomY()
					- BottomMargin
					- descY
			);

		const Array<String> descLines =
			WrapText(
				descFont,
				game.desc,
				DescFontSize,
				descWidth
			);

		const size_t maxVisibleLines =
			static_cast<size_t>(
				descHeight / DescLineHeight
			);

		const size_t visibleDescLines =
			Min(maxVisibleLines, descLines.size());

		for (size_t i = 0; i < visibleDescLines; ++i)
		{
			String line = descLines[i];

			if (
				i == visibleDescLines - 1
				&& descLines.size() > visibleDescLines
			)
			{
				line += U"…";
			}

			line = FitTextWithEllipsis(
				descFont,
				line,
				DescFontSize,
				descWidth
			);

			descFont(line).draw(
				DescFontSize,
				Vec2{
					infoArea.x + 30,
					descY + i * DescLineHeight
				},
				UI::TextColor
			);
		}
	}
	// ========================================================
	// 詳細表示：全文スクロール
	// ========================================================
	else
	{
		const RectF scrollArea{
			infoArea.x + 30,
			infoArea.y + DescriptionTopMargin,
			Max(0.0, infoArea.w - 75),
			Max(
				0.0,
				infoArea.h
					- DescriptionTopMargin
					- DescriptionBottomMargin
			)
		};

		const Array<DescriptionLine> lines =
			BuildDescriptionLines(
				game,
				scrollArea.w
			);

		// ====================================================
		// 論理座標から実描画ピクセルへ変換
		// ====================================================

		const double offsetX =
			(Scene::Width() - UI::RenderSize.x) / 2.0;

		const double offsetY =
			(Scene::Height() - UI::RenderSize.y) / 2.0;

		const Rect clipRect{
			static_cast<int32>(
				offsetX
				+ scrollArea.x * UI::RenderScale
			),
			static_cast<int32>(
				offsetY
				+ scrollArea.y * UI::RenderScale
			),
			static_cast<int32>(
				scrollArea.w * UI::RenderScale
			),
			static_cast<int32>(
				scrollArea.h * UI::RenderScale
			)
		};

		// ====================================================
		// 詳細欄の文章だけを切り抜く
		// ====================================================

		Graphics2D::SetScissorRect(clipRect);

		{
			RasterizerState rs =
				RasterizerState::Default2D;

			rs.scissorEnable = true;

			const ScopedRenderStates2D clipState{ rs };

			for (size_t i = 0; i < lines.size(); ++i)
			{
				const DescriptionLine& line = lines[i];

				const double rawY =
					scrollArea.y
					+ i * DescriptionLineHeight
					- descriptionScrollY_;

				const double y =
					Math::Round(
						rawY * UI::RenderScale
					) / UI::RenderScale;

				if (
					y + DescriptionLineHeight < scrollArea.y
					|| y >= scrollArea.bottomY()
				)
				{
					continue;
				}

				const double fontSize =
					line.isTitle ? 28.0 : 26.0;

				const Font& font =
					line.isTitle ? titleFont : descFont;

				font(line.text).draw(
					fontSize,
					Vec2{ scrollArea.x, y },
					UI::TextColor
				);
			}
		}

		// 他のUIへ影響しないように戻す
		Graphics2D::SetScissorRect(
			Rect{
				0,
				0,
				Scene::Width(),
				Scene::Height()
			}
		);
	}

	// ========================================================
	// アンケートボタン
	// ========================================================

	const RectF surveyButton{
		infoArea.rightX() - 255,
		infoArea.y + 12,
		135,
		33
	};

	surveyButton
		.rounded(6)
		.draw(
			surveyButton.mouseOver()
			? ColorF{ 0.78, 0.91, 1.0 }
			: ColorF{ 0.90, 0.95, 1.0 }
		);

	FontAsset(U"Game.Small")(U"意見・感想")
		.drawAt(
			16,
			surveyButton.center(),
			UI::TextColor
		);

	if (surveyButton.mouseOver())
	{
		Cursor::RequestStyle(
			CursorStyle::Hand
		);
	}

	if (surveyButton.leftClicked())
	{
		return true;
	}

	// ========================================================
	// 詳細・閉じる
	// ========================================================

	const String detailsText =
		isDescriptionExpanded_
		? U"▲ 閉じる"
		: U"▼ 詳細";

	FontAsset(U"Game.Small")(detailsText)
		.draw(
			16,
			Arg::topRight =
				infoArea.tr().movedBy(-20, 15),
			UI::TextColor
		);

	if (infoArea.mouseOver())
	{
		Cursor::RequestStyle(
			CursorStyle::Hand
		);
	}

	return false;
}
