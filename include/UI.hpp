#pragma once
#include <Siv3D.hpp>

// 画面共通の基準値。論理座標は1280x640、描画は1.5倍。
namespace UI
{
	inline constexpr Size WindowSize{ 1280, 640 };
	inline constexpr double RenderScale = 1.5;
	inline constexpr Size RenderSize{ 1920, 960 };

	// 基本配色とUIスタイル
	inline constexpr ColorF BackgroundColor{ 0.85, 0.9, 0.95 };
	inline constexpr ColorF TextColor{ 0.2 };
	inline constexpr ColorF InfoAreaMouseOverColor{ 1.0, 0.95, 0.9 };
	inline constexpr double InfoAreaRound = 8.0;

	// ゲーム一覧
	inline constexpr double TileSize = 250.0;
	inline constexpr ColorF TileFrameColor{ 1.0, 0.7, 0.3 };
	inline constexpr RectF GameListArea{ 0, 80, 1280, 250 };

	// 上部ロゴとランチャー共通の操作説明
	inline constexpr RectF LauncherGuideArea{ 0, 50, 1280, 30 };
	inline constexpr ColorF LauncherGuideBackground{ 0.93, 0.96, 1.0 };
	inline constexpr ColorF LauncherGuideTextColor{ 0.20, 0.30, 0.45 };
	inline constexpr Vec2 BaseTilePos{ 240, 205 };

	// プレイボタン
	inline constexpr ColorF PlayButtonColor{ 0.0, 0.67, 1.0 };

	// ========================================================
	// 下部UIの相対配置
	// ========================================================

	// 下部UI全体の幅
	inline constexpr double BottomUIWidth = 1100.0;

	// 画面中央に配置するための左端座標
	inline constexpr double BottomUIX =
		(WindowSize.x - BottomUIWidth) / 2.0;

	// ロゴ
	inline constexpr RectF LogoArea{
		BottomUIX,
		375,
		220,
		220
	};

	// ゲーム説明
	inline constexpr RectF InfoArea{
		BottomUIX + 230,
		340,
		645,
		185
	};

	// スタッフ情報
	inline constexpr RectF StaffArea{
		BottomUIX + 230,
		530,
		645,
		70
	};

	// プレイボタン
	inline constexpr RectF PlayButton{
		BottomUIX + 880,
		340,
		220,
		85
	};

	// 操作方法
	inline constexpr RectF ControlArea{
		BottomUIX + 880,
		430,
		220,
		170
	};
}


// ============================================================
// UI 共通処理
// ============================================================


namespace UI
{
    enum class ConfirmationResult { Pending, Back, Accept };

    void RegisterFonts();
    bool DrawChoiceButton(const RectF& rect, const String& label, bool selected);
    ConfirmationResult DrawConfirmation(bool openingSurvey);
    ConfirmationResult DrawPostGameSurveyPrompt(const String& title);
}
