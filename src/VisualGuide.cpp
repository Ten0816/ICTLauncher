
#include "..\include\VisualGuide.hpp"
#include "..\include\KeyGuide.hpp"
#include <initializer_list>
#include <utility>
#include <algorithm>

// ============================================================
// Launcher 起動前のビジュアルキーガイド
// JSON識別子はEditorと共通。表示は閲覧専用。
// ============================================================
namespace VisualGuide
{
	struct KeySet
	{
		Array<String> regular;
		Array<Array<String>> exitCombos;
	};

	KeySet Read(const JSON& root, const String& device)
	{
		KeySet result;
		if (not root.isObject() || not root.hasElement(device)) return result;
		const JSON entries = root[device];
		if (not entries.isArray()) return result;
		for (const auto& entry : entries.arrayView())
		{
			if (entry.isString()) result.regular << entry.getString();
			else if (entry.isArray())
			{
				Array<String> combo;
				for (const auto& k : entry.arrayView())
					if (k.isString()) combo << k.getString();
				if (not combo.isEmpty()) result.exitCombos << combo;
			}
		}
		return result;
	}

	bool Contains(const Array<String>& values, const String& id)
	{
		for (const auto& v : values) if (v == id) return true;
		return false;
	}

	bool IsExit(const Array<Array<String>>& combos, const String& id)
	{
		for (const auto& combo : combos)
			if (Contains(combo, id)) return true;
		return false;
	}

	constexpr ColorF Base{ 0.14, 0.19, 0.27 };
	constexpr ColorF Cyan{ 0.00, 0.69, 0.94 };
	constexpr ColorF Red{ 0.92, 0.23, 0.30 };

	void Key(const RectF& rect, const String& id, const String& label,
		const KeySet& regular, const KeySet& exits, double textSize = 13.0)
	{
		const bool r = Contains(regular.regular, id);
		const bool e = IsExit(exits.exitCombos, id);
		const ColorF fill = e ? Red : (r ? Cyan : Base);
		rect.rounded(5).draw(fill);
		rect.rounded(5).drawFrame(1.4, r && e ? Cyan : ColorF{ 0.35, 0.44, 0.56 });
		FontAsset(U"Survey.Small")(label).drawAt(textSize, rect.center(), ColorF{ 1.0 });
	}

	void Keyboard(const KeySet& regular, const KeySet& exits)
	{
		// ANSI フルサイズ。表示幅に収まるよう、実寸の比率を簡略化。
		const double x0 = 105, y0 = 204, u = 39, h = 35, gap = 4;
		auto row = [&](int index, std::initializer_list<std::pair<String, String>> keys,
			std::initializer_list<double> widths)
			{
				double x = x0;
				auto it = widths.begin();
				for (const auto& k : keys)
				{
					const double units = (it != widths.end()) ? *it++ : 1.0;
					const double w = units * u + (units - 1) * gap;
					Key(RectF{ x, y0 + index * (h + gap), w, h }, k.first, k.second, regular, exits, 12);
					x += w + gap;
				}
			};
		row(0, { { U"Escape",U"Esc" },{ U"F1",U"F1" },{ U"F2",U"F2" },{ U"F3",U"F3" },{ U"F4",U"F4" },{ U"F5",U"F5" },{ U"F6",U"F6" },{ U"F7",U"F7" },{ U"F8",U"F8" },{ U"F9",U"F9" },{ U"F10",U"F10" },{ U"F11",U"F11" },{ U"F12",U"F12" } }, {});
		row(1, { { U"Backquote",U"`" },{ U"1",U"1" },{ U"2",U"2" },{ U"3",U"3" },{ U"4",U"4" },{ U"5",U"5" },{ U"6",U"6" },{ U"7",U"7" },{ U"8",U"8" },{ U"9",U"9" },{ U"0",U"0" },{ U"Minus",U"-" },{ U"Equal",U"=" },{ U"Backspace",U"Bksp" } }, { 1,1,1,1,1,1,1,1,1,1,1,1,1,2 });
		row(2, { { U"Tab",U"Tab" },{ U"Q",U"Q" },{ U"W",U"W" },{ U"E",U"E" },{ U"R",U"R" },{ U"T",U"T" },{ U"Y",U"Y" },{ U"U",U"U" },{ U"I",U"I" },{ U"O",U"O" },{ U"P",U"P" },{ U"LeftBracket",U"[" },{ U"RightBracket",U"]" },{ U"Backslash",U"\\" } }, { 1.5,1,1,1,1,1,1,1,1,1,1,1,1,1.5 });
		row(3, { { U"CapsLock",U"Caps" },{ U"A",U"A" },{ U"S",U"S" },{ U"D",U"D" },{ U"F",U"F" },{ U"G",U"G" },{ U"H",U"H" },{ U"J",U"J" },{ U"K",U"K" },{ U"L",U"L" },{ U"Semicolon",U";" },{ U"Quote",U"'" },{ U"Enter",U"Enter" } }, { 1.8,1,1,1,1,1,1,1,1,1,1,1,2.2 });
		row(4, { { U"LeftShift",U"Shift" },{ U"Z",U"Z" },{ U"X",U"X" },{ U"C",U"C" },{ U"V",U"V" },{ U"B",U"B" },{ U"N",U"N" },{ U"M",U"M" },{ U"Comma",U"," },{ U"Period",U"." },{ U"Slash",U"/" },{ U"RightShift",U"Shift" } }, { 2.3,1,1,1,1,1,1,1,1,1,1,2.7 });
		row(5, { { U"LeftControl",U"Ctrl" },{ U"LeftWindows",U"Win" },{ U"Alt",U"Alt" },{ U"Space",U"Space" },{ U"RightAlt",U"Alt" },{ U"RightWindows",U"Win" },{ U"MenuKey",U"Menu" },{ U"RightControl",U"Ctrl" } }, { 1.3,1.2,1.2,6.0,1.2,1.2,1.2,1.3 });
		// 編集キー・方向キー・テンキー
		auto extra = [&](double x, double y, const String& id, const String& label, double w = 39, double hh = 35)
			{ Key(RectF{ x, y, w, hh }, id, label, regular, exits, 11); };
		const double nx = 790, ty = y0 + 39;
		extra(nx, ty, U"Insert", U"Ins"); extra(nx + 43, ty, U"Home", U"Home"); extra(nx + 86, ty, U"PageUp", U"PgUp");
		extra(nx, ty + 39, U"Delete", U"Del"); extra(nx + 43, ty + 39, U"End", U"End"); extra(nx + 86, ty + 39, U"PageDown", U"PgDn");
		extra(nx + 43, ty + 117, U"Up", U"↑");
		extra(nx, ty + 156, U"Left", U"←"); extra(nx + 43, ty + 156, U"Down", U"↓"); extra(nx + 86, ty + 156, U"Right", U"→");
		const double tx = 942;
		const Array<std::pair<String, String>> pad = {
			{ U"NumLock",U"Num" },{ U"NumpadDivide",U"/" },{ U"NumpadMultiply",U"*" },{ U"NumpadSubtract",U"-" },
			{ U"Numpad7",U"7" },{ U"Numpad8",U"8" },{ U"Numpad9",U"9" },{ U"NumpadAdd",U"+" },
			{ U"Numpad4",U"4" },{ U"Numpad5",U"5" },{ U"Numpad6",U"6" },
			{ U"Numpad1",U"1" },{ U"Numpad2",U"2" },{ U"Numpad3",U"3" },{ U"NumpadEnter",U"Ent" },
			{ U"Numpad0",U"0" },{ U"NumpadDecimal",U"." }
		};
		extra(tx, ty, pad[0].first, pad[0].second); extra(tx + 43, ty, pad[1].first, pad[1].second);
		extra(tx + 86, ty, pad[2].first, pad[2].second); extra(tx + 129, ty, pad[3].first, pad[3].second);
		for (int n = 0; n < 3; ++n) extra(tx + 43 * n, ty + 39, pad[4 + n].first, pad[4 + n].second);
		extra(tx + 129, ty + 39, U"NumpadAdd", U"+", 39, 74);
		for (int n = 0; n < 3; ++n) extra(tx + 43 * n, ty + 78, pad[8 + n].first, pad[8 + n].second);
		for (int n = 0; n < 3; ++n) extra(tx + 43 * n, ty + 117, pad[11 + n].first, pad[11 + n].second);
		extra(tx + 129, ty + 117, U"NumpadEnter", U"Ent", 39, 74);
		extra(tx, ty + 156, U"Numpad0", U"0", 82); extra(tx + 86, ty + 156, U"NumpadDecimal", U".");
	}

	// Device illustrations share the same color rules as the keyboard.
	ColorF FillFor(const String& id, const KeySet& regular, const KeySet& exits)
	{
		return IsExit(exits.exitCombos, id) ? Red
			: (Contains(regular.regular, id) ? Cyan : Base);
	}

	ColorF OutlineFor(const String& id, const KeySet& regular, const KeySet& exits)
	{
		return (Contains(regular.regular, id) && IsExit(exits.exitCombos, id))
			? Cyan : ColorF{ 0.43, 0.55, 0.68 };
	}

	void RoundKey(const RectF& rect, double radius, const String& id,
		const String& label, const KeySet& regular, const KeySet& exits,
		double fontSize = 14.0)
	{
		rect.rounded(radius).draw(FillFor(id, regular, exits))
			.drawFrame(2.0, OutlineFor(id, regular, exits));
		FontAsset(U"Survey.Small")(label).drawAt(fontSize, rect.center(), ColorF{ 1.0 });
	}

	void CircleKey(const Vec2& center, double radius, const String& id,
		const String& label, const KeySet& regular, const KeySet& exits,
		double fontSize = 17.0)
	{
		s3d::Circle{ center, radius }.draw(FillFor(id, regular, exits))
			.drawFrame(2.0, OutlineFor(id, regular, exits));
		FontAsset(U"Survey.Small")(label).drawAt(fontSize, center, ColorF{ 1.0 });
	}

	void Mouse(const KeySet& regular, const KeySet& exits)
	{
		// Front-facing mouse. Keep wheel and middle click separate and readable.
		const RectF shell{ 535, 185, 210, 334 };
		shell.rounded(96).draw(ColorF{ 0.10, 0.15, 0.22 })
			.drawFrame(3, ColorF{ 0.32, 0.47, 0.62 });
		RoundKey(RectF{ 549, 199, 85, 114 }, 22, U"MouseLeft", U"左", regular, exits, 20);
		RoundKey(RectF{ 646, 199, 85, 114 }, 22, U"MouseRight", U"右", regular, exits, 20);
		// A narrow wheel sits between the main buttons; no overlap.
		RoundKey(RectF{ 625, 215, 30, 64 }, 13, U"MouseMiddle", U"●", regular, exits, 12);
		// A small central divider gives the illustration a physical feel.
		Line{ 640, 281, 640, 312 }.draw(2, ColorF{ 0.38, 0.52, 0.65 });
		RoundKey(RectF{ 600, 339, 80, 38 }, 15, U"WheelUp", U"↑", regular, exits, 17);
		RoundKey(RectF{ 600, 385, 80, 38 }, 15, U"WheelDown", U"↓", regular, exits, 17);
		// External labels are placed outside the silhouette to avoid collisions.
		RoundKey(RectF{ 375, 341, 138, 44 }, 12, U"MouseSide1", U"サイド1", regular, exits);
		RoundKey(RectF{ 375, 394, 138, 44 }, 12, U"MouseSide2", U"サイド2", regular, exits);
		RoundKey(RectF{ 767, 367, 145, 48 }, 12, U"MouseMove", U"マウス移動", regular, exits);
		Line{ 513, 363, 535, 363 }.draw(2, Cyan);
		Line{ 745, 391, 767, 391 }.draw(2, Cyan);
	}

	void Gamepad(const KeySet& regular, const KeySet& exits)
	{
		// Xbox-style controller: curved grips, circular sticks and face buttons.
		const ColorF shell{ 0.12, 0.17, 0.25 };
		const ColorF shellEdge{ 0.33, 0.45, 0.60 };
		RectF{ 390, 252, 500, 232 }.rounded(104).draw(shell);
		RectF{ 342, 290, 230, 228 }.rounded(102).draw(shell);
		RectF{ 708, 290, 230, 228 }.rounded(102).draw(shell);
		RectF{ 450, 263, 380, 204 }.rounded(80).draw(shell)
			.drawFrame(2, shellEdge);
		// Shoulder controls: upper triggers above the bumpers.
		RoundKey(RectF{ 416, 194, 140, 39 }, 16, U"LT", U"LT", regular, exits);
		RoundKey(RectF{ 724, 194, 140, 39 }, 16, U"RT", U"RT", regular, exits);
		RoundKey(RectF{ 416, 239, 140, 34 }, 12, U"LB", U"LB", regular, exits);
		RoundKey(RectF{ 724, 239, 140, 34 }, 12, U"RB", U"RB", regular, exits);
		// Left and right analog sticks are round. The small inner dot represents click.
		CircleKey(Vec2{ 475, 341 }, 43, U"LeftStick", U"LS", regular, exits);
		CircleKey(Vec2{ 475, 341 }, 13, U"LeftStickClick", U"", regular, exits);
		CircleKey(Vec2{ 685, 442 }, 43, U"RightStick", U"RS", regular, exits);
		CircleKey(Vec2{ 685, 442 }, 13, U"RightStickClick", U"", regular, exits);
		// Center buttons.
		RoundKey(RectF{ 580, 309, 47, 30 }, 12, U"View", U"View", regular, exits, 11);
		RoundKey(RectF{ 652, 309, 47, 30 }, 12, U"Menu", U"Menu", regular, exits, 11);
		// ABXY diamond: Xbox arrangement Y top, X left, B right, A bottom.
		CircleKey(Vec2{ 808, 327 }, 26, U"Y", U"Y", regular, exits);
		CircleKey(Vec2{ 771, 364 }, 26, U"X", U"X", regular, exits);
		CircleKey(Vec2{ 845, 364 }, 26, U"B", U"B", regular, exits);
		CircleKey(Vec2{ 808, 401 }, 26, U"A", U"A", regular, exits);
		// D-pad: cross-shaped cluster in the lower-left.
		RoundKey(RectF{ 503, 405, 34, 37 }, 5, U"DPadUp", U"↑", regular, exits, 15);
		RoundKey(RectF{ 469, 442, 34, 37 }, 5, U"DPadLeft", U"←", regular, exits, 15);
		RoundKey(RectF{ 537, 442, 34, 37 }, 5, U"DPadRight", U"→", regular, exits, 15);
		RoundKey(RectF{ 503, 479, 34, 37 }, 5, U"DPadDown", U"↓", regular, exits, 15);
		RectF{ 503, 442, 34, 37 }.rounded(4).draw(ColorF{ 0.20, 0.27, 0.37 });
	}

	// IDs drawn in each device illustration. An ID absent here cannot be highlighted.
	bool IsIllustrated(const String& device, const String& id)
	{
		static const Array<String> keyboard = { U"0", U"1", U"2", U"3", U"4", U"5", U"6", U"7", U"8", U"9", U"A", U"Alt", U"B", U"Backquote", U"Backslash", U"Backspace", U"C", U"CapsLock", U"Comma", U"D", U"Delete", U"Down", U"E", U"End", U"Enter", U"Equal", U"Escape", U"F", U"F1", U"F10", U"F11", U"F12", U"F2", U"F3", U"F4", U"F5", U"F6", U"F7", U"F8", U"F9", U"G", U"H", U"Home", U"I", U"Insert", U"J", U"K", U"L", U"Left", U"LeftBracket", U"LeftControl", U"LeftShift", U"LeftWindows", U"M", U"MenuKey", U"Minus", U"N", U"NumLock", U"Numpad0", U"Numpad1", U"Numpad2", U"Numpad3", U"Numpad4", U"Numpad5", U"Numpad6", U"Numpad7", U"Numpad8", U"Numpad9", U"NumpadAdd", U"NumpadDecimal", U"NumpadDivide", U"NumpadEnter", U"NumpadMultiply", U"NumpadSubtract", U"O", U"P", U"PageDown", U"PageUp", U"Period", U"Q", U"Quote", U"R", U"Right", U"RightAlt", U"RightBracket", U"RightControl", U"RightShift", U"RightWindows", U"S", U"Semicolon", U"Slash", U"Space", U"T", U"Tab", U"U", U"Up", U"V", U"W", U"X", U"Y", U"Z" };
		static const Array<String> mouse = { U"MouseLeft", U"MouseMiddle", U"MouseMove", U"MouseRight", U"MouseSide1", U"MouseSide2", U"WheelDown", U"WheelUp" };
		static const Array<String> gamepad = { U"A", U"B", U"DPadDown", U"DPadLeft", U"DPadRight", U"DPadUp", U"LB", U"LT", U"LeftStick", U"LeftStickClick", U"Menu", U"RB", U"RT", U"RightStick", U"RightStickClick", U"View", U"X", U"Y" };
		const Array<String>& ids = (device == U"keyboard") ? keyboard
			: ((device == U"mouse") ? mouse : gamepad);
		return Contains(ids, id);
	}

	Array<String> UnknownKeys(const String& device, const KeySet& regular, const KeySet& exits)
	{
		Array<String> unknown;
		auto add = [&](const String& key)
			{
				if (not IsIllustrated(device, key) && not Contains(unknown, key)) unknown << key;
			};
		for (const auto& k : regular.regular) add(k);
		for (const auto& combo : exits.exitCombos)
			for (const auto& k : combo) add(k);
		return unknown;
	}

	// 詳細画面には、図に描けないキーも含めてJSONの値をすべて表示する。
	// キー組み合わせの順序や内容は変更せず、表示用に折り返すだけにする。
	void AppendWrappedLine(Array<String>& lines, const String& source)
	{
		constexpr size_t MaxChars = 44;
		if (source.isEmpty())
		{
			lines << U"";
			return;
		}
		String remaining = source;
		while (remaining.size() > MaxChars)
		{
			lines << remaining.substr(0, MaxChars);
			remaining = U"    " + remaining.substr(MaxChars);
		}
		lines << remaining;
	}

	void DetailsOverlay(const String& device, const KeySet& regular,
		const KeySet& exits, const String& legacy, bool& open, int32& page)
	{
		Array<String> lines;
		lines << U"通常操作キー（このデバイス）";
		if (regular.regular.isEmpty())
		{
			lines << U"登録なし";
		}
		else
		{
			for (size_t i = 0; i < regular.regular.size(); ++i)
			{
				const String& id = regular.regular[i];
				const String marker = IsIllustrated(device, id) ? U"" : U" [図に未対応]";
				AppendWrappedLine(lines, U"・" + KeyGuide::KeyName(id) + marker);
			}
		}

		lines << U"";
		lines << U"終了操作（組ごとに同時押し／各組は別の終了方法）";
		if (exits.exitCombos.isEmpty())
		{
			lines << (legacy.isEmpty() ? U"登録なし" : (U"旧形式の終了説明: " + legacy));
		}
		else
		{
			for (size_t i = 0; i < exits.exitCombos.size(); ++i)
			{
				const auto& combo = exits.exitCombos[i];
				String line = Format(i + 1) + U". ";
				for (size_t j = 0; j < combo.size(); ++j)
				{
					if (j) line += U" + ";
					line += KeyGuide::KeyName(combo[j]);
					if (not IsIllustrated(device, combo[j])) line += U" [図に未対応]";
				}
				AppendWrappedLine(lines, line);
			}
		}

		const Array<String> unknown = UnknownKeys(device, regular, exits);
		if (not unknown.isEmpty())
		{
			lines << U"";
			lines << U"注意：図に表示できないキー識別子";
			for (const auto& id : unknown) AppendWrappedLine(lines, U"・" + id);
		}

		RectF{ 0, 0, UI::WindowSize.x, UI::WindowSize.y }.draw(ColorF{ 0, 0, 0, 0.68 });
		const RectF panel{ 135, 85, 1010, 468 };
		panel.rounded(12).draw(ColorF{ 1.0 }).drawFrame(2, Cyan);
		FontAsset(U"Survey.Title")(U"操作キー・終了キーの詳細").drawAt(24, Vec2{ 640, 119 }, UI::TextColor);
		constexpr int32 PerPage = 12;
		const int32 pages = Max<int32>(1, (static_cast<int32>(lines.size()) + PerPage - 1) / PerPage);
		page = Clamp(page, 0, pages - 1);
		for (int32 i = 0; i < PerPage; ++i)
		{
			const int32 index = page * PerPage + i;
			if (index >= static_cast<int32>(lines.size())) break;
			FontAsset(U"Survey.Small")(lines[index]).draw(16, Vec2{ 170, 154 + 27.0 * i }, UI::TextColor);
		}
		const RectF previous{ 360, 488, 140, 44 };
		const RectF next{ 520, 488, 140, 44 };
		const RectF close{ 780, 488, 150, 44 };
		previous.rounded(6).draw(page > 0 ? Cyan : ColorF{ 0.70 });
		next.rounded(6).draw(page + 1 < pages ? Cyan : ColorF{ 0.70 });
		close.rounded(6).draw(UI::PlayButtonColor);
		FontAsset(U"Survey.Body")(U"前へ").drawAt(17, previous.center(), ColorF{ 1.0 });
		FontAsset(U"Survey.Body")(U"次へ").drawAt(17, next.center(), ColorF{ 1.0 });
		FontAsset(U"Survey.Body")(U"閉じる").drawAt(17, close.center(), ColorF{ 1.0 });
		FontAsset(U"Survey.Small")(Format(page + 1) + U" / " + Format(pages))
			.drawAt(15, Vec2{ 720, 510 }, UI::TextColor);
		if ((previous.leftClicked() || KeyLeft.down()) && page > 0) --page;
		if ((next.leftClicked() || KeyRight.down()) && page + 1 < pages) ++page;
		if (close.leftClicked() || KeyEscape.down()) { open = false; page = 0; }
	}

	// The exit instructions are scoped to the currently selected device.
	// Render keycaps instead of a plain string so first-time players can
	// recognize the physical buttons and simultaneous presses.

	void ExitSummary(
		const KeySet& exits,
		const String& legacy,
		bool& detailsOpen)
	{
		const RectF panel{ 105, 514, 1070, 64 };

		panel.rounded(9)
			.draw(ColorF{ 1.0, 0.965, 0.966 })
			.drawFrame(2, ColorF{ 0.92, 0.43, 0.46 });

		// 終了案内タイトル
		FontAsset(U"Survey.Body")(
			U"ゲームを終えるには"
		).draw(
			17,
			Vec2{ 124, 536 },
			ColorF{ 0.66, 0.12, 0.17 }
		);

		// キー詳細ボタン
		const RectF details{ 1020, 522, 135, 42 };

		details.rounded(6).draw(Cyan);

		FontAsset(U"Survey.Small")(
			U"キー詳細"
		).drawAt(
			14,
			details.center(),
			ColorF{ 1.0 }
		);

		if (details.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (details.leftClicked())
		{
			detailsOpen = true;
		}

		// 終了キーが登録されていない場合
		if (exits.exitCombos.isEmpty())
		{
			const String message = legacy.isEmpty()
				? U"このデバイスには終了キーが登録されていません"
				: U"終了方法：" + legacy;

			FontAsset(U"Survey.Small")(message)
				.draw(
					14,
					Vec2{ 330, 535 },
					UI::TextColor
				);

			return;
		}

		// ========================================================
		// 終了キーの表示領域
		// ========================================================

		constexpr double StartX = 330.0;
		constexpr double MaxX = 1005.0;

		// 上段: キー
		constexpr double KeyY = 521.0;
		constexpr double KeyHeight = 25.0;

		// 下段: 同時押しの補足
		constexpr double NoteY = 552.0;

		constexpr double KeyGap = 7.0;
		constexpr double PlusWidth = 21.0;
		constexpr double AlternativeWidth = 43.0;

		auto KeyWidth = [](const String& label)
			{
				return Max(
					47.0,
					Min(
						115.0,
						16.0 + label.size() * 10.0
					)
				);
			};

		double x = StartX;

		int32 shown = 0;
		bool hasMultiKey = false;

		// 最大3組の終了操作を表示
		for (const auto& combo : exits.exitCombos)
		{
			if (combo.isEmpty())
			{
				continue;
			}

			// ====================================================
			// 描画に必要な幅を計算
			// ====================================================

			double needed = 0.0;

			for (size_t i = 0; i < combo.size(); ++i)
			{
				if (i > 0)
				{
					needed += PlusWidth;
				}

				const String label =
					KeyGuide::KeyName(combo[i]);

				needed += KeyWidth(label) + KeyGap;
			}

			if (shown > 0)
			{
				needed += AlternativeWidth;
			}

			// 表示領域を超える場合は詳細画面に任せる
			if (x + needed > MaxX)
			{
				break;
			}

			// ====================================================
			// 複数の終了方法がある場合
			// ====================================================

			if (shown > 0)
			{
				FontAsset(U"Survey.Small")(
					U"または"
				).drawAt(
					12,
					Vec2{
						x + AlternativeWidth / 2.0,
						KeyY + KeyHeight / 2.0
					},
					UI::TextColor
				);

				x += AlternativeWidth;
			}

			// ====================================================
			// キーの組み合わせを描画
			// ====================================================

			for (size_t i = 0; i < combo.size(); ++i)
			{
				if (i > 0)
				{
					FontAsset(U"Survey.Body")(
						U"+"
					).drawAt(
						14,
						Vec2{
							x + PlusWidth / 2.0,
							KeyY + KeyHeight / 2.0
						},
						UI::TextColor
					);

					x += PlusWidth;
				}

				const String label =
					KeyGuide::KeyName(combo[i]);

				const double w = KeyWidth(label);

				const RectF cap{
					x,
					KeyY,
					w,
					KeyHeight
				};

				cap.rounded(5)
					.draw(Red)
					.drawFrame(
						1.5,
						ColorF{ 0.72, 0.10, 0.15 }
					);

				FontAsset(U"Survey.Small")(label)
					.drawAt(
						12,
						cap.center(),
						ColorF{ 1.0 }
					);

				x += w + KeyGap;
			}

			if (combo.size() > 1)
			{
				hasMultiKey = true;
			}

			++shown;

			if (shown >= 3)
			{
				break;
			}
		}

		// ========================================================
		// 下段の補足テキスト
		// ========================================================

		if (shown == 0)
		{
			FontAsset(U"Survey.Small")(
				U"終了キーの組み合わせは「キー詳細」で確認してください"
			).draw(
				12,
				Vec2{ StartX, NoteY },
				UI::TextColor
			);
		}
		else if (hasMultiKey)
		{
			FontAsset(U"Survey.Small")(
				U"複数のキーは同時に押してください"
			).draw(
				11,
				Vec2{ StartX, NoteY },
				UI::TextColor
			);
		}
	}


	UI::ConfirmationResult Draw(const String& title, const JSON& regularJson,
		const JSON& exitJson, const String& legacy, int32& device, uint8& visitedDevices)
	{
		static bool detailsOpen = false;
		static int32 detailsPage = 0;
		static String lastTitle;
		static int32 lastDevice = -1;
		if (title != lastTitle || device != lastDevice)
		{
			detailsOpen = false;
			detailsPage = 0;
			lastTitle = title;
			lastDevice = device;
		}
		RectF{ 0,0,UI::WindowSize.x,UI::WindowSize.y }.draw(UI::BackgroundColor);
		FontAsset(U"Survey.Title")(U"ゲームの操作案内").drawAt(29, Vec2{ 640,33 }, UI::TextColor);
		// Game title omitted to free vertical space for the guide.
		RectF{ 75,65,1130,560 }.rounded(12).draw(ColorF{ 1.0 }).drawFrame(2, ColorF{ 0.74,0.83,0.92 });
		// Arrow keys switch device tabs, including wrap-around at either end.
		// Mark the destination as visited so keyboard navigation counts as confirmation.
		if (not detailsOpen && (KeyLeft.down() || KeyUp.down()))
		{
			device = (device + 2) % 3;
		}
		else if (not detailsOpen && (KeyRight.down() || KeyDown.down()))
		{
			device = (device + 1) % 3;
		}
		visitedDevices |= static_cast<uint8>(1u << device);
		const Array<String> devices{ U"keyboard",U"mouse",U"gamepad" };
		const Array<String> labels{ U"キーボード",U"マウス",U"ゲームパッド" };
		for (int32 i = 0; i < 3; ++i)
		{
			const RectF tab{ 110 + i * 182.0,89,172,40 };
			tab.rounded(6).draw(device == i ? Cyan : ColorF{ 0.90,0.94,0.98 });
			if ((visitedDevices & (1u << i)) != 0 && device != i)
				RectF{ tab.x + tab.w - 23, tab.y + 5, 15, 15 }.rounded(7).draw(ColorF{ 0.12,0.68,0.39 });
			FontAsset(U"Survey.Body")(labels[i]).drawAt(17, tab.center(), device == i ? ColorF{ 1.0 } : UI::TextColor);
			if (tab.mouseOver()) Cursor::RequestStyle(CursorStyle::Hand);
			if (not detailsOpen && tab.leftClicked())
			{
				device = i;
				visitedDevices |= static_cast<uint8>(1u << i);
			}
		}
		RectF{ 880,95,16,16 }.rounded(3).draw(Cyan);
		FontAsset(U"Survey.Small")(U"操作キー").draw(13, Vec2{ 903,93 }, UI::TextColor);
		RectF{ 1010,95,16,16 }.rounded(3).draw(Red);
		FontAsset(U"Survey.Small")(U"終了キー").draw(13, Vec2{ 1033,93 }, UI::TextColor);
		const KeySet regular = Read(regularJson, devices[device]);
		const KeySet exits = Read(exitJson, devices[device]);
		// Move the illustrations and exit card up to reserve a clear status line above the buttons.
		// The scene is 1280x640; the old card extended below its bottom edge.
		{
			{
				const Transformer2D shiftUp{
					Mat3x2::Translate(0, -55),
					TransformCursor::Yes
				};

				if (device == 0) Keyboard(regular, exits);
				else if (device == 1) Mouse(regular, exits);
				else Gamepad(regular, exits);

				ExitSummary(exits, legacy, detailsOpen);
			}
		}
		const auto unknown = UnknownKeys(devices[device], regular, exits);
		if (not unknown.isEmpty())
		{
			FontAsset(U"Survey.Small")(U"図に未対応のキーあり：キー詳細で確認")
				.draw(12, Vec2{ 840, 132 }, Red);
		}
		const bool allViewed = (visitedDevices & 0x07) == 0x07;
		const int32 viewedCount = ((visitedDevices & 1u) ? 1 : 0)
			+ ((visitedDevices & 2u) ? 1 : 0) + ((visitedDevices & 4u) ? 1 : 0);
		const String progress = allViewed
			? U"すべて確認しました。ゲームを始められます"
			: U"ゲームを始めるには、3種類の操作案内をすべて確認してください（"
			+ Format(viewedCount) + U" / 3）";
		FontAsset(U"Survey.Body")(progress).drawAt(18, Vec2{ 640,547 },
			allViewed ? ColorF{ 0.12,0.55,0.32 } : UI::TextColor);
		const RectF back{ 420,574,190,42 }, start{ 635,574,225,42 };
		back.rounded(7).draw(back.mouseOver() ? ColorF{ 0.85,0.90,0.95 } : ColorF{ 0.93 });
		start.rounded(7).draw(!allViewed ? ColorF{ 0.65,0.68,0.72 }
		: (start.mouseOver() ? ColorF{ 0.12,0.74,1.0 } : UI::PlayButtonColor));
		FontAsset(U"Survey.Body")(U"戻る").drawAt(19, back.center(), UI::TextColor);
		FontAsset(U"Survey.Body")(U"ゲームを起動").drawAt(19, start.center(), ColorF{ 1.0 });
		if (back.mouseOver() || (allViewed && start.mouseOver())) Cursor::RequestStyle(CursorStyle::Hand);
		// The overlay consumes navigation and Escape so it cannot launch a game.
		if (detailsOpen)
		{
			DetailsOverlay(devices[device], regular, exits, legacy, detailsOpen, detailsPage);
			return UI::ConfirmationResult::Pending;
		}
		if (back.leftClicked() || KeyEscape.down()) return UI::ConfirmationResult::Back;
		if (allViewed && (start.leftClicked() || KeyEnter.down())) return UI::ConfirmationResult::Accept;
		return UI::ConfirmationResult::Pending;
	}
}
