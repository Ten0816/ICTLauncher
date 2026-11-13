# ICTLauncher — 機能分割・保守性重視版

対象: **Windows / OpenSiv3D 0.6.16 / Visual Studio**。ユーザーから提供された既存の `Main.cpp` を基に、処理を責務ごとに分離しています。

## Visual Studio への導入

1. 元の Visual Studio プロジェクトをバックアップしてください。
2. プロジェクトに登録されている **旧 Main.cpp を置き換え**、このフォルダの新 `Main.cpp` を追加します。**旧 Main.cpp を同時にコンパイルしないでください。**
3. 同階層に `.hpp` と `.cpp` を配置し、**全 `.cpp` をプロジェクトの「ソースファイル」に追加**します（フォルダへコピーするだけではビルド対象にならない場合があります）。
4. `#include` に使う `.hpp` は同じフォルダへ置いてください。
5. `source/logo.png` と `games/<ゲームフォルダ>/launcher_info.json` の配置は以前のままです。これらは exe のあるフォルダを基準に探索します。
6. リンクに必要な `Shlwapi.lib` は `Config.cpp` 内の `#pragma comment(lib, ...)` で指定しています。

## ファイルの役割

| ファイル | 担当範囲 |
|---|---|
| `Main.cpp` | Siv3D のエントリーポイントのみ |
| `LauncherApp.hpp/.cpp` | シーン遷移、確認ダイアログ、全体の制御 |
| `Game.hpp/.cpp` | ゲームのデータ構造、`launcher_info.json` のロード |
| `GameRunner.hpp/.cpp` | EXE/ブラウザの起動、終了検出 |
| `Config.hpp/.cpp` | Windowsブラウザ検出・起動・ウィンドウ監視 |
| `GameSelectScreen.hpp/.cpp` | ゲーム選択画面・説明パネル・ロゴ帯 |
| `VisualGuide.hpp/.cpp` | 起動前のキーボード・マウス・ゲームパッド案内 |
| `KeyGuide.hpp/.cpp` | JSON中のキー識別子から表示文字列への変換 |
| `SurveyForm.hpp/.cpp` | アンケート画面の描画、入力状態、送信処理 |
| `Survey.hpp/.cpp` | 回答JSONの個別保存 |
| `UI.hpp/.cpp` | 共通フォント、色、座標、ダイアログ、ボタン |

## 変更箇所の目安

- ゲームカードの表示 → `GameSelectScreen.cpp`
- ゲームタイトル/説明/スタッフ表示 → `GameSelectScreen.cpp`
- `launcher_info.json` の項目追加 → `Game.hpp` + `Game.cpp`
- ゲームの起動・終了判定 → `GameRunner.cpp`（ブラウザ固有部分は `Config.cpp`）
- キー案内の外観 → `VisualGuide.cpp`
- アンケート設問・入力欄 → `SurveyForm.cpp`
- JSON保存項目・ファイル名 → `Survey.cpp`
- 画面遷移 → `LauncherApp.cpp`
- UI座標や共通配色 → `UI.hpp`

## 設計ルール

- `.hpp` は基本的にデータ宣言と関数宣言、`.cpp` は処理内容。旧版の全実装をヘッダーへ書く方式と異なり、依存関係が追いやすくなります。
- 状態は可能な限りクラス内へ閉じ込めています（アンケート入力状態、選択画面のスクロール、実行中のプロセスなど）。
- `GameSelectScreen` や `SurveyForm` は **操作の結果を戻り値で返し**、画面遷移は `LauncherApp` が一括管理します。
- **既存のJSONのキー名・保存先・保存形式・操作案内の仕様**を維持しています。
- `Main.cpp` 以外の `.cpp` ファイルも Visual Studio に登録する必要があります。

## 手動確認チェックリスト

- [ ] リリースビルドが通る
- [ ] `games` を読み込める・優先度順に並ぶ
- [ ] 画像・説明・ロゴスクロールが表示される
- [ ] 左右キー・マウスで選択できる
- [ ] 各入力デバイスの案内を確認した後でゲームを起動できる
- [ ] EXEゲームとWebゲームの終了後に任意の感想案内が出る
- [ ] アンケートの送信・キャンセル・任意項目が動作する
- [ ] `games/<ゲーム>/感想文/` にゲーム名入りJSONが保存される
- [ ] Esc 単独ではランチャー終了しない / Shift+Esc でランチャー終了する

**動作確認の制約:** この環境には Visual Studio と OpenSiv3D 0.6.16 がないため、Windowsでのコンパイル・実機テストは未実施です。ソースの分割は機械的チェックを通していますが、最終的に手動確認してください。
