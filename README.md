# cli-base

組み込みLinux上で動く、純C（C11）の対話型デバッグCLIのベース環境です。
各プロジェクトではコマンド表・ハンドラ・対象アドレス範囲を変更します。
外部のTUIライブラリは不要です。

## ビルドと起動

```sh
make
./build/mock/debug_cli
make real                 # build/real/debug_cli を生成
make test                 # Python 3。模擬ファイルだけで動作確認
```

通常ビルドはファイルをメモリとして使います。実機ビルドだけが `/dev/mem` を使います。
両者のオブジェクトは別ディレクトリに置くため、切り替え時に混ざりません。
クロスコンパイルは `make CC=aarch64-linux-gnu-gcc` のように指定できます。
コンパイラやフラグを変更するときは `make clean` してからビルドしてください。
`make clean` は `build/` のみ削除し、模擬メモリの内容を残します。

## 構成と役割

| 場所 | 役割 |
|---|---|
| `src/main.c` | メインメニューのコマンド登録・起動 |
| `src/commands/` | 機能ごとのコマンド表、ハンドラ、専用ヘッダ |
| `src/core/cli.c` | 入力ループ・振り分け・メニュー移動・help |
| `src/core/register_io.c` | レジスタアクセス・ビットフィールド操作 |
| `src/core/parse.c` | 数値引数の検証・変換 |
| `include/` | 共通APIのヘッダ |
| `include/app_config.h` | プロジェクトごとのアドレス範囲・既定ファイル名 |
| `tests/` | 模擬環境での動作確認 |
| `build/mock/`, `build/real/` | 実行ファイル・オブジェクト・依存情報（Git管理外） |

## コマンド例

```text
cli> write 0x40000000 0xaabbccdd
cli> reg field 0x40000000 0x0000ff00 8 0x12
cli> read 0x40000000
0xaabb12dd
cli> reg
reg> read 0x40000000
0xaabb12dd
reg> field 0x40000000 0x0000ff00 8
0x00000012
reg> back
cli> exit
```

`reg <command ...>` は一行実行、`reg` 単独はサブメニューに入ります。
`help` は現在のメニュー、`reg help` は指定メニューのコマンドを表示します。
`back` は一つ前に戻り、`exit` はどのメニューからでもアプリケーションを終了します。
EOFでも終了します。ハンドラは順番に同期実行します。

## コマンドを追加する

1. `src/commands/<機能名>_cmd.c` と対応する `.h` を追加します。
2. ハンドラを `int handler(int argc, char **argv)` で実装します。
3. `src/main.c` でヘッダをincludeし、コマンド表に登録します。

登録例：

```c
{"status", status_handler, "status", "Show status", NULL},
```

機能ごとのサブメニューを追加する場合は、`register_cmd.c` と同じように
`static const cli_command[]` と `const cli_menu` を定義します。
メインには次のようにメニューを一件登録します。

```c
{"device", NULL, "device [command ...]", "Device commands", &device_menu},
```

`handler` と `submenu` はどちらか一方だけ設定します。コマンド名は各メニュー内で
一意にし、共通コマンドの `help`・`back`・`exit` は使用しません。
`src/commands/` と `src/core/` 直下の `.c` は自動でビルド対象になります。
ヘッダの依存関係も自動追跡します。

ハンドラにはその階層のコマンド名が `argv[0]` として渡ります。
`argv[argc]` はNULLです。引数は実行中だけ有効なので、保持する場合はコピーします。
戻り値は `CLI_OK`（成功）、`CLI_USAGE`（引数誤り）、`CLI_ERROR`（実行エラー）です。
引数誤りの場合だけusageを表示し、実行エラーの詳細はハンドラが出力します。

入力は空白区切り、最大32引数（コマンド名を含む）、一行1023文字です。
引用符・エスケープ・履歴・補完は扱いません。メニューの深さはルートを含め16です。
`parse_u64` と `parse_u32` は10進数または `0x` 付き16進数を受け付け、
負数・範囲超過・末尾の余計な文字を拒否します。

## メモリ・レジスタ設定

`include/app_config.h` の `REG_BASE` と `REG_SIZE` を対象に合わせます。
既定値は `0x40000000` と `0x1000` で、アクセスは4バイト境界の32ビット単位です。
範囲外・非整列アクセスはエラーにします。

模擬ファイル内の位置は `address - REG_BASE` です。未書き込み領域は0として読み、
再起動後も値が残ります。既存ファイルを切り詰めません。ファイル名は次のように変更できます。

```sh
DEBUG_MEMORY_FILE=/tmp/my_memory.bin ./build/mock/debug_cli
```

独自の模擬ファイルの保存先は、リポジトリ外にするか `.gitignore` に追加してください。
通常のファイル読み書きではCPUのネイティブバイト順を使います。

`field <addr> <mask> <shift> [value]` のmaskはワード中のビット位置を表します。
読み出しは `(raw & mask) >> shift`、書き込みは該当ビットのみを更新します。
値がmaskに収まらない場合は拒否します。書き込みは非アトミックなread-modify-writeなので、
W1C・read-clear・他の処理が同時更新するレジスタには使用しないでください。

実機アクセスには対象ボードの `/dev/mem` 権限・マッピング属性が必要です。
`volatile` な32ビットアクセスを行います。ボード固有のメモリバリアや
キャッシュ管理が必要な場合はI/O層で対応してください。実機動作は未検証です。

## ベース環境への適用方針

既存ツールのコマンド表、サブメニュー、マスク操作、模擬メモリ切り替えを参考に、
別プロジェクトでも使える共通処理として整理しています。
装置固有のレジスタ定義・設定データ・ファームウェア・IPI処理はプロジェクト側で追加します。
常時監視や非同期UIも、それらが必要になったプロジェクトで拡張する想定です。
