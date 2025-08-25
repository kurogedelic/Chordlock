# ChordLock プロジェクト構造

## 📁 ディレクトリ構成

### Core/ - コアエンジン
高速コード識別の中核となるモジュール群
- `ChordDatabase.*` - コードデータベース管理
- `ChordIdentifier.*` - メインの識別ロジック
- `IntervalEngine.*` - インターバル計算エンジン
- `ChordNameGenerator.*` - コード名生成
- `ErrorHandling.*` - エラーハンドリング (Result<T>)
- `AdvancedChordRecognition.*` - 高度な認識 (ジャズ/現代音楽)
- `ProgressionAnalyzer.*` - コード進行解析

### Core/最適化モジュール
高速化のための特殊データ構造
- `RobinHoodHash.h` - Robin Hood Hashing実装
- `PerfectHash.h` - Perfect Minimal Hash実装
- `CacheObliviousBTree.h` - キャッシュ最適化B-tree
- `PerformanceStrategy.h` - パフォーマンス戦略
- `ChordDatabaseCompat.h` - 互換性レイヤー

### Analysis/ - 解析モジュール
- `InversionDetector.*` - 転回形検出

### Utils/ - ユーティリティ
- `NoteConverter.*` - MIDI⇔音名変換
- `OutputFormatter.*` - 出力フォーマット
- `MemoryTracker.*` - メモリ使用量追跡
- `PerformanceProfiler.h` - パフォーマンス計測

### Data/ - データファイル
- `interval_dict.yaml` - 基本コード辞書 (304種類)
- `extended_chords.yaml` - 拡張コード辞書 (200種類以上)
- `CompiledTables.h` - コンパイル時生成テーブル

### Tests/ - テストスイート
- `unit_tests/` - 単体テスト
- `integration_tests/` - 統合テスト
- `benchmark_tests/` - パフォーマンステスト
- `*.yaml` - テストデータ

### CLI/ - コマンドラインインターフェース
- `main.cpp` - CLIエントリポイント

## 🔑 重要ファイル

### 設定・ビルド
- `CMakeLists.txt` - CMakeビルド設定
- `chordlock.pc.in` - pkg-config設定
- `.gitignore` - Git除外設定

### ドキュメント
- `README.md` - 英語版README
- `README_JP.md` - 日本語版README
- `CLAUDE.md` - 開発仕様書
- `PERFORMANCE_REPORT.md` - パフォーマンスレポート
- `LICENSE.txt` - LGPL-3.0ライセンス

## 📊 統計情報
- **総ファイル数**: 43
- **コアモジュール**: 21ファイル
- **対応コード数**: 500種類以上
- **メモリ使用量**: 215KB
- **識別速度**: 0.749μs/chord