#include "..\include\AppSettings.hpp"

#include <ctime>

namespace
{
	// HH:MM（24時間表記）を午前0時からの経過分に変換する。
	bool ParseTime(const s3d::String& value, int& minutes)
	{
		if (value.size() != 5 || value[2] != U':') return false;
		for (const size_t index : { 0u, 1u, 3u, 4u })
		{
			if (value[index] < U'0' || value[index] > U'9') return false;
		}

		const int hour = (value[0] - U'0') * 10 + (value[1] - U'0');
		const int minute = (value[3] - U'0') * 10 + (value[4] - U'0');
		if (hour > 23 || minute > 59) return false;
		minutes = hour * 60 + minute;
		return true;
	}
}

void AppSettings::Load(const s3d::FilePath& path)
{
	// 再読み込みした場合も古い設定を引きずらない。
	deviceId_ = U"UNSET";
	exhibitionEnabled_ = false;
	exhibitionEndMinuteOfDay_ = 0;

	if (not s3d::FileSystem::Exists(path))
	{
		s3d::Print << U"ランチャー設定がありません（自動展示終了は無効）";
		return;
	}

	const s3d::JSON root = s3d::JSON::Load(path);
	if (not root || not root.isObject())
	{
		s3d::Print << U"ランチャー設定のJSONが不正です: " << path;
		return;
	}

	// 展示PC IDは時刻設定とは独立して読み込む。
	if (root.hasElement(U"device") && root[U"device"].isObject())
	{
		const s3d::JSON device = root[U"device"];
		if (device.hasElement(U"id") && device[U"id"].isString())
		{
			const s3d::String id = device[U"id"].getString();
			// ログ・回収データの識別子として扱う。改行や空白は許可しない。
			bool valid = (not id.isEmpty() && id.size() <= 64);
			for (const char32 ch : id)
			{
				if (not ((ch >= U'0' && ch <= U'9')
					|| (ch >= U'A' && ch <= U'Z')
					|| (ch >= U'a' && ch <= U'z')
					|| ch == U'-' || ch == U'_')) valid = false;
			}
			if (valid) deviceId_ = id;
		}
	}
	if (deviceId_ == U"UNSET")
	{
		s3d::Print << U"device.id が未設定または不正です。展示PCごとに設定してください。";
	}

	// 他の機能の設定を今後追加しても、exhibition 以外には影響しない。
	if (not root.hasElement(U"exhibition")) return;
	const s3d::JSON exhibition = root[U"exhibition"];
	if (not exhibition.isObject()
		|| not exhibition.hasElement(U"enabled")
		|| not exhibition[U"enabled"].isBool()
		|| not exhibition.hasElement(U"endTime")
		|| not exhibition[U"endTime"].isString())
	{
		s3d::Print << U"exhibition の設定が不正です: " << path;
		return;
	}

	if (not exhibition[U"enabled"].get<bool>()) return;

	int parsed = 0;
	if (not ParseTime(exhibition[U"endTime"].getString(), parsed))
	{
		s3d::Print << U"exhibition.endTime は HH:MM 形式で指定してください: " << path;
		return;
	}

	exhibitionEndMinuteOfDay_ = parsed;
	exhibitionEnabled_ = true;
}

bool AppSettings::IsExhibitionEnded() const
{
	if (not exhibitionEnabled_) return false;

	const std::time_t now = std::time(nullptr);
	std::tm local{};
#if SIV3D_PLATFORM(WINDOWS)
	if (localtime_s(&local, &now) != 0) return false;
#else
	if (localtime_r(&now, &local) == nullptr) return false;
#endif

	const int current = local.tm_hour * 60 + local.tm_min;
	return current >= exhibitionEndMinuteOfDay_;
}
