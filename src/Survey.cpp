#include "..\include\Survey.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <string>
#include <exception>



// ============================================================
// アンケート回答の保存
// ============================================================
namespace Survey
{


	// JSON文字列をUTF-8で安全にエスケープする
	std::string EscapeJSON(const String& value)
	{
		const std::string utf8 = value.toUTF8();
		std::string result = "\"";
		constexpr char hex[] = "0123456789abcdef";
		for (const unsigned char c : utf8)
		{
			switch (c)
			{
			case '"': result += "\\\""; break;
			case '\\': result += "\\\\"; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '	': result += "\\t"; break;
			default:
				if (c < 0x20)
				{
					result += "\\u00";
					result += hex[c >> 4];
					result += hex[c & 0x0F];
				}
				else result += static_cast<char>(c);
				break;
			}
		}
		result += '"';
		return result;
	}

	bool Save(const Game& game, int32 fun, int32 usability, int32 difficulty,
		const String& good, const String& improvement, int32 age,
		const String& deviceId)
	{
		try
		{
			const std::filesystem::path folder =
				std::filesystem::path(FileSystem::ParentPath(FileSystem::ModulePath()).toWstr())
				/ L"感想文" / std::filesystem::path(game.gameId.toWstr());
			std::filesystem::create_directories(folder);

			const std::time_t now = std::time(nullptr);
			std::tm local{};
# if SIV3D_PLATFORM(WINDOWS)
			if (localtime_s(&local, &now) != 0) return false;
# else
			if (!localtime_r(&now, &local)) return false;
# endif
			char filenameTime[32]{};
			char displayTime[32]{};
			if (!std::strftime(filenameTime, sizeof(filenameTime), "%Y-%m-%d_%H-%M-%S", &local)) return false;
			if (!std::strftime(displayTime, sizeof(displayTime), "%Y-%m-%d %H:%M:%S", &local)) return false;

			// 同じ秒に複数の回答があっても既存ファイルを上書きしない
			std::filesystem::path file = folder / (std::string(filenameTime) + ".json");
			for (int32 suffix = 2; std::filesystem::exists(file); ++suffix)
			{
				file = folder / (std::string(filenameTime) + "_" + std::to_string(suffix) + ".json");
			}

			const String difficultyText = (difficulty == 1) ? U"簡単"
				: (difficulty == 2) ? U"ちょうどよい" : U"難しい";
			const Array<String> ageLabels{ U"10代", U"20代", U"30代", U"40代", U"50代", U"60代以上" };

			std::ofstream stream(file, std::ios::binary | std::ios::out | std::ios::trunc);
			if (!stream) return false;
			stream << "{\n"
				<< "  " << EscapeJSON(U"ゲームID") << ": " << EscapeJSON(game.gameId) << ",\n"
				<< "  " << EscapeJSON(U"ゲーム名") << ": " << EscapeJSON(game.title) << ",\n"
				<< "  " << EscapeJSON(U"PC識別子") << ": " << EscapeJSON(deviceId) << ",\n"
				<< "  " << EscapeJSON(U"回答日時") << ": " << EscapeJSON(Unicode::FromUTF8(displayTime)) << ",\n"
				<< "  " << EscapeJSON(U"楽しさ") << ": " << fun << ",\n"
				<< "  " << EscapeJSON(U"操作性") << ": " << usability << ",\n"
				<< "  " << EscapeJSON(U"難易度") << ": " << EscapeJSON(difficultyText) << ",\n"
				<< "  " << EscapeJSON(U"良かったところ") << ": " << EscapeJSON(good) << ",\n"
				<< "  " << EscapeJSON(U"改善してほしいところ") << ": " << EscapeJSON(improvement) << ",\n"
				<< "  " << EscapeJSON(U"年代") << ": ";
			if (age >= 1 && age <= 6) stream << EscapeJSON(ageLabels[age - 1]);
			else stream << "null";
			stream << "\n"
				<< "}\n";
			stream.flush();
			const bool ok = stream.good();
			stream.close();
			if (!ok) std::filesystem::remove(file);
			return ok;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

}
