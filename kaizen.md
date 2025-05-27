# Chordlockコード検出アルゴリズム改善レポート

## 目次

1. [現状分析と課題](#1-現状分析と課題)
2. [コード表記の一貫性向上](#2-コード表記の一貫性向上)
3. [複雑なコード検出の強化](#3-複雑なコード検出の強化)
4. [ONコード検出の改善](#4-onコード検出の改善)
5. [ボイシング検出の実装](#5-ボイシング検出の実装)
6. [アルゴリズムの最適化](#6-アルゴリズムの最適化)
7. [実装ロードマップ](#7-実装ロードマップ)
8. [まとめ](#8-まとめ)

## 1. 現状分析と課題

Chordlockのコード検出アルゴリズムは、MIDIノート入力からコードを識別する優れた基本機能を備えていますが、いくつかの課題が確認されています。

### 1.1 コード表記の不一致

現状のCLIテストでは以下の表記不一致が確認されています：
- マイナーコードの表記が「min」と「m」で混在（例：Dm7とDmin7）
- メジャーコードの表記が空文字と「maj」で混在
- 拡張コードの表記に一貫性がない

### 1.2 複雑なコード検出の限界

- テンションノート（9th, 11th, 13th）の解釈が不十分
- 同じ音の組み合わせに対する複数の解釈の優先順位が不明確
- コンテキスト（調性、ジャンル）の考慮が限定的

### 1.3 ONコード検出の課題

- ONコードの検出精度が低い（例：F/GがFadd9/Gとして検出される）
- ベース音がコードの構成音でない場合の処理が不十分
- 転回形の明示的な識別がない

### 1.4 ボイシング検出の欠如

- 同じコードでも異なるボイシングの区別がない
- 音の広がりや配置の情報が活用されていない
- ジャズやクラシックで重要なボイシングの特定機能がない

### 1.5 パフォーマンスの課題

- 全ての可能性を探索するため計算量が多い
- 不要な計算や重複処理がある
- キャッシュ機構が不十分

## 2. コード表記の一貫性向上

### 2.1 統一された表記規則

以下の表記規則を採用することで一貫性を確保します：

| コードタイプ | 現状の表記 | 提案する統一表記 | 例 |
|------------|----------|--------------|-----|
| メジャー | "", "maj" | "" | C |
| マイナー | "min", "m" | "m" | Cm |
| ドミナント7th | "7" | "7" | C7 |
| メジャー7th | "maj7", "M7" | "M7" | CM7 |
| マイナー7th | "min7", "m7" | "m7" | Cm7 |
| サスペンデッド4th | "sus4" | "sus4" | Csus4 |
| サスペンデッド2nd | "sus2" | "sus2" | Csus2 |
| ディミニッシュ | "dim" | "dim" | Cdim |
| オーギュメント | "aug" | "aug" | Caug |
| 6th | "6" | "6" | C6 |
| マイナー6th | "min6", "m6" | "m6" | Cm6 |
| 9th | "9" | "9" | C9 |
| メジャー9th | "maj9" | "M9" | CM9 |
| マイナー9th | "min9", "m9" | "m9" | Cm9 |
| 11th | "11" | "11" | C11 |
| 13th | "13" | "13" | C13 |
| アドノート | "add9", "add11" | "add9", "add11" | Cadd9 |

### 2.2 実装案：表記変換マッピング

```cpp
// formatChordName関数の改善
std::string Chordlock::formatChordName(int root, const std::string &type, int bass) {
  static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  std::string rootName = names[root % 12];

  // 表記の一貫性を確保するマッピングテーブル
  static const std::unordered_map<std::string, std::string> typeMap = {
    {"maj", ""},      // メジャーは記号なし
    {"min", "m"},     // マイナーはmに統一
    {"maj7", "M7"},   // メジャー7はM7に統一
    {"min7", "m7"},   // マイナー7はm7に統一
    {"7", "7"},       // ドミナント7はそのまま
    {"dim", "dim"},   // ディミニッシュはそのまま
    {"aug", "aug"},   // オーギュメントはそのまま
    {"sus2", "sus2"}, // サスペンデッド2はそのまま
    {"sus4", "sus4"}, // サスペンデッド4はそのまま
    {"min9", "m9"},   // マイナー9はm9に統一
    {"maj9", "M9"},   // メジャー9はM9に統一
    {"min11", "m11"}, // マイナー11はm11に統一
    {"maj11", "M11"}, // メジャー11はM11に統一
    {"min13", "m13"}, // マイナー13はm13に統一
    {"maj13", "M13"}  // メジャー13はM13に統一
  };

  // 表記を変換
  std::string formattedType = type;
  auto it = typeMap.find(type);
  if (it != typeMap.end()) {
    formattedType = it->second;
  }

  // 結合
  std::string name = rootName + formattedType;

  // ベース音が指定されている場合は追加
  if (detectOnChords && bass >= 0 && bass != root) {
    name += "/" + std::string(names[bass % 12]);
  }

  return name;
}
```

## 3. 複雑なコード検出の強化

### 3.1 コンテキスト考慮の強化

調性、コード進行、ジャンルなどのコンテキストを考慮することで、複雑なコードの検出精度を向上させます。

```cpp
// 調性とコード進行の重み付けを強化
float Chordlock::adjustConfidenceByContext(int root, const std::string &type) {
  float keyAdjustment = adjustConfidenceByKey(root, type, currentKey);
  float progressionAdjustment = adjustConfidenceByProgression(root, type);
  
  // ジャンルや曲調に基づく追加調整
  float genreAdjustment = 1.0f;
  if (currentGenre == GENRE_JAZZ) {
    // ジャズではテンションコードを優先
    if (type.find("9") != std::string::npos || 
        type.find("11") != std::string::npos || 
        type.find("13") != std::string::npos) {
      genreAdjustment *= 1.3f;
    }
  } else if (currentGenre == GENRE_ROCK) {
    // ロックでは5thパワーコードを優先
    if (type == "5") {
      genreAdjustment *= 1.5f;
    }
  }
  
  return keyAdjustment * progressionAdjustment * genreAdjustment;
}
```

### 3.2 テンションノート検出の改善

テンションノート（9th, 11th, 13th）を正確に検出し、コード名に反映させます。

```cpp
// テンションノートの検出を改善
void Chordlock::analyzeTensions(uint16_t mask, int root, std::string &type) {
  // 基本コードの構成音を除外
  uint16_t baseChordMask = 0;
  for (const auto &pattern : chordPatterns) {
    if (pattern.name == type) {
      baseChordMask = ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) & 0xFFF;
      break;
    }
  }
  
  // 残りの音をテンションとして分析
  uint16_t tensionMask = mask & ~baseChordMask;
  if (tensionMask == 0) return;
  
  // 9th (2nd + オクターブ)
  if (tensionMask & (1 << ((root + 2) % 12))) {
    if (type.find("9") == std::string::npos) {
      // 7thコードの場合は9thに、それ以外はadd9に
      if (type.find("7") != std::string::npos || type.find("M7") != std::string::npos) {
        // 既に7thコードの場合、9thに拡張
        if (type == "7") type = "9";
        else if (type == "m7") type = "m9";
        else if (type == "M7") type = "M9";
        // その他の7thコードはそのまま+9
        else type += "9";
      } else {
        type += "add9";
      }
    }
  }
  
  // 11th (4th + オクターブ)
  if (tensionMask & (1 << ((root + 5) % 12))) {
    if (type.find("11") == std::string::npos && type.find("sus4") == std::string::npos) {
      // 9thコードの場合は11thに、7thコードの場合は11に、それ以外はadd11に
      if (type.find("9") != std::string::npos) {
        // 9thから11thへ
        if (type == "9") type = "11";
        else if (type == "m9") type = "m11";
        else type += "11";
      } else if (type.find("7") != std::string::npos || type.find("M7") != std::string::npos) {
        type += "11";
      } else {
        type += "add11";
      }
    }
  }
  
  // 13th (6th + オクターブ)
  if (tensionMask & (1 << ((root + 9) % 12))) {
    if (type.find("13") == std::string::npos && type.find("6") == std::string::npos) {
      // 11thコードの場合は13thに、9th/7thコードの場合は13に、それ以外は6に
      if (type.find("11") != std::string::npos) {
        // 11thから13thへ
        if (type == "11") type = "13";
        else if (type == "m11") type = "m13";
        else type += "13";
      } else if (type.find("9") != std::string::npos || 
                type.find("7") != std::string::npos || 
                type.find("M7") != std::string::npos) {
        type += "13";
      } else {
        // 基本コードの場合は6th
        if (type == "") type = "6";
        else if (type == "m") type = "m6";
        else type += "6";
      }
    }
  }
}
```

### 3.3 コード検出の優先順位付け

複数の解釈が可能な場合の優先順位を明確にします。

```cpp
// コード候補の優先順位付け
void Chordlock::prioritizeCandidates(std::vector<ChordCandidate> &candidates) {
  // 基本的な信頼度でソート
  std::sort(candidates.begin(), candidates.end(),
            [](const auto &a, const auto &b) { return a.confidence > b.confidence; });
  
  // 特定のケースで優先順位を調整
  for (auto &candidate : candidates) {
    // 1. ルート音が最低音の場合は優先度アップ
    if (candidate.name.find('/') == std::string::npos) {
      candidate.confidence *= 1.2f;
    }
    
    // 2. シンプルなコードを優先（テンションが少ないほど優先）
    float complexity = 1.0f;
    if (candidate.name.find("add") != std::string::npos) complexity *= 0.95f;
    if (candidate.name.find("9") != std::string::npos) complexity *= 0.98f;
    if (candidate.name.find("11") != std::string::npos) complexity *= 0.96f;
    if (candidate.name.find("13") != std::string::npos) complexity *= 0.94f;
    
    candidate.confidence *= complexity;
    
    // 3. 一般的なコードタイプを優先
    if (candidate.name.length() <= 3) { // C, Cm, C7など
      candidate.confidence *= 1.1f;
    }
  }
  
  // 再ソート
  std::sort(candidates.begin(), candidates.end(),
            [](const auto &a, const auto &b) { return a.confidence > b.confidence; });
}
```

## 4. ONコード検出の改善

### 4.1 ONコード検出の精度向上

ONコードの検出精度を向上させ、特に一般的なONコードパターンを正確に識別します。

```cpp
// ONコード検出の改善
float Chordlock::evaluateOnChord(int root, const std::string &type, int bass, uint16_t mask) {
  if (bass < 0 || bass == root) return 1.0f;
  
  // ベース音がコードの構成音かチェック
  uint16_t chordMask = 0;
  for (const auto &pattern : chordPatterns) {
    if (pattern.name == type) {
      chordMask = ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) & 0xFFF;
      break;
    }
  }
  
  bool bassIsChordTone = (chordMask & (1 << bass)) != 0;
  
  // ONコードの種類に基づく調整
  if (bassIsChordTone) {
    // ベース音がコードの構成音の場合は転回形
    
    // 第3音がベース（例：C/E）
    if (type == "" && bass == (root + 4) % 12) {
      return 2.0f; // 第1転回形は非常に一般的
    }
    
    // 第5音がベース（例：C/G）
    if (type == "" && bass == (root + 7) % 12) {
      return 1.8f; // 第2転回形も一般的
    }
    
    // マイナーコードの第3音がベース（例：Cm/Eb）
    if (type == "m" && bass == (root + 3) % 12) {
      return 2.0f; // マイナーの第1転回形
    }
    
    // 7thコードの第3音がベース（例：C7/E）
    if (type == "7" && bass == (root + 4) % 12) {
      return 1.9f;
    }
    
    // 7thコードの第5音がベース（例：C7/G）
    if (type == "7" && bass == (root + 7) % 12) {
      return 1.8f;
    }
    
    // 7thコードの第7音がベース（例：C7/Bb）
    if (type == "7" && bass == (root + 10) % 12) {
      return 1.7f;
    }
    
    return 1.6f; // その他の転回形
  } else {
    // ベース音がコードの構成音でない場合
    
    // 4度上のルートがベース（例：G7/C）- 特にV7/Iは非常に一般的
    if (type.find("7") != std::string::npos && (root - bass + 12) % 12 == 5) {
      return 1.9f;
    }
    
    // 短2度上の音がベース（例：C/Db）- テンション的な効果
    if ((bass - root + 12) % 12 == 1) {
      return 1.3f;
    }
    
    // 長2度上の音がベース（例：C/D）- sus9的な効果
    if ((bass - root + 12) % 12 == 2) {
      return 1.4f;
    }
    
    // その他のONコード
    return 1.2f;
  }
}
```

### 4.2 ONコード表記の改善

ONコードの表記を改善し、特に転回形の場合は明示的に表示します。

```cpp
// ONコード表記の改善
std::string Chordlock::formatOnChordName(int root, const std::string &type, int bass) {
  static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  std::string rootName = names[root % 12];
  std::string formattedType = formatChordType(type); // 前述の統一表記を適用
  
  // 基本形
  if (bass < 0 || bass == root) {
    return rootName + formattedType;
  }
  
  // ベース音がコードの構成音かチェック
  uint16_t chordMask = getChordMask(root, type);
  bool bassIsChordTone = (chordMask & (1 << bass)) != 0;
  
  // 転回形の特別表記（オプション機能）
  if (bassIsChordTone && showInversionInfo) {
    // メジャーコードの転回形
    if (type == "" || type == "maj") {
      if (bass == (root + 4) % 12) {
        return rootName + formattedType + "/1st"; // 第1転回形
      } else if (bass == (root + 7) % 12) {
        return rootName + formattedType + "/2nd"; // 第2転回形
      }
    }
    // マイナーコードの転回形
    else if (type == "m" || type == "min") {
      if (bass == (root + 3) % 12) {
        return rootName + formattedType + "/1st"; // 第1転回形
      } else if (bass == (root + 7) % 12) {
        return rootName + formattedType + "/2nd"; // 第2転回形
      }
    }
    // 7thコードの転回形
    else if (type == "7") {
      if (bass == (root + 4) % 12) {
        return rootName + formattedType + "/1st"; // 第1転回形
      } else if (bass == (root + 7) % 12) {
        return rootName + formattedType + "/2nd"; // 第2転回形
      } else if (bass == (root + 10) % 12) {
        return rootName + formattedType + "/3rd"; // 第3転回形
      }
    }
  }
  
  // 標準的なONコード表記
  return rootName + formattedType + "/" + names[bass % 12];
}
```

## 5. ボイシング検出の実装

### 5.1 ボイシング分析機能

コードのボイシング（音の配置）を分析し、転回形や音の広がりを検出します。

```cpp
// ボイシング検出の追加
struct ChordVoicing {
  std::string name;       // コード名
  std::string voicingType; // ボイシングタイプ（"root", "1st", "2nd", "spread", etc.）
  std::vector<int> notes; // 実際の音符（MIDI番号）
  int range;              // 最高音と最低音の差（半音単位）
  int density;            // 音の密度（音数/範囲）
};

ChordVoicing Chordlock::detectVoicing(uint32_t now_ms) {
  ChordCandidate chord = detect(now_ms);
  ChordVoicing voicing;
  voicing.name = chord.name;
  
  // 実際に押されている音符を収集
  std::vector<int> activeNotes;
  for (int i = 0; i < 128; ++i) {
    if (noteStates[i]) {
      activeNotes.push_back(i);
    }
  }
  
  if (activeNotes.empty()) {
    voicing.voicingType = "none";
    return voicing;
  }
  
  voicing.notes = activeNotes;
  
  // ルート音と種類を抽出
  int root = -1;
  std::string type;
  parseChordName(chord.name, root, type);
  
  if (root < 0) {
    voicing.voicingType = "unknown";
    return voicing;
  }
  
  // 最低音と最高音を確認
  int lowestNote = *std::min_element(activeNotes.begin(), activeNotes.end());
  int highestNote = *std::max_element(activeNotes.begin(), activeNotes.end());
  int lowestPitch = lowestNote % 12;
  
  // 音域を計算
  voicing.range = highestNote - lowestNote;
  
  // 音の密度を計算
  voicing.density = activeNotes.size() * 12 / (voicing.range + 1);
  
  // 転回形を判定
  if (lowestPitch == root) {
    voicing.voicingType = "root";
  } else if (type == "" && lowestPitch == (root + 4) % 12) {
    voicing.voicingType = "1st";  // 第1転回形（例：C/E）
  } else if (type == "" && lowestPitch == (root + 7) % 12) {
    voicing.voicingType = "2nd";  // 第2転回形（例：C/G）
  } else if (type == "m" && lowestPitch == (root + 3) % 12) {
    voicing.voicingType = "1st";  // マイナーの第1転回形
  } else if (type == "m" && lowestPitch == (root + 7) % 12) {
    voicing.voicingType = "2nd";  // マイナーの第2転回形
  } else if (type == "7" && lowestPitch == (root + 4) % 12) {
    voicing.voicingType = "1st";  // 7thの第1転回形
  } else if (type == "7" && lowestPitch == (root + 7) % 12) {
    voicing.voicingType = "2nd";  // 7thの第2転回形
  } else if (type == "7" && lowestPitch == (root + 10) % 12) {
    voicing.voicingType = "3rd";  // 7thの第3転回形
  } else {
    // その他のボイシング
    voicing.voicingType = "other";
  }
  
  // 音の広がりを分析
  if (voicing.range > 12) {
    voicing.voicingType += "-spread"; // 広いボイシング
  } else {
    voicing.voicingType += "-close";  // 狭いボイシング
  }
  
  return voicing;
}
```

### 5.2 特殊ボイシングの検出

ジャズやクラシックで使用される特殊なボイシングを検出します。

```cpp
// 特殊ボイシングの検出
void Chordlock::detectSpecialVoicings(ChordVoicing &voicing, int root, const std::string &type) {
  // 音符の相対位置を分析
  std::vector<int> pitchClasses;
  for (int note : voicing.notes) {
    pitchClasses.push_back(note % 12);
  }
  
  // 重複を除去して並べ替え
  std::sort(pitchClasses.begin(), pitchClasses.end());
  pitchClasses.erase(std::unique(pitchClasses.begin(), pitchClasses.end()), pitchClasses.end());
  
  // ジャズボイシング: シェルボイシング（ルート + 3rd/7th）
  if (pitchClasses.size() == 3) {
    if (type == "7" || type == "9" || type == "13") {
      bool hasRoot = std::find(pitchClasses.begin(), pitchClasses.end(), root) != pitchClasses.end();
      bool has3rd = std::find(pitchClasses.begin(), pitchClasses.end(), (root + 4) % 12) != pitchClasses.end();
      bool has7th = std::find(pitchClasses.begin(), pitchClasses.end(), (root + 10) % 12) != pitchClasses.end();
      
      if (hasRoot && has3rd && has7th) {
        voicing.voicingType = "shell";
        return;
      }
    }
  }
  
  // ジャズボイシング: So What Chord (Bill Evans)
  if (pitchClasses.size() == 5) {
    // D-7(11) in So What voicing: D F A C G
    std::vector<int> soWhatPattern = {0, 3, 7, 10, 5}; // 相対位置
    std::vector<int> relativePitches;
    
    for (int pitch : pitchClasses) {
      relativePitches.push_back((pitch - root + 12) % 12);
    }
    
    std::sort(relativePitches.begin(), relativePitches.end());
    
    if (relativePitches == soWhatPattern) {
      voicing.voicingType = "so-what";
      return;
    }
  }
  
  // クラシカルボイシング: トライアド + オクターブ
  if (pitchClasses.size() == 3 && voicing.notes.size() == 4) {
    bool hasRoot = std::find(pitchClasses.begin(), pitchClasses.end(), root) != pitchClasses.end();
    bool has3rd = std::find(pitchClasses.begin(), pitchClasses.end(), 
                           (root + (type == "m" ? 3 : 4)) % 12) != pitchClasses.end();
    bool has5th = std::find(pitchClasses.begin(), pitchClasses.end(), (root + 7) % 12) != pitchClasses.end();
    
    if (hasRoot && has3rd && has5th) {
      // オクターブ重複をチェック
      std::map<int, int> pitchCount;
      for (int note : voicing.notes) {
        pitchCount[note % 12]++;
      }
      
      for (const auto &[pitch, count] : pitchCount) {
        if (count > 1) {
          voicing.voicingType = "triad-octave";
          return;
        }
      }
    }
  }
}
```

## 6. アルゴリズムの最適化

### 6.1 ビットマスク操作の最適化

ビット操作を最適化し、計算効率を向上させます。

```cpp
// ビット操作の最適化
uint16_t Chordlock::buildChordMask() {
  // 事前計算されたビットマスクを使用
  static const uint16_t noteMasks[12] = {
    1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5,
    1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11
  };
  
  uint16_t mask = 0;
  
  // ベロシティ考慮が無効な場合
  if (!velocitySensitive) {
    for (int i = 0; i < 128; ++i) {
      if (noteStates[i]) {
        mask |= noteMasks[i % 12];
      }
    }
    return mask;
  }
  
  // ベロシティ考慮が有効な場合
  // 事前にソートされた配列を使用
  std::vector<std::pair<int, uint8_t>> activeNotes;
  activeNotes.reserve(12); // 最大12音
  
  for (int i = 0; i < 128; ++i) {
    if (noteStates[i]) {
      activeNotes.push_back({i % 12, velocities[i]});
    }
  }
  
  // ベロシティでソート
  std::sort(activeNotes.begin(), activeNotes.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });
  
  // マスクに追加
  for (const auto &[note, velocity] : activeNotes) {
    mask |= noteMasks[note];
  }
  
  return mask;
}
```

### 6.2 キャッシュの活用

結果キャッシュを実装し、同じ入力に対する重複計算を回避します。

```cpp
// 結果キャッシュの実装
struct ChordCache {
  uint16_t mask;
  uint32_t timestamp;
  ChordCandidate result;
};

// キャッシュを使用したdetect関数
ChordCandidate Chordlock::detect(uint32_t now_ms) {
  // 最近のキャッシュをチェック（同じマスクで短時間に複数回呼ばれる場合に有効）
  static ChordCache cache = {0, 0, {"", 0, 0.0}};
  
  uint16_t mask = buildChordMask();
  if (mask == 0) return {"—", 0, 0.0};
  
  // キャッシュヒット条件: 同じマスクで100ms以内
  if (mask == cache.mask && now_ms - cache.timestamp < 100) {
    return cache.result;
  }
  
  // 以下、通常の検出処理
  // ...（既存のコード）...
  
  // 結果をキャッシュに保存
  cache.mask = mask;
  cache.timestamp = now_ms;
  cache.result = bestCandidate;
  
  return bestCandidate;
}
```

### 6.3 パターンマッチングの最適化

パターンマッチングを高速化するためのルックアップテーブルを実装します。

```cpp
// パターンマッチングの高速化
void Chordlock::initializePatternLookup() {
  // 各ビットマスクパターンに対応するコードタイプのルックアップテーブルを構築
  patternLookup.clear();
  
  for (int root = 0; root < 12; ++root) {
    for (const auto &pattern : chordPatterns) {
      uint16_t shifted = ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) & 0xFFF;
      
      // このパターンをキーとして、対応するルートとコードタイプを保存
      patternLookup[shifted].push_back({root, pattern.name, pattern.weight});
    }
  }
}

// 高速なパターンマッチング
std::vector<ChordCandidate> Chordlock::findMatchingPatterns(uint16_t mask) {
  std::vector<ChordCandidate> candidates;
  
  // 完全一致を最初にチェック
  auto it = patternLookup.find(mask);
  if (it != patternLookup.end()) {
    for (const auto &[root, type, weight] : it->second) {
      candidates.push_back({formatChordName(root, type), mask, weight * 2.0f});
    }
    return candidates;
  }
  
  // 部分一致をチェック
  for (const auto &[pattern, matches] : patternLookup) {
    if ((mask & pattern) == pattern) {
      for (const auto &[root, type, weight] : matches) {
        float confidence = weight;
        
        // 余分な音の数に基づいて信頼度を調整
        int patternBits = __builtin_popcount(pattern);
        int maskBits = __builtin_popcount(mask);
        int extraBits = maskBits - patternBits;
        confidence *= (1.0f / (1.0f + extraBits * 0.3f));
        
        candidates.push_back({formatChordName(root, type), mask, confidence});
      }
    }
  }
  
  return candidates;
}
```

### 6.4 並列処理の導入

マルチスレッド処理を導入し、複数のコード候補を並列評価します。

```cpp
// 並列処理の導入
std::vector<ChordCandidate> Chordlock::detectParallel(uint32_t now_ms) {
  uint16_t mask = buildChordMask();
  std::vector<ChordCandidate> candidates;
  
  if (mask == 0) {
    candidates.push_back({"—", 0, 0.0});
    return candidates;
  }
  
  // 各ルート音ごとの候補を並列で評価
  std::vector<std::vector<ChordCandidate>> rootCandidates(12);
  
  #pragma omp parallel for
  for (int root = 0; root < 12; ++root) {
    std::vector<ChordCandidate> localCandidates;
    
    for (const auto &pattern : chordPatterns) {
      uint16_t shifted = ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) & 0xFFF;
      
      if ((mask & shifted) == shifted) {
        // 信頼度計算（既存のロジック）
        float confidence = calculateConfidence(root, pattern.name, pattern.weight, mask, shifted);
        
        // コード名を生成
        std::string name = formatChordName(root, pattern.name);
        
        localCandidates.push_back({name, mask, confidence});
      }
    }
    
    rootCandidates[root] = std::move(localCandidates);
  }
  
  // 結果をマージ
  for (const auto &rootCandidate : rootCandidates) {
    candidates.insert(candidates.end(), rootCandidate.begin(), rootCandidate.end());
  }
  
  // 信頼度でソート
  std::sort(candidates.begin(), candidates.end(),
            [](const auto &a, const auto &b) { return a.confidence > b.confidence; });
  
  return candidates;
}
```

## 7. 実装ロードマップ

以下の順序で改善を実装することを推奨します：

1. **コード表記の一貫性向上**
   - 最も基本的な改善であり、他の機能に影響を与えない
   - `formatChordName`関数の修正のみで実装可能

2. **アルゴリズムの最適化**
   - パフォーマンス向上は他の機能改善の基盤となる
   - キャッシュ機構とビットマスク最適化を優先実装

3. **ONコード検出の改善**
   - 現状の検出精度の問題を解決する重要な改善
   - `evaluateOnChord`関数の実装と信頼度計算の調整

4. **複雑なコード検出の強化**
   - テンションノート検出と優先順位付けの実装
   - コンテキスト考慮機能の拡張

5. **ボイシング検出の実装**
   - 最も高度な機能として最後に実装
   - 他の改善が完了した後に追加機能として実装

## 8. まとめ

Chordlockのコード検出アルゴリズムは、以下の改善によって大幅に強化できます：

1. **コード表記の一貫性向上**
   - 統一された表記規則の採用
   - マッピングテーブルによる表記変換

2. **複雑なコード検出の強化**
   - コンテキスト考慮の強化
   - テンションノート検出の改善
   - コード候補の優先順位付け

3. **ONコード検出の改善**
   - ONコード検出の精度向上
   - 転回形の明示的な表記

4. **ボイシング検出の実装**
   - ボイシング分析機能
   - 特殊ボイシングの検出

5. **アルゴリズムの最適化**
   - ビットマスク操作の最適化
   - 結果キャッシュの活用
   - パターンマッチングの高速化
   - 並列処理の導入

これらの改善を実装することで、Chordlockはより正確で高速なコード検出が可能になり、音楽制作やコード学習、即興演奏のサポートツールとしての価値が大幅に向上するでしょう。
