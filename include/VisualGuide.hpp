#pragma once
#include <Siv3D.hpp>
#include "UI.hpp"

// 事前操作案内の描画。キー定義は launcher_info.json から読み取る。
namespace VisualGuide
{
    UI::ConfirmationResult Draw(const String& title, const JSON& regularJson,
        const JSON& exitJson, const String& legacy,
        int32& device, uint8& visitedDevices);
}
