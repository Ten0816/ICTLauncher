#include "..\include\Game.hpp"
#include "..\include\Config.hpp"
#include "..\include\KeyGuide.hpp"
#include "..\include\LauncherLog.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <exception>
#include <system_error>
#include <cstring>

namespace
{
	// LoadGames()の処理中だけ、既存のログ出力を診断結果に反映する。
	thread_local GameLoadReport* activeLoadReport = nullptr;

	class GameLoadReportScope
	{
	public:
		explicit GameLoadReportScope(GameLoadReport* report)
			: previous_(activeLoadReport)
		{
			activeLoadReport = report;
		}

		~GameLoadReportScope()
		{
			activeLoadReport = previous_;
		}

	private:
		GameLoadReport* previous_;
	};

	// ========================================================
	// ログ出力
	// ========================================================

	void WriteConfigLog(
		const char* event,
		const FilePath& jsonPath,
		const String& reason)
	{
		const String message =
			U"file=" + jsonPath
			+ U" reason=" + reason;

		LauncherLog::Write(event, message);

		// 開発時の確認用
		Print << message;

		if (activeLoadReport != nullptr)
		{
			GameLoadIssue issue;
			issue.path = jsonPath;
			issue.reason = reason;
			issue.warning = (std::strcmp(event, "GAME_CONFIG_WARNING") == 0);
			activeLoadReport->issues.push_back(std::move(issue));
		}
	}

	// ========================================================
	// 必須文字列の読み込み
	// ========================================================

	bool ReadRequiredString(
		const JSON& json,
		const String& key,
		String& result)
	{
		if (not json.hasElement(key))
		{
			return false;
		}

		const JSON value = json[key];

		if (not value.isString())
		{
			return false;
		}

		result = value.getString();

		// 空白のみの場合も無効とする
		if (result.trimmed().isEmpty())
		{
			return false;
		}

		return true;
	}

	// ========================================================
	// 任意項目の安全な読み込み
	// ========================================================

	String ReadOptionalString(
		const JSON& json,
		const String& key,
		const FilePath& jsonPath)
	{
		if (not json.hasElement(key))
		{
			return U"";
		}

		if (not json[key].isString())
		{
			WriteConfigLog(
				"GAME_CONFIG_WARNING",
				jsonPath,
				U"invalid_string_type:" + key
			);

			return U"";
		}

		return json[key].getString();
	}

	bool ReadOptionalBool(
		const JSON& json,
		const String& key,
		const FilePath& jsonPath)
	{
		if (not json.hasElement(key))
		{
			return false;
		}

		if (not json[key].isBool())
		{
			WriteConfigLog(
				"GAME_CONFIG_WARNING",
				jsonPath,
				U"invalid_bool_type:" + key
			);

			return false;
		}

		return json[key].getOr<bool>(false);
	}

	int32 ReadPriority(
		const JSON& json,
		const FilePath& jsonPath)
	{
		if (not json.hasElement(U"priority"))
		{
			return 0;
		}

		const int32 invalidValue = -2147483647;

		const int32 value =
			json[U"priority"].getOr<int32>(
				invalidValue
			);

		if (value == invalidValue)
		{
			WriteConfigLog(
				"GAME_CONFIG_WARNING",
				jsonPath,
				U"invalid_priority"
			);

			return 0;
		}

		return value;
	}

	// ========================================================
	// gameId
	// ========================================================

	bool IsSafeGameId(const String& id)
	{
		if (id.isEmpty() || id.size() > 128)
		{
			return false;
		}

		for (const char32 ch : id)
		{
			if (
				not (
					(ch >= U'0' && ch <= U'9')
					|| (ch >= U'A' && ch <= U'Z')
					|| (ch >= U'a' && ch <= U'z')
					|| ch == U'_'
					|| ch == U'-'
					)
			)
			{
				return false;
			}
		}

		return true;
	}

	// gameIdが存在しない旧形式の互換処理
	String LegacyGameId(const FilePath& directory)
	{
		std::filesystem::path path{
			directory.toWstr()
		};

		if (path.filename().empty())
		{
			path = path.parent_path();
		}

		const String name =
			Unicode::FromWstring(
				path.filename().wstring()
			);

		if (IsSafeGameId(name))
		{
			return name;
		}

		const std::string bytes = name.toUTF8();

		constexpr char32 hex[] =
			U"0123456789abcdef";

		String result = U"legacy_";

		for (const unsigned char byte : bytes)
		{
			result += hex[byte >> 4];
			result += hex[byte & 15];
		}

		return result;
	}

	// ========================================================
	// schemaVersion
	// ========================================================

	void CheckSchemaVersion(
		const JSON& json,
		const FilePath& jsonPath)
	{
		if (not json.hasElement(U"schemaVersion"))
		{
			// 旧形式として扱う
			return;
		}

		const int32 version =
			json[U"schemaVersion"].getOr<int32>(-1);

		if (version != 1)
		{
			WriteConfigLog(
				"GAME_CONFIG_WARNING",
				jsonPath,
				U"unknown_schema_version="
				+ Format(version)
			);
		}
	}

	// ========================================================
	// ローカルファイルパスの検証
	//
	// ・ゲームフォルダ基準の相対パスのみ許可
	// ・絶対パスを拒否
	// ・フォルダ外参照を拒否
	// ・シンボリックリンク経由の脱出も確認
	// ・通常ファイルとして存在することを確認
	// ========================================================

	bool ResolveLocalFile(
		const FilePath& gameDirectory,
		const String& relativePath,
		FilePath& resolvedPath,
		String& reason)
	{
		if (relativePath.trimmed().isEmpty())
		{
			reason = U"empty_local_path";
			return false;
		}

		std::error_code ec;

		const std::filesystem::path base =
			std::filesystem::weakly_canonical(
				std::filesystem::path{
					gameDirectory.toWstr()
				},
				ec
			);

		if (ec)
		{
			reason = U"cannot_resolve_game_directory";
			return false;
		}

		const std::filesystem::path relative{
			relativePath.toWstr()
		};

		if (
			relative.is_absolute()
			|| relative.has_root_name()
			|| relative.has_root_directory()
		)
		{
			reason = U"absolute_path_not_allowed";
			return false;
		}

		const std::filesystem::path candidate =
			std::filesystem::weakly_canonical(
				base / relative,
				ec
			);

		if (ec)
		{
			reason = U"cannot_resolve_local_path";
			return false;
		}

		// candidateがbaseの配下にあるか確認する
		auto baseIt = base.begin();
		auto candidateIt = candidate.begin();

		for (
			;
			baseIt != base.end();
			++baseIt, ++candidateIt
		)
		{
			if (
				candidateIt == candidate.end()
				|| *baseIt != *candidateIt
			)
			{
				reason = U"path_outside_game_directory";
				return false;
			}
		}

		// ゲームフォルダ自身はファイルではない
		if (candidateIt == candidate.end())
		{
			reason = U"path_is_game_directory";
			return false;
		}

		ec.clear();

		if (
			not std::filesystem::is_regular_file(
				candidate,
				ec
			)
		)
		{
			reason = ec
				? U"file_access_error"
				: U"file_not_found";

			return false;
		}

		resolvedPath =
			Unicode::FromWstring(
				candidate.wstring()
			);

		return true;
	}


	// ========================================================
	// 操作キー・終了キーのJSON構造を検証
	// ========================================================

	bool ValidateKeySettings(
		const JSON& settings,
		const bool isExit,
		String& reason)
	{
		if (not settings.isObject())
		{
			reason = U"not_object";
			return false;
		}

		for (const String device :
		{ U"keyboard", U"mouse", U"gamepad" })
		{
			// デバイス自体の省略は許可
			if (not settings.hasElement(device))
			{
				continue;
			}

			const JSON entries = settings[device];

			if (not entries.isArray())
			{
				reason = U"device_not_array:" + device;
				return false;
			}

			for (const auto& entry : entries.arrayView())
			{
				// 通常操作キー
				if (not isExit)
				{
					if (not entry.isString())
					{
						reason = U"regular_key_not_string:"
							+ device;
						return false;
					}

					if (entry.getString().trimmed().isEmpty())
					{
						reason = U"empty_regular_key:"
							+ device;
						return false;
					}

					continue;
				}

				// 終了キーは組み合わせの配列
				if (not entry.isArray())
				{
					reason = U"exit_combo_not_array:"
						+ device;
					return false;
				}

				bool hasKey = false;

				for (const auto& key : entry.arrayView())
				{
					hasKey = true;

					if (not key.isString())
					{
						reason = U"exit_key_not_string:"
							+ device;
						return false;
					}

					if (key.getString().trimmed().isEmpty())
					{
						reason = U"empty_exit_key:"
							+ device;
						return false;
					}
				}

				if (not hasKey)
				{
					reason = U"empty_exit_combo:"
						+ device;
					return false;
				}
			}
		}

		return true;
	}



	// ========================================================
	// キー設定の読み込み
	// ========================================================

	void ReadKeySettings(
		const JSON& json,
		const FilePath& jsonPath,
		Game& game)
	{
		// ====================================================
		// 通常操作キー
		// ====================================================

		if (json.hasElement(U"keys"))
		{
			const JSON settings = json[U"keys"];
			String reason;

			if (ValidateKeySettings(settings, false, reason))
			{
				game.regularKeysJson = settings;

				game.regularKeyGuide =
					KeyGuide::ReadRegular(settings);
			}
			else
			{
				WriteConfigLog(
					"GAME_CONFIG_WARNING",
					jsonPath,
					U"invalid_keys:" + reason
				);
			}
		}

		// ====================================================
		// 終了キー
		// ====================================================

		if (json.hasElement(U"exitKeys"))
		{
			const JSON settings = json[U"exitKeys"];
			String reason;

			if (ValidateKeySettings(settings, true, reason))
			{
				game.exitKeysJson = settings;

				game.exitKeyGuide =
					KeyGuide::ReadExit(settings);
			}
			else
			{
				WriteConfigLog(
					"GAME_CONFIG_WARNING",
					jsonPath,
					U"invalid_exit_keys:" + reason
				);
			}
		}

		// ====================================================
		// 旧形式の終了方法
		// ====================================================

		game.legacyEnd = ReadOptionalString(
			json,
			U"end",
			jsonPath
		);
	}

}

// ============================================================
// ゲーム一覧の読み込み
// ============================================================

Array<Game> LoadGames(GameLoadReport* report)
{
	if (report)
	{
		*report = GameLoadReport{};
	}

	GameLoadReportScope reportScope{ report };
	Array<Game> games;

	// Windowsでは大文字・小文字の違いだけで
	// 同じフォルダ名になるため、IDも区別しない
	HashSet<String> loadedIds;

	const FilePath appDirectory =
		FileSystem::ParentPath(
			FileSystem::ModulePath()
		);

	const FilePath gamesDirectory =
		appDirectory + U"games/";

	if (not FileSystem::IsDirectory(gamesDirectory))
	{
		WriteConfigLog(
			"GAME_CONFIG_ERROR",
			gamesDirectory,
			U"games_directory_not_found"
		);

		return games;
	}

	for (
		const FilePath& gameDirectory :
		FileSystem::DirectoryContents(
			gamesDirectory,
			Recursive::No
		)
	)
	{
		if (not FileSystem::IsDirectory(gameDirectory))
		{
			continue;
		}

		const FilePath jsonPath =
			gameDirectory + U"launcher_info.json";

		if (not FileSystem::IsFile(jsonPath))
		{
			// 設定JSONがないフォルダはゲームではない
			continue;
		}

		if (report)
		{
			++report->scanned;
		}

		try
		{
			// =================================================
			// JSON読み込み
			// =================================================

			const JSON json = JSON::Load(jsonPath);

			if (not json || not json.isObject())
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"invalid_json"
				);

				continue;
			}

			// =================================================
			// 必須項目
			// =================================================

			String title;
			String path;
			String image;

			if (
				not ReadRequiredString(
					json, U"title", title
				)
			)
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"missing_or_invalid_title"
				);

				continue;
			}

			if (
				not ReadRequiredString(
					json, U"path", path
				)
			)
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"missing_or_invalid_path"
				);

				continue;
			}

			if (
				not ReadRequiredString(
					json, U"image", image
				)
			)
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"missing_or_invalid_image"
				);

				continue;
			}

			// =================================================
			// バージョン
			// =================================================

			CheckSchemaVersion(json, jsonPath);

			// =================================================
			// Game構築
			// =================================================

			Game game;

			game.directory = gameDirectory;
			game.title = title;

			// =================================================
			// gameId
			// =================================================


			if (json.hasElement(U"gameId"))
			{
				// gameIdが存在する場合は、正しい文字列であることを必須にする
				if (not json[U"gameId"].isString())
				{
					WriteConfigLog(
						"GAME_CONFIG_ERROR",
						jsonPath,
						U"invalid_game_id_type"
					);

					continue;
				}

				game.gameId = json[U"gameId"].getString();

				if (not IsSafeGameId(game.gameId))
				{
					WriteConfigLog(
						"GAME_CONFIG_ERROR",
						jsonPath,
						U"invalid_game_id"
					);

					continue;
				}
			}
			else
			{
				// gameId自体が存在しない旧形式だけ互換処理
				game.gameId = LegacyGameId(gameDirectory);

				WriteConfigLog(
					"GAME_CONFIG_WARNING",
					jsonPath,
					U"game_id_missing_using_legacy_id="
					+ game.gameId
				);
			}


			const String normalizedId =
				game.gameId.lowercased();

			if (loadedIds.contains(normalizedId))
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"duplicate_game_id="
					+ game.gameId
				);

				continue;
			}

			// =================================================
			// 実行先の検証
			// =================================================

			if (Config::IsURL(path))
			{
				game.path = path;
				game.isWebApp = true;
			}
			else
			{
				// 不正なURL形式をローカルパスとして
				// 誤って扱わないようにする
				if (path.includes(U"://"))
				{
					WriteConfigLog(
						"GAME_CONFIG_ERROR",
						jsonPath,
						U"unsupported_url_scheme"
					);

					continue;
				}

				FilePath resolvedPath;
				String reason;

				if (
					not ResolveLocalFile(
						gameDirectory,
						path,
						resolvedPath,
						reason
					)
				)
				{
					WriteConfigLog(
						"GAME_CONFIG_ERROR",
						jsonPath,
						U"invalid_game_path:" + reason
					);

					continue;
				}

				game.path = resolvedPath;
				game.isWebApp =
					Config::IsHTML(game.path);
			}

			// =================================================
			// 画像の検証
			// =================================================

			FilePath resolvedImage;
			String imageReason;

			if (
				not ResolveLocalFile(
					gameDirectory,
					image,
					resolvedImage,
					imageReason
				)
			)
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"invalid_image_path:"
					+ imageReason
				);

				continue;
			}

			game.texture = Texture{
				resolvedImage,
				TextureDesc::Mipped
			};

			if (not game.texture)
			{
				WriteConfigLog(
					"GAME_CONFIG_ERROR",
					jsonPath,
					U"image_decode_failed"
				);

				continue;
			}

			// =================================================
			// 任意の文字列
			// =================================================

			game.desc = ReadOptionalString(
				json, U"desc", jsonPath
			);

			game.staff = ReadOptionalString(
				json, U"staff", jsonPath
			);

			game.tools = ReadOptionalString(
				json, U"tools", jsonPath
			);

			// =================================================
			// デバイス情報
			// =================================================

			game.useMouse = ReadOptionalBool(
				json, U"mouse", jsonPath
			);

			game.useKeyboard = ReadOptionalBool(
				json, U"keyboard", jsonPath
			);

			game.useGamepad = ReadOptionalBool(
				json, U"gamepad", jsonPath
			);

			// =================================================
			// 優先順位
			// =================================================

			game.priority =
				ReadPriority(json, jsonPath);

			// =================================================
			// 操作キー・終了キー
			// =================================================

			ReadKeySettings(
				json,
				jsonPath,
				game
			);

			// =================================================
			// 読み込み成功
			// =================================================

			loadedIds.insert(normalizedId);

			LauncherLog::Write(
				"GAME_CONFIG_LOADED",
				U"gameId=" + game.gameId
				+ U" title=" + game.title
			);

			games << std::move(game);
		}
		catch (const std::exception& e)
		{
			WriteConfigLog(
				"GAME_CONFIG_ERROR",
				jsonPath,
				U"exception:"
				+ Unicode::FromUTF8(e.what())
			);
		}
		catch (...)
		{
			WriteConfigLog(
				"GAME_CONFIG_ERROR",
				jsonPath,
				U"unknown_exception"
			);
		}
	}

	// ========================================================
	// ゲームの表示順
	// ========================================================

	games.sort_by(
		[](const Game& a, const Game& b)
		{
			return a.priority > b.priority;
		}
	);

	LauncherLog::Write(
		"GAME_CONFIG_SUMMARY",
		U"loaded=" + Format(games.size())
	);

	if (report)
	{
		report->loaded = games.size();
	}

	return games;
}
