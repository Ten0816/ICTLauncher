#include "..\include\LauncherApp.hpp"
#include "..\include\LauncherLog.hpp"
#include "..\include\AppSettings.hpp"
#include "..\include\Game.hpp"
#include "..\include\GameRunner.hpp"
#include "..\include\GameSelectScreen\GameSelectScreen.hpp"
#include "..\include\SurveyForm.hpp"
#include "..\include\UI.hpp"
#include "..\include\VisualGuide.hpp"

namespace
{
	enum class Screen
	{
		GameSelect,
		LaunchGuide,
		PostGameSurveyPrompt,
		SurveyForm,
		SurveyCompleted,
		ExhibitionEnded
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
			System::SetTerminationTriggers(
				UserAction::CloseButtonClicked
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

			games_ = LoadGames();

			if (games_.isEmpty())
			{
				LauncherLog::Write(
					"LAUNCHER_ERROR",
					U"no_games_found"
				);

				System::MessageBoxOK(
					U"ゲームがありません。"
				);

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

			while (System::Update())
			{
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
						continue;
					}

					if (
						status == GameRunStatus::LaunchFailed
						|| status == GameRunStatus::Crashed
						|| status == GameRunStatus::ForceRecovered
					)
					{
						// 起動失敗・クラッシュ・管理者復旧ではアンケートを出さない。
						screen_ = Screen::GameSelect;
						dialog_ = ConfirmDialog::None;
						lastActivityAt_ = Scene::Time();
						continue;
					}

					if (status == GameRunStatus::Finished)
					{
						// 通常終了後はアンケートを案内する。
						screen_ = Screen::PostGameSurveyPrompt;
						lastActivityAt_ = Scene::Time();
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
					: U"window_closed_or_update_ended"
			);
		}

	private:

		static constexpr double IdleTimeoutSeconds = 60.0;

		// 管理者用ショートカットの押下状態
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
