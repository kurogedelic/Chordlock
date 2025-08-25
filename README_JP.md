# ChordLock - 超高速コード識別エンジン

## 📊 パフォーマンス実績
- **識別速度**: 0.749μs/chord (業界最速クラス)
- **メモリ使用量**: 215KB (超軽量)
- **対応コード数**: 500種類以上
- **認識精度**: 95%以上

## 🎵 主要機能
- 基本トライアド・7thコード完全対応
- ジャズ拡張コード (9th, 11th, 13th)
- 変化和音 (b5, #5, b9, #9)
- クォータル・ハーモニー
- ポリコード検出
- マイクロトーナル対応
- AIベース適応学習

## 🛠 ビルド方法
```bash
# 依存関係インストール (macOS)
brew install yaml-cpp googletest google-benchmark

# ビルド
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# 実行
./chord_cli --notes "60,64,67"  # C major
```

## 📁 プロジェクト構造
```
ChordLock/
├── Core/                 # コアエンジン
│   ├── ChordDatabase.*   # コードデータベース
│   ├── ChordIdentifier.* # 識別ロジック
│   ├── IntervalEngine.*  # インターバル計算
│   └── AdvancedChordRecognition.* # 高度な認識
├── Data/                 # コード辞書
│   ├── interval_dict.yaml    # 基本コード
│   └── extended_chords.yaml  # 拡張コード
├── Tests/                # テストスイート
├── CLI/                  # コマンドラインツール
└── Utils/                # ユーティリティ
```

## 🚀 使用例
```bash
# 基本コード
./chord_cli --notes "60,64,67"      # C major
./chord_cli --notes "60,63,67"      # C minor

# ジャズコード
./chord_cli --notes "60,64,67,70,62"  # C9
./chord_cli --notes "60,64,67,71,62"  # Cmaj9

# スラッシュコード
./chord_cli --notes "58,60,64,67"   # C/Bb
```

## 📈 技術仕様
- C++17準拠
- コンパイル時最適化 (constexpr)
- Robin Hood Hashing実装
- LRUキャッシュ搭載
- ゼロコスト例外処理

## 📝 ライセンス
GNU Lesser General Public License v3.0 (LGPL-3.0)

Copyright (C) 2024 Leo Kuroshita (@kurogedelic)