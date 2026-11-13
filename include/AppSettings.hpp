#pragma once
#include <Siv3D.hpp>

// ICTLauncher 全体の設定を保持する。
// 新しい設定を増やす場合も、このクラスを拡張する。
// 画面遷移・描画は LauncherApp が担当する。
class AppSettings
{
public:
	// 設定が存在しない・不正な場合は安全な既定値を採用する。
	void Load(const s3d::FilePath& path);

	// 終了時刻に達したか（PCのローカル時刻を使用）。
	[[nodiscard]] bool IsExhibitionEnded() const;

	// 各展示PCでlauncher_settings.jsonに設定する識別子。
	[[nodiscard]] const s3d::String& DeviceId() const { return deviceId_; }

private:
	s3d::String deviceId_ = U"UNSET";
	bool exhibitionEnabled_ = false;
	int exhibitionEndMinuteOfDay_ = 0;
};
