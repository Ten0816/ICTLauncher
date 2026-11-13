#pragma once
#include <Siv3D.hpp>

// JSON のキー識別子から表示用の文字列に変換する（入力制御はしない）。
namespace KeyGuide
{
    String DeviceName(const String& id);
    String KeyName(const String& id);
    String ReadRegular(const JSON& root);
    String ReadExit(const JSON& root);
}
