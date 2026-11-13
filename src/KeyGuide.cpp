#include "..\include\KeyGuide.hpp"



// ============================================================
// JSONキー案内の表示用整形（未知の識別子もそのまま表示）
// ============================================================
namespace KeyGuide
{
	String DeviceName(const String& id)
	{
		if (id == U"keyboard") return U"キーボード";
		if (id == U"mouse") return U"マウス";
		if (id == U"gamepad") return U"ゲームパッド";
		return id;
	}

	String KeyName(const String& id)
	{
		if (id == U"Escape") return U"Esc";
		if (id == U"LeftControl") return U"左Ctrl";
		if (id == U"RightControl") return U"右Ctrl";
		if (id == U"LeftShift") return U"左Shift";
		if (id == U"RightShift") return U"右Shift";
		if (id == U"Space") return U"Space";
		if (id == U"MouseLeft") return U"左クリック";
		if (id == U"MouseRight") return U"右クリック";
		if (id == U"MouseMiddle") return U"中クリック";
		if (id == U"WheelUp") return U"ホイール上";
		if (id == U"WheelDown") return U"ホイール下";
		if (id == U"DPadUp") return U"十字上";
		if (id == U"DPadDown") return U"十字下";
		if (id == U"DPadLeft") return U"十字左";
		if (id == U"DPadRight") return U"十字右";
		return id;
	}

	String ReadRegular(const JSON& root)
	{
		String result;
		for (const String device : { U"keyboard", U"mouse", U"gamepad" })
		{
			const JSON entries = root[device];
			if (not entries.isArray()) continue;
			String line;
			for (const auto& entry : entries.arrayView())
			{
				if (not entry.isString()) continue;
				if (not line.isEmpty()) line += U" / ";
				line += KeyName(entry.getString());
			}
			if (not line.isEmpty())
			{
				if (not result.isEmpty()) result += U"\n";
				result += DeviceName(device) + U"：" + line;
			}
		}
		return result;
	}

	String ReadExit(const JSON& root)
	{
		String result;
		for (const String device : { U"keyboard", U"mouse", U"gamepad" })
		{
			const JSON combos = root[device];
			if (not combos.isArray()) continue;
			for (const auto& combo : combos.arrayView())
			{
				if (not combo.isArray()) continue;
				String line;
				for (const auto& key : combo.arrayView())
				{
					if (not key.isString()) continue;
					if (not line.isEmpty()) line += U" + ";
					line += KeyName(key.getString());
				}
				if (line.isEmpty()) continue;
				if (not result.isEmpty()) result += U"\n";
				result += DeviceName(device) + U"：" + line;
			}
		}
		return result;
	}
}
