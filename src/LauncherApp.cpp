#include "..\include\LauncherApp.hpp"
#include "..\include\LauncherLog.hpp"
#include "..\include\AppSettings.hpp"
#include "..\include\Game.hpp"
#include "..\include\GameRunner.hpp"
#include "..\include\GameSelectScreen\GameSelectScreen.hpp"
#include "..\include\SurveyForm.hpp"
#include "..\include\UI.hpp"
#include "..\include\VisualGuide.hpp"
#include <algorithm>
#include <utility>

namespace
{
	enum class Screen
	{
		GameSelect,
		LaunchGuide,
		PostGameSurveyPrompt,
		SurveyForm,
		SurveyCompleted,
		ExhibitionEnded,
		Diagnostics
	};

	enum class ConfirmDialog
	{
		None,
		OpenSurvey,
		CancelSurvey
	};

	class App
	{
	public:
		void Run()
		{
			Window::SetTitle(U"ICT Launcher");

			// 単独Escは戻る操作に使用する。
			// Launcherの終了はShift + Esc。
			// ウィンドウを閉じる操作で自動終了させない。
			// 終了してよいかは毎フレーム Launcher 側で判断する。
			System::SetTerminationTriggers(
				UserAction::NoAction
			);

			Scene::Resize(UI::RenderSize);
			Scene::SetResizeMode(ResizeMode::Keep);

			Window::SetFullscreen(true);

			Scene::SetBackground(UI::BackgroundColor);

			UI::RegisterFonts();

			TextureAsset::Register(
				U"Icon.Play",
				0xf144_icon,
				48
			);

			// ====================================================
			// 初期化
			// ====================================================

			const FilePath moduleDir =
				FileSystem::ParentPath(
					FileSystem::ModulePath()
				);

			logo_ = Texture{
				Resource(U"source/logo.png")
			};

			settings_.Load(
				moduleDir + U"launcher_settings.json"
			);

			LauncherLog::Initialize(
				moduleDir,
				settings_.DeviceId()
			);

			LauncherLog::Write(
				"LAUNCHER_START",
				U"ICTLauncher started"
			);

			lastActivityAt_ = Scene::Time();
			lastCursorPos_ = Cursor::Pos();

			games_ = LoadGames(&gameLoadReport_);

			if (games_.isEmpty())
			{
				LauncherLog::Write(
					"LAUNCHER_ERROR",
					U"no_games_found"
				);

				String message =
					U"読み込めるゲームがありません。\n確認した設定ファイル: "
					+ Format(gameLoadReport_.scanned);

				if (not gameLoadReport_.issues.isEmpty())
				{
					const auto& issue = gameLoadReport_.issues.front();
					message += U"\n\n最初の診断結果: " + issue.reason
						+ U"\n" + issue.path;
				}

				message += U"\n\n詳細: logs/launcher.log";
				System::MessageBoxOK(message);

				LauncherLog::Write(
					"LAUNCHER_EXIT",
					U"no_games_found"
				);

				return;
			}

			// ====================================================
			// メインループ
			// ====================================================

			bool shiftEscExit = false;
			bool closeButtonExit = false;

			while (System::Update())
			{
				// =================================================
				// ウィンドウの「閉じる」操作の処理
				//
				// UserAction::NoAction を指定してあるため、
				// ゲーム実行中は Launcher のみが終了するのを防ぐ。
				// =================================================
				if ((System::GetUserActions()
					& UserAction::CloseButtonClicked) != 0)
				{
					if (runner_.IsActive())
					{
						LauncherLog::Write(
							"LAUNCHER_CLOSE_BLOCKED",
							U"reason=game_running"
						);
					}
					else
					{
						closeButtonExit = true;
						break;
					}
				}

				// =================================================
				// Launcher終了ショートカット
				// Shift + Esc
				// =================================================

				// ゲーム実行中はLauncherだけを終了させない。
				if (
					not runner_.IsActive()
					&& KeyEscape.down()
					&& (
						KeyLShift.pressed()
						|| KeyRShift.pressed()
						)
				)
				{
					shiftEscExit = true;
					break;
				}

				// =================================================
				// 展示終了時刻の確認
				// =================================================

				// 終了時刻を過ぎても、
				// 実行中のゲームは強制終了しない。
				if (
					not exhibitionEnded_
					&& settings_.IsExhibitionEnded()
				)
				{
					exhibitionEnded_ = true;
				}

				// =================================================
				// 無操作時間の記録
				// =================================================

				TrackActivity();

				// 展示終了画面はEscで解除できる。
				if (
					screen_ == Screen::ExhibitionEnded
					&& KeyEscape.down()
				)
				{
					exhibitionDismissed_ = true;
					screen_ = Screen::GameSelect;
					lastActivityAt_ = Scene::Time();

					continue;
				}

				// =================================================
				// 無操作時のゲーム選択画面への復帰
				// =================================================

				// ゲーム実行中やアンケート関連の画面、
				// 確認ダイアログ表示中は対象外。
				if (
					dialog_ == ConfirmDialog::None
					&& not runner_.IsActive()
					&& (
						screen_ == Screen::GameSelect
						|| screen_ == Screen::LaunchGuide
						)
					&& Scene::Time() - lastActivityAt_ >= IdleTimeoutSeconds
					)
				{
					// 選択中のゲームは変更しない。
					// 詳細説明を閉じ、縦スクロール位置をリセットする。
					selector_.CollapseDescription();

					// 操作案内を閉じる。
					screen_ = Screen::GameSelect;
					guideVisitedDevices_ = 0;

					lastActivityAt_ = Scene::Time();
				}

				// =================================================
				// UI座標変換
				// =================================================

				// 描画座標とマウス座標を同じ倍率にする。
				const Transformer2D uiTransform{
					Mat3x2::Scale(UI::RenderScale),
					TransformCursor::Yes
				};

				// =================================================
				// 管理者用ゲーム復旧ショートカット
				// Ctrl + Shift + F11
				// =================================================

				CheckRecoveryHotkey();
				CheckDiagnosticsHotkey();
				CheckReloadHotkey();

				// =================================================
				// ゲーム実行状態の監視
				// =================================================

				// プロセスの終了確認は、
				// 通常の画面遷移より優先する。
				if (runner_.IsActive())
				{
					lastActivityAt_ = Scene::Time();

					const GameRunStatus status =
						runner_.Update();

					if (status == GameRunStatus::Running)
					{
#if SIV3D_PLATFORM(WINDOWS)
						// ゲームが前面化された際、GameRunner はLauncherを最小化する。
						// それを一度確認してから、Launcherが再び前面に来た場合のみ
						// 最小化してゲーム画面に戻る。起動直後の検出を妨げない。
						const HWND launcherHwnd = static_cast<HWND>(
							Platform::Windows::Window::GetHWND()
						);

						if (launcherHwnd && ::IsWindow(launcherHwnd))
						{
							if (::IsIconic(launcherHwnd))
							{
								launcherMinimizedForGame_ = true;
							}
							else if (
								launcherMinimizedForGame_
								&& ::GetForegroundWindow() == launcherHwnd
							)
							{
								Window::Minimize();
							}
						}
#endif
						continue;
					}

#if SIV3D_PLATFORM(WINDOWS)
					launcherMinimizedForGame_ = false;
#endif

					if (
						status == GameRunStatus::LaunchFailed
						|| status == GameRunStatus::Crashed
						|| status == GameRunStatus::ForceRecovered
					)
					{
						// 起動失敗・管理者による復旧時は
						// アンケートを表示しない。
						screen_ = Screen::GameSelect;
						dialog_ = ConfirmDialog::None;

						continue;
					}

					if (status == GameRunStatus::Finished)
					{
						// 正常なゲーム終了後は、
						// アンケートへの回答を案内する。
						//
						// 展示終了時刻を過ぎていても、
						// 回答やスキップの機会を確保する。
						screen_ = Screen::PostGameSurveyPrompt;

						continue;
					}
				}

				// =================================================
				// 展示終了画面への遷移
				// =================================================

				// アンケート関連の処理が進行中なら、
				// 終了画面には切り替えない。
				if (
					exhibitionEnded_
					&& not exhibitionDismissed_
					&& not IsSurveyFlowActive()
				)
				{
					screen_ = Screen::ExhibitionEnded;
					dialog_ = ConfirmDialog::None;

					DrawExhibitionEnded();

					continue;
				}

				// =================================================
				// 確認ダイアログ
				// =================================================

				if (dialog_ != ConfirmDialog::None)
				{
					HandleConfirmation();

					// 背後の画面を操作させない。
					continue;
				}

				// =================================================
				// 各画面の更新・描画
				// =================================================

				switch (screen_)
				{
				case Screen::GameSelect:
					HandleGameSelect();
					break;

				case Screen::LaunchGuide:
					HandleLaunchGuide();
					break;

				case Screen::PostGameSurveyPrompt:
					HandlePostGameSurveyPrompt();
					break;

				case Screen::SurveyForm:
					HandleSurveyForm();
					break;

				case Screen::Diagnostics:
					DrawDiagnostics();
					break;

				case Screen::ExhibitionEnded:
					DrawExhibitionEnded();
					break;

				case Screen::SurveyCompleted:

					SurveyForm::DrawCompleted();

					if (
						Scene::Time() - completedAt_
						>= 3.0
					)
					{
						form_.Reset();

						screen_ = Screen::GameSelect;
						lastActivityAt_ = Scene::Time();
					}

					break;
				}
			}

			// ====================================================
			// Launcher終了ログ
			// ====================================================

			LauncherLog::Write(
				"LAUNCHER_EXIT",
				shiftEscExit
					? U"shift_escape"
					: (closeButtonExit
						? U"window_close"
						: U"update_ended")
			);
		}

	private:

		static constexpr double IdleTimeoutSeconds = 60.0;

		// 管理者用ショートカットの押下状態
#if SIV3D_PLATFORM(WINDOWS)
		// ゲーム起動後にLauncherが一度最小化されたか。
		bool launcherMinimizedForGame_ = false;
#endif
		bool recoveryHotkeyWasDown_ = false;

		// ========================================================
		// 操作検出
		// ========================================================


		void TrackActivity()
		{
			const Point pos = Cursor::Pos();

			bool active =
				(pos != lastCursorPos_)
				|| MouseL.down()
				|| MouseR.down()
				|| MouseM.down()
				|| (Mouse::Wheel() != 0.0)
				|| (Mouse::WheelH() != 0.0);

			lastCursorPos_ = pos;

			if (not active)
			{
				for (const auto& key : Keyboard::GetAllInputs())
				{
					if (key.down() || key.pressed())
					{
						active = true;
						break;
					}
				}
			}

			if (active)
			{
				lastActivityAt_ = Scene::Time();
			}
		}


		// ========================================================
		// 管理者用復旧ショートカット
		// ========================================================

		// 管理者専用。アンケート途中やゲーム実行中には開けない。
		// Ctrl + Shift + F10
		void CheckDiagnosticsHotkey()
		{
#if SIV3D_PLATFORM(WINDOWS)
			const bool ctrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			const bool shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			const bool f10 = (::GetAsyncKeyState(VK_F10) & 0x8000) != 0;
			const bool pressed = ctrl && shift && f10;

			if (pressed && not diagnosticsHotkeyWasDown_ && not runner_.IsActive())
			{
				if (screen_ == Screen::GameSelect && dialog_ == ConfirmDialog::None)
				{
					diagnosticsPage_ = 0;
					screen_ = Screen::Diagnostics;
				}
				else if (screen_ == Screen::Diagnostics)
				{
					screen_ = Screen::GameSelect;
				}
				lastActivityAt_ = Scene::Time();
			}
			diagnosticsHotkeyWasDown_ = pressed;
#endif
		}

		// 管理者専用: Ctrl + Shift + F9
		// ゲーム選択画面でだけ再読み込み可能にする。
		void CheckReloadHotkey()
		{
#if SIV3D_PLATFORM(WINDOWS)
			const bool ctrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			const bool shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			const bool f9 = (::GetAsyncKeyState(VK_F9) & 0x8000) != 0;
			const bool pressed = ctrl && shift && f9;

			if (pressed && not reloadHotkeyWasDown_
				&& not runner_.IsActive()
				&& screen_ == Screen::GameSelect
				&& dialog_ == ConfirmDialog::None)
			{
				ReloadGames();
			}

			reloadHotkeyWasDown_ = pressed;
#endif
		}

		void ReloadGames()
		{
			String selectedGameId;
			if (not games_.isEmpty())
			{
				const size_t selected = selector_.SelectedIndex();
				if (selected < games_.size())
				{
					selectedGameId = games_[selected].gameId;
				}
			}

			LauncherLog::Write(
				"GAME_RELOAD_START",
				U"previousCount=" + Format(games_.size())
			);

			GameLoadReport newReport;
			Array<Game> newGames = LoadGames(&newReport);

			// 全件の読み込みに失敗した場合は、既存の一覧を破棄しない。
			// ただし診断情報は最新の結果に更新する。
			if (newGames.isEmpty())
			{
				gameLoadReport_ = std::move(newReport);
				diagnosticsPage_ = 0;

				LauncherLog::Write(
					"GAME_RELOAD_FAILED",
					U"no_valid_games previousCount=" + Format(games_.size())
				);

				System::MessageBoxOK(
					U"再読み込みできるゲームがありませんでした。\n"
					U"現在のゲーム一覧を維持します。\n"
					U"Ctrl + Shift + F10 で診断結果を確認してください。"
				);
				lastActivityAt_ = Scene::Time();
				return;
			}

			size_t newSelectedIndex = 0;
			if (not selectedGameId.isEmpty())
			{
				for (size_t i = 0; i < newGames.size(); ++i)
				{
					if (newGames[i].gameId == selectedGameId)
					{
						newSelectedIndex = i;
						break;
					}
				}
			}

			const size_t previousCount = games_.size();
			games_ = std::move(newGames);
			gameLoadReport_ = std::move(newReport);

			selector_.Select(newSelectedIndex);
			selector_.CollapseDescription();
			diagnosticsPage_ = 0;
			launchGameIndex_ = newSelectedIndex;
			playedGameIndex_ = newSelectedIndex;
			surveyGameIndex_ = newSelectedIndex;
			guideVisitedDevices_ = 0;
			lastActivityAt_ = Scene::Time();

			LauncherLog::Write(
				"GAME_RELOAD_COMPLETE",
				U"previousCount=" + Format(previousCount)
				+ U" loaded=" + Format(games_.size())
				+ U" issues=" + Format(gameLoadReport_.issues.size())
			);

			String message =
				U"ゲーム一覧を再読み込みしました。\n"
				U"読み込み成功: " + Format(gameLoadReport_.loaded)
				+ U" 件";

			if (not gameLoadReport_.issues.isEmpty())
			{
				message += U"\nエラー・警告: ";
				message += Format(gameLoadReport_.issues.size());
				message += U" 件\nCtrl + Shift + F10 で詳細を確認できます。";
			}

			System::MessageBoxOK(message);
		}

		void DrawDiagnostics()
		{
			constexpr size_t ItemsPerPage = 7;
			RectF{ 0, 0, UI::WindowSize.x, UI::WindowSize.y }
			.draw(UI::BackgroundColor);

			const Font& title = FontAsset(U"Survey.Title");
			const Font& body = FontAsset(U"Survey.Body");
			const Font& smallFont = FontAsset(U"Survey.Small");

			title(U"ゲーム読み込み診断")
				.draw(30, Vec2{ 40, 24 }, UI::TextColor);

			body(U"読み込み成功: " + Format(gameLoadReport_.loaded)
				+ U" / " + Format(gameLoadReport_.scanned))
				.draw(20, Vec2{ 45, 90 }, UI::TextColor);

			size_t errors = 0;
			size_t warnings = 0;
			for (const auto& issue : gameLoadReport_.issues)
			{
				if (issue.warning) ++warnings;
				else ++errors;
			}

			body(U"エラー: " + Format(errors)
				+ U"    警告: " + Format(warnings))
				.draw(18, Vec2{ 45, 130 }, UI::TextColor);

			const size_t count = gameLoadReport_.issues.size();
			const size_t pages = std::max<size_t>(
				1, (count + ItemsPerPage - 1) / ItemsPerPage);
			diagnosticsPage_ = std::min(diagnosticsPage_, pages - 1);

			if (count == 0)
			{
				body(U"読み込み時のエラー・警告はありません。")
					.draw(20, Vec2{ 45, 210 }, UI::TextColor);
			}

			const size_t start = diagnosticsPage_ * ItemsPerPage;
			const size_t end = std::min(start + ItemsPerPage, count);
			for (size_t i = start; i < end; ++i)
			{
				const auto& issue = gameLoadReport_.issues[i];
				const double y = 185 + (i - start) * 52.0;
				RectF{ 40, y, 1200, 47 }.rounded(5).draw(ColorF{ 1.0 });

				const ColorF color = issue.warning
					? ColorF{ 0.60, 0.39, 0.08 }
				: ColorF{ 0.80, 0.18, 0.18 };
				const String category = issue.warning ? U"警告" : U"エラー";

				// 一時的な文字列ビューや文字単位の暗黙変換を使わず、
				// 表示用文字列を明示的に構築する。
				String message = issue.reason;
				if (message.size() > 115)
				{
					message = String{ message.substr(0, 112) };
					message += U"...";
				}

				String summary = U"[";
				summary += category;
				summary += U"] ";
				summary += message;
				smallFont(summary).draw(15, Vec2{ 52, y + 3 }, color);

				String pathText = issue.path;
				if (pathText.size() > 115)
				{
					const String tail = pathText.substr(pathText.size() - 112);
					pathText = U"...";
					pathText += tail;
				}
				smallFont(pathText).draw(12, Vec2{ 52, y + 26 }, UI::TextColor);
			}

			const RectF prev{ 390, 570, 130, 45 };
			const RectF next{ 540, 570, 130, 45 };
			const RectF close{ 1030, 570, 210, 45 };

			if (UI::DrawChoiceButton(prev, U"前へ", false)
				&& diagnosticsPage_ > 0)
			{
				--diagnosticsPage_;
			}
			if (UI::DrawChoiceButton(next, U"次へ", false)
				&& diagnosticsPage_ + 1 < pages)
			{
				++diagnosticsPage_;
			}
			body(Format(diagnosticsPage_ + 1) + U" / " + Format(pages))
				.draw(17, Vec2{ 700, 580 }, UI::TextColor);

			if (UI::DrawChoiceButton(close, U"閉じる", true)
				|| KeyEscape.down())
			{
				screen_ = Screen::GameSelect;
				lastActivityAt_ = Scene::Time();
			}
		}

		void CheckRecoveryHotkey()
		{
#if SIV3D_PLATFORM(WINDOWS)

			// Ctrl + Shift + F11

			const bool ctrl =
				(::GetAsyncKeyState(VK_CONTROL) & 0x8000)
				!= 0;

			const bool shift =
				(::GetAsyncKeyState(VK_SHIFT) & 0x8000)
				!= 0;

			const bool f11 =
				(::GetAsyncKeyState(VK_F11) & 0x8000)
				!= 0;

			const bool hotkeyDown =
				ctrl && shift && f11;

			// 押した瞬間に一度だけ終了を要求する。
			if (
				hotkeyDown
				&& not recoveryHotkeyWasDown_
				&& runner_.IsActive()
			)
			{
				runner_.RequestForceRecovery();
			}

			recoveryHotkeyWasDown_ = hotkeyDown;

#endif
		}

		// ========================================================
		// アンケート中かどうか
		// ========================================================

		// 展示終了時刻を過ぎても、
		// アンケートの回答や確認を妨げない。
		[[nodiscard]]
		bool IsSurveyFlowActive() const
		{
			return (
				screen_ == Screen::PostGameSurveyPrompt
				|| screen_ == Screen::SurveyForm
				|| screen_ == Screen::SurveyCompleted
				|| dialog_ == ConfirmDialog::OpenSurvey
				|| dialog_ == ConfirmDialog::CancelSurvey
			);
		}

		// ========================================================
		// 展示終了画面
		// ========================================================

		void DrawExhibitionEnded() const
		{
			RectF{
				0,
				0,
				UI::WindowSize.x,
				UI::WindowSize.y
			}.draw(UI::BackgroundColor);

			const Vec2 center{
				UI::WindowSize.x / 2.0,
				UI::WindowSize.y / 2.0
			};

			const Font& font =
				FontAsset(U"Survey.Title");

			font(
				U"展示は終了しました。"
			).drawAt(
				36,
				center.movedBy(0, -35),
				UI::TextColor
			);

			font(
				U"遊んでいただきありがとうございました"
			).drawAt(
				28,
				center.movedBy(0, 35),
				UI::TextColor
			);
		}

		// ========================================================
		// ゲーム選択画面
		// ========================================================

		void HandleGameSelect()
		{
			const GameSelectAction action =
				selector_.UpdateAndDraw(
					games_,
					logo_
				);

			if (
				action
				== GameSelectAction::PlayRequested
			)
			{
				launchGameIndex_ =
					selector_.SelectedIndex();

				guideDevice_ = 0;
				guideVisitedDevices_ = 0;

				screen_ = Screen::LaunchGuide;
			}
			else if (
				action
				== GameSelectAction::SurveyRequested
			)
			{
				dialog_ = ConfirmDialog::OpenSurvey;
			}
		}

		// ========================================================
		// 操作案内画面
		// ========================================================

		void HandleLaunchGuide()
		{
			const Game& game =
				games_[launchGameIndex_];

			const UI::ConfirmationResult result =
				VisualGuide::Draw(
					game.title,
					game.regularKeysJson,
					game.exitKeysJson,
					game.legacyEnd,
					guideDevice_,
					guideVisitedDevices_
				);

			// ゲーム選択画面に戻る。
			if (
				result == UI::ConfirmationResult::Back
			)
			{
				screen_ = Screen::GameSelect;
				lastActivityAt_ = Scene::Time();

				return;
			}

			if (
				result != UI::ConfirmationResult::Accept
			)
			{
				return;
			}

			// ====================================================
			// ゲーム起動
			// ====================================================

			screen_ = Screen::GameSelect;

#if SIV3D_PLATFORM(WINDOWS)
			launcherMinimizedForGame_ = false;
#endif
			const bool started =
				runner_.Start(game);

			if (not started)
			{
				LauncherLog::Write(
					"GAME_LAUNCH_FAILED",
					U"gameId=" + game.gameId
					+ U" title=" + game.title
				);

				lastActivityAt_ = Scene::Time();

				return;
			}

			// 起動成功時のみ、
			// アンケート対象のゲームを記録する。
			playedGameIndex_ = launchGameIndex_;

			LauncherLog::Write(
				"GAME_LAUNCH_ACCEPTED",
				U"gameId=" + game.gameId
				+ U" title=" + game.title
			);

			lastActivityAt_ = Scene::Time();
		}

		// ========================================================
		// ゲーム終了後のアンケート案内
		// ========================================================

		void HandlePostGameSurveyPrompt()
		{
			const UI::ConfirmationResult result =
				UI::DrawPostGameSurveyPrompt(
					games_[playedGameIndex_].title
				);

			if (
				result == UI::ConfirmationResult::Back
			)
			{
				screen_ = Screen::GameSelect;
				lastActivityAt_ = Scene::Time();
			}
			else if (
				result == UI::ConfirmationResult::Accept
			)
			{
				OpenSurvey(playedGameIndex_);
			}
		}

		// ========================================================
		// アンケート入力画面
		// ========================================================

		void HandleSurveyForm()
		{
			const SurveyFormAction action =
				form_.Draw(
					games_[surveyGameIndex_],
					settings_.DeviceId()
				);

			if (
				action
				== SurveyFormAction::CancelRequested
			)
			{
				dialog_ = ConfirmDialog::CancelSurvey;
			}
			else if (
				action
				== SurveyFormAction::Submitted
			)
			{
				completedAt_ = Scene::Time();
				screen_ = Screen::SurveyCompleted;
			}
		}

		// ========================================================
		// 確認ダイアログ
		// ========================================================

		void HandleConfirmation()
		{
			const bool opening =
				(dialog_ == ConfirmDialog::OpenSurvey);

			const UI::ConfirmationResult result =
				UI::DrawConfirmation(opening);

			if (
				result == UI::ConfirmationResult::Back
			)
			{
				dialog_ = ConfirmDialog::None;
			}
			else if (
				result == UI::ConfirmationResult::Accept
			)
			{
				if (opening)
				{
					OpenSurvey(
						selector_.SelectedIndex()
					);
				}
				else
				{
					form_.Reset();

					screen_ = Screen::GameSelect;
					lastActivityAt_ = Scene::Time();
				}

				dialog_ = ConfirmDialog::None;
			}
		}

		// ========================================================
		// アンケートを開く
		// ========================================================

		void OpenSurvey(size_t index)
		{
			surveyGameIndex_ = index;

			selector_.Select(index);
			selector_.CollapseDescription();

			form_.Reset();

			screen_ = Screen::SurveyForm;
		}

		// ========================================================
		// メンバー変数
		// ========================================================

		GameLoadReport gameLoadReport_;
		bool diagnosticsHotkeyWasDown_ = false;
		bool reloadHotkeyWasDown_ = false;
		size_t diagnosticsPage_ = 0;

		AppSettings settings_;

		bool exhibitionEnded_ = false;
		bool exhibitionDismissed_ = false;

		double lastActivityAt_ = 0.0;

		Point lastCursorPos_{ 0, 0 };

		Array<Game> games_;

		Texture logo_;

		GameSelectScreen selector_;
		GameRunner runner_;
		SurveyForm form_;

		Screen screen_ = Screen::GameSelect;
		ConfirmDialog dialog_ = ConfirmDialog::None;

		size_t launchGameIndex_ = 0;
		size_t playedGameIndex_ = 0;
		size_t surveyGameIndex_ = 0;

		int32 guideDevice_ = 0;
		uint8 guideVisitedDevices_ = 0;

		double completedAt_ = 0.0;
	};
}

// ============================================================
// LauncherApp
// ============================================================

void LauncherApp::Run()
{
	App{}.Run();
}
