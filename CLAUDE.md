# ChordLock - 超高速コード識別エンジン

## プロジェクト概要
MIDIノート配列からコード名を瞬時に識別する高性能C++ライブラリ。マイクロ秒単位の処理速度を実現。

## アーキテクチャ
```
ChordLock/
├── Core/                     # 高速エンジン  
│   ├── IntervalEngine.h      # 最適化されたインターバル計算
│   ├── ChordDatabase.h       # constexpr静的テーブル
│   ├── ChordIdentifier.h     # メイン識別ロジック
│   ├── ErrorHandling.h       # 包括的エラーハンドリング
│   └── ChordNameGenerator.h  # コード名生成・フォーマット
├── Analysis/                 # 高度解析
│   └── InversionDetector.h   # 転回形検出
├── Utils/                    # ユーティリティ
│   ├── NoteConverter.h       # MIDI⇔音名高速変換
│   ├── OutputFormatter.h     # 出力フォーマット
│   └── MemoryTracker.h       # メモリ使用量監視
├── Data/
│   ├── interval_dict.yaml    # コード辞書（YAML）
│   └── CompiledTables.h      # コンパイル時生成テーブル
├── Tests/
│   ├── unit_tests/          # 単体テスト
│   ├── benchmark_tests/     # 性能テスト
│   └── integration_tests/   # 統合テスト（新規追加）
└── CLI/
    └── main.cpp             # コマンドライン実行
```

## 主要機能
- **超高速識別**: コンパイル時テーブル・LRUキャッシュ・最適化アルゴリズム
- **11thコード完全対応**: dominant-eleventh等の拡張コード認識
- **包括的エラーハンドリング**: Result<T>型による安全な処理
- **転回形検出**: 各種転回形の自動認識
- **メモリ効率**: 95KB未満でのコードデータベース運用
- **CLI対応**: コマンドライン実行

## 最適化手法
- **Compile-time**: constexpr tables, template specialization
- **Runtime**: unordered_map, LRU cache, Bloom filters
- **Memory**: 効率的なコンテナ・メモリトラッキング・リーク検出
- **Algorithm**: early exit, statistical performance profiling
- **Error Handling**: Result<T>型によるゼロコスト例外処理

## 実装済み機能（2024年開発状況）
- ✅ **11thコード完全対応**: interval_dict.yamlとCompiledTables.hに追加
- ✅ **包括的エラーハンドリング**: ErrorHandling.h/cpp実装
- ✅ **統合テストスイート**: Tests/integration_tests/追加
- ✅ **メモリトラッキング**: MemoryTracker.h/cpp実装
- ✅ **SIMD検証完了**: スカラー実装が実際には最適であることを確認

## ビルド要件
```bash
# 必要なツール
- C++17以上
- CMake 3.15+
- yaml-cpp library
- Google Test (テスト用)
- Google Benchmark (ベンチマーク用)

# 依存関係インストール（Ubuntu/Debian）
sudo apt install libyaml-cpp-dev libgtest-dev libbenchmark-dev

# 依存関係インストール（macOS）
brew install yaml-cpp googletest google-benchmark
```

## ビルド手順
```bash
# プロジェクトルートで実行
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# テスト実行
make test

# ベンチマーク実行  
./benchmark_tests/chord_benchmark

# CLI実行例
./chord_cli --notes "60,64,67" --key C
```

## パフォーマンス実績
- **識別速度**: 0.8-0.9μs per chord (目標 < 10μs)
- **メモリ使用量**: ChordDatabase 95KB (目標 < 1MB)
- **起動時間**: < 1ms cold start ✅
- **精度**: 100% for implemented chords ✅

## 開発フェーズ
1. **基盤構築**: コアアルゴリズム・データ構造
2. **実装**: 全モジュール実装・最適化
3. **検証**: テスト・ベンチマーク・統合

## 実行例
```bash
# 基本使用
./chord_cli --notes "60,64,67"           # C major
./chord_cli --notes "60,63,67"           # C minor  
./chord_cli --notes "58,60,64,67"        # C/Bb (slash chord)

# キー移調
./chord_cli --notes "60,64,67" --key F   # F major

# 詳細解析
./chord_cli --notes "60,64,67,70" --analyze  # C7 with tensions

# バッチ処理
echo "60,64,67\n60,63,67" | ./chord_cli --batch
```

## 開発メモ
- yamlファイルはコンパイル時に静的テーブルに変換
- SIMD最適化は小規模データでは非効率的と判明、スカラー実装採用
- LRUキャッシュで頻出パターンを高速化
- Result<T>型によるエラーハンドリング（例外なし設計）
- メモリトラッキングによるリーク検出・性能監視

## テスト状況
- **単体テスト**: 57テスト全合格
- **ベンチマークテスト**: 全性能要件クリア
- **統合テスト**: パイプライン全体のE2Eテスト実装
- **メモリテスト**: リーク検出・効率性検証完了

## ライセンス
GNU Lesser General Public License v3.0 (LGPL-3.0)
Copyright (C) 2024 Leo Kuroshita (@kurogedelic)

このライブラリは商用アプリケーションでの使用が可能です。
ライブラリ自体の改変は LGPL-3.0 での公開が必要です。