#include "Chordlock.hpp"
#include "chord_definitions.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <unordered_map>

Chordlock::Chordlock()
    : chordPatterns(getChordPatternsData()) // ← 追加
{
  historyIndex = 0;
  for (int i = 0; i < MAX_CHORD_HISTORY; ++i)
    chordHistory[i] = {"", 0, 0.0};
}

void Chordlock::noteOn(uint8_t note, uint32_t time_ms) {
  noteStates[note % 128] = true;
  lastOnTime[note % 128] = time_ms;
}

void Chordlock::noteOff(uint8_t note) { noteStates[note % 128] = false; }

void Chordlock::setVelocity(uint8_t note, uint8_t velocity) {
  velocities[note % 128] = velocity;
}

void Chordlock::reset() {
  for (int i = 0; i < 128; ++i) {
    noteStates[i] = false;
    lastOnTime[i] = 0;
    velocities[i] = 0;
  }
  historyIndex = 0;
}

uint16_t Chordlock::buildChordMask() {
  // 事前計算されたビットマスクを使用
  static const uint16_t noteMasks[12] = {
    1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5,
    1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11
  };
  
  uint16_t mask = 0;

  // ベロシティ考慮が無効な場合は単純に押されているノートをマスクに追加
  if (!velocitySensitive) {
    for (int i = 0; i < 128; ++i) {
      if (noteStates[i]) {
        mask |= noteMasks[i % 12];
      }
    }
    return mask;
  }

  // ベロシティ考慮が有効な場合は、強く押されたノートを優先
  // 事前にソートされた配列を使用
  std::vector<std::pair<int, uint8_t>> activeNotes;
  activeNotes.reserve(12); // 最大12音
  
  for (int i = 0; i < 128; ++i) {
    if (noteStates[i]) {
      activeNotes.push_back({i % 12, velocities[i]});
    }
  }

  // ベロシティで降順ソート
  std::sort(activeNotes.begin(), activeNotes.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // マスクに追加
  for (const auto &[note, velocity] : activeNotes) {
    mask |= noteMasks[note];
  }

  return mask;
}

// コード表記の改善
std::string Chordlock::formatChordName(int root, const std::string &type,
                                       int bass) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
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
    {"6", "6"},       // 6thはそのまま
    {"min6", "m6"},   // マイナー6はm6に統一
    {"9", "9"},       // 9thはそのまま
    {"min9", "m9"},   // マイナー9はm9に統一
    {"maj9", "M9"},   // メジャー9はM9に統一
    {"11", "11"},     // 11thはそのまま
    {"min11", "m11"}, // マイナー11はm11に統一
    {"13", "13"},     // 13thはそのまま
    {"min13", "m13"}, // マイナー13はm13に統一
    {"add9", "add9"}, // add9はそのまま
    {"add11", "add11"}, // add11はそのまま
    {"5", "5"}        // パワーコードはそのまま
  };

  // 表記を変換
  std::string formattedType = type;
  auto it = typeMap.find(type);
  if (it != typeMap.end()) {
    formattedType = it->second;
  }

  // 結合
  std::string name = rootName + formattedType;

  // ベース音が指定されている場合は追加（ONコード検知が有効な場合のみ）
  if (detectOnChords && bass >= 0 && bass != root) {
    name += "/" + std::string(names[bass % 12]);
  }

  return name;
}

std::string Chordlock::noteName(int n) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  return names[n % 12];
}

// 調性に基づく確信度調整
float Chordlock::adjustConfidenceByKey(int root, const std::string &type,
                                       int key) {
  // 調性内のダイアトニックコードは確信度が高くなるように調整
  // C Majorの場合：C, Dm, Em, F, G, Am, Bdim がダイアトニックコード

  // 調性における度数を計算（0=I, 1=ii, 2=iii, ...）
  int degree = (root - key + 12) % 12;

  // メジャースケールの音階（調性内の音）
  static const bool majorScale[12] = {
      true,  // I (C in C major)
      false, //
      true,  // ii (D in C major)
      false, //
      true,  // iii (E in C major)
      true,  // IV (F in C major)
      false, //
      true,  // V (G in C major)
      false, //
      true,  // vi (A in C major)
      false, //
      true,  // vii (B in C major)
  };

  // 調性内のコードパターン（メジャースケール）
  // C majorの場合: C(I), Dm(ii), Em(iii), F(IV), G(V), Am(vi), Bdim(vii)
  static const std::string diatonicTypes[7] = {"maj", "min", "min", "maj",
                                               "maj", "min", "dim"};
  static const int diatonicDegrees[7] = {0, 2, 4, 5, 7, 9, 11};

  // 調性内のダイアトニックコードか確認
  bool isDiatonic = false;
  for (int i = 0; i < 7; i++) {
    if (degree == diatonicDegrees[i] && type == diatonicTypes[i]) {
      isDiatonic = true;
      break;
    }
  }

  // 調性内の音（スケール内の音）が使われているか
  int scaleNotesUsed = 0;
  int nonScaleNotesUsed = 0;

  uint16_t mask = 0;
  for (const auto &pattern : chordPatterns) {
    if (pattern.name == type) {
      // このコードタイプのビットマスクを取得し、ルート音に合わせてシフト
      mask = ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) &
             0xFFF;
      break;
    }
  }

  // マスク内のビットを調べて、調性内の音かどうかを確認
  for (int i = 0; i < 12; i++) {
    if (mask & (1 << i)) {
      if (majorScale[(i - key + 12) % 12]) {
        scaleNotesUsed++;
      } else {
        nonScaleNotesUsed++;
      }
    }
  }

  // 調整値を計算
  float adjustment = 1.0f;

  // ダイアトニックコードは確信度上昇
  if (isDiatonic) {
    adjustment *= 1.3f;
  }

  // スケール内の音が多いほど確信度上昇、スケール外の音が多いほど確信度減少
  if (scaleNotesUsed + nonScaleNotesUsed > 0) {
    float ratio = static_cast<float>(scaleNotesUsed) /
                  (scaleNotesUsed + nonScaleNotesUsed);
    adjustment *= (1.0f + ratio * 0.2f);
  }

  return adjustment;
}

// コード進行の一般的なパターンに基づく確信度調整
float Chordlock::adjustConfidenceByProgression(int root,
                                               const std::string &type) {
  // 履歴が空の場合は調整なし
  if (historyIndex == 0)
    return 1.0f;

  // 一つ前のコードを取得
  int prevIndex = (historyIndex - 1) % MAX_CHORD_HISTORY;
  ChordCandidate prevChord = chordHistory[prevIndex];

  // 前のコードが検出されていない場合は調整なし
  if (prevChord.name.empty() || prevChord.confidence < 0.5f)
    return 1.0f;

  // 前のコードからルート音と種類を抽出
  int prevRoot = -1;
  std::string prevType;

  // コード名からルート音を抽出（例: "C#m7" -> C#）
  static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};
  for (int i = 0; i < 12; i++) {
    if (prevChord.name.find(noteNames[i]) == 0) {
      prevRoot = i;
      // タイプを抽出（ルート音の部分を取り除く）
      prevType = prevChord.name.substr(strlen(noteNames[i]));

      // スラッシュがある場合はベース音の部分を取り除く
      size_t slashPos = prevType.find('/');
      if (slashPos != std::string::npos) {
        prevType = prevType.substr(0, slashPos);
      }

      break;
    }
  }

  // 前のコードのルートが見つからなかった場合は調整なし
  if (prevRoot < 0)
    return 1.0f;

  // 一般的なコード進行パターンを評価
  float adjustment = 1.0f;

  // 5度圏進行（V -> I）
  if ((prevRoot - root + 12) % 12 == 7 &&
      (prevType.find("7") != std::string::npos ||
       prevType.find("9") != std::string::npos) &&
      (type == "maj" || type == "maj7")) {
    adjustment *= 1.4f; // V7 -> I は非常に一般的
  }

  // 2-5-1進行の一部
  if (((prevRoot - root + 12) % 12 == 7) &&        // V -> I
      (prevType.find("7") != std::string::npos) && // V7
      (type == "maj" || type == "maj7")) {         // I
    adjustment *= 1.3f;
  }

  if (((prevRoot - root + 12) % 12 == 5) &&        // ii -> V
      (prevType == "min" || prevType == "min7") && // ii or iim7
      (type.find("7") != std::string::npos)) {     // V7
    adjustment *= 1.2f;
  }

  // 平行調のマイナーコードに移行
  if ((root - prevRoot + 12) % 12 == 9 && // vi from I
      prevType == "maj" && type == "min") {
    adjustment *= 1.15f; // I -> vi
  }

  // 4度上昇（I -> IV）
  if ((root - prevRoot + 12) % 12 == 5 && prevType == "maj" && type == "maj") {
    adjustment *= 1.1f; // I -> IV
  }

  return adjustment;
}

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

ChordCandidate Chordlock::detect(uint32_t now_ms) {
  // ノートが押されていなければ判定しない
  uint16_t mask = buildChordMask();
  if (mask == 0)
    return {"—", 0, 0.0};

  if (__builtin_popcount(mask) == 1) {
    int note = __builtin_ctz(mask); // 0-11
    std::string n = noteName(note);
    return {n, mask, 1.0f}; // confidence 1
  }

  // 押されているノートを収集（音程クラスのみ）
  std::vector<int> pitchClasses;
  for (int i = 0; i < 12; ++i) {
    if (mask & (1 << i)) {
      pitchClasses.push_back(i);
    }
  }

  // 実際に押されている音符を収集（オクターブ情報を含む）
  std::vector<int> activeNotes;
  for (int i = 0; i < 128; ++i) {
    if (noteStates[i]) {
      activeNotes.push_back(i);
    }
  }

  // 最低音（ベース音）を取得
  int bassNote = -1;
  if (!activeNotes.empty()) {
    bassNote = *std::min_element(activeNotes.begin(), activeNotes.end()) % 12;
  } else if (!pitchClasses.empty()) {
    bassNote = *std::min_element(pitchClasses.begin(), pitchClasses.end());
  }

  // 候補を格納する配列
  std::vector<ChordCandidate> candidates;

  // 特殊ケースの判定
  // 特定の組み合わせを直接判定することで、アルゴリズムの一貫性を確保

  // C+D+G (0,2,7) -> Csus2
  if (pitchClasses.size() == 3 &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 0) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 2) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 7) !=
          pitchClasses.end()) {
    // ベース音に応じて転回形を判定
    std::string name =
        formatChordName(0, "sus2", bassNote != 0 ? bassNote : -1);
    ChordCandidate candidate = {name, mask, 1.0};

    // 履歴に追加
    chordHistory[historyIndex % MAX_CHORD_HISTORY] = candidate;
    historyIndex++;

    return candidate;
  }

  // C+F+G (0,5,7) -> Csus4
  if (pitchClasses.size() == 3 &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 0) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 5) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 7) !=
          pitchClasses.end()) {
    std::string name =
        formatChordName(0, "sus4", bassNote != 0 ? bassNote : -1);
    ChordCandidate candidate = {name, mask, 1.0};

    // 履歴に追加
    chordHistory[historyIndex % MAX_CHORD_HISTORY] = candidate;
    historyIndex++;

    return candidate;
  }

  // F+A#+C (5,10,0) -> Fsus4
  if (pitchClasses.size() == 3 &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 5) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 10) !=
          pitchClasses.end() &&
      std::find(pitchClasses.begin(), pitchClasses.end(), 0) !=
          pitchClasses.end()) {
    std::string name =
        formatChordName(5, "sus4", bassNote != 5 ? bassNote : -1);
    ChordCandidate candidate = {name, mask, 1.0};

    // 履歴に追加
    chordHistory[historyIndex % MAX_CHORD_HISTORY] = candidate;
    historyIndex++;

    return candidate;
  }

  // 各音をルートとして試す（押されている音だけでなく、すべての可能性を試す）
  for (int root = 0; root < 12; ++root) {
    for (const auto &pattern : chordPatterns) {
      uint16_t shifted =
          ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) &
          0xFFF;

      // パターンがマスクに含まれているか確認
      if ((mask & shifted) == shifted) {
        // パターンの音がすべてマスクに含まれているか確認
        bool exactMatch = (mask == shifted);

        // 信頼度を計算
        int patternBits = __builtin_popcount(shifted);
        int maskBits = __builtin_popcount(mask);
        int extraBits = maskBits - patternBits;

        // 信頼度の基本値
        float confidence = pattern.weight;

        // 1. 完全一致の場合、信頼度を大幅に上げる
        if (exactMatch) {
          confidence *= 2.0f;
        }

        // 2. ルートが押されている音に含まれる場合、信頼度を上げる
        bool rootIsPresent = (mask & (1 << root)) != 0;
        if (rootIsPresent) {
          confidence *= 1.2f;
        } else {
          // ルートが押されていない場合、信頼度を大幅に下げる
          confidence *= 0.4f;
        }

        // 3. ルートが最低音と一致する場合、信頼度を上げる
        if (root == bassNote) {
          confidence *= 1.5f;
        }

        // 4. 余分な音が多いほど信頼度が下がる
        confidence *= (1.0f / (1.0f + extraBits * 0.3f));

        // 5. コードタイプごとの特別処理
        if (std::string(pattern.name) == "sus2") {
          // sus2の場合、2度の音が含まれているか確認
          int second = (root + 2) % 12;
          if (mask & (1 << second)) {
            confidence *= 1.5f;
          } else {
            confidence *= 0.3f;
          }
        } else if (std::string(pattern.name) == "sus4") {
          // sus4の場合、4度の音が含まれているか確認
          int fourth = (root + 5) % 12;
          if (mask & (1 << fourth)) {
            confidence *= 1.5f;
          } else {
            confidence *= 0.3f;
          }
        } else if (std::string(pattern.name) == "maj") {
          // メジャーコードの場合、3度と5度の音が含まれているか確認
          int third = (root + 4) % 12;
          int fifth = (root + 7) % 12;
          if ((mask & (1 << third)) && (mask & (1 << fifth))) {
            confidence *= 1.3f;
          }
        } else if (std::string(pattern.name) == "min") {
          // マイナーコードの場合、短3度と5度の音が含まれているか確認
          int flatThird = (root + 3) % 12;
          int fifth = (root + 7) % 12;
          if ((mask & (1 << flatThird)) && (mask & (1 << fifth))) {
            confidence *= 1.3f;
          }
        }

        // 6. 調性に基づく確信度調整
        confidence *= adjustConfidenceByKey(root, pattern.name, currentKey);

        // 7. コード進行パターンに基づく確信度調整
        confidence *= adjustConfidenceByProgression(root, pattern.name);

        // 8. テンションノートの検出
        std::string typeWithTensions = pattern.name;
        analyzeTensions(mask, root, typeWithTensions);

        // コード名を生成
        std::string name = formatChordName(root, typeWithTensions,
                                           root != bassNote ? bassNote : -1);

        // ONコードの評価を改善
        if (detectOnChords && bassNote >= 0) {
          confidence *= evaluateOnChord(root, pattern.name, bassNote, mask);
        }

        candidates.push_back({name, mask, confidence});
      }
    }
  }

  // 候補がない場合
  if (candidates.empty()) {
    ChordCandidate candidate = {"???", mask, 0.0};

    // 履歴に追加
    chordHistory[historyIndex % MAX_CHORD_HISTORY] = candidate;
    historyIndex++;

    return candidate;
  }

  // 最も信頼度の高い候補を選択
  auto bestCandidate = *std::max_element(
      candidates.begin(), candidates.end(),
      [](const auto &a, const auto &b) { return a.confidence < b.confidence; });

  // 履歴に追加
  chordHistory[historyIndex % MAX_CHORD_HISTORY] = bestCandidate;
  historyIndex++;

  return bestCandidate;
}

std::vector<ChordCandidate> Chordlock::detectAlternatives(uint32_t now_ms,
                                                          int maxResults) {
  uint16_t mask = buildChordMask();
  std::vector<ChordCandidate> candidates;

  if (mask == 0) {
    candidates.push_back({"—", 0, 0.0});
    return candidates;
  }

  // すべての可能性を探索
  for (int root = 0; root < 12; ++root) {
    for (const auto &pattern : chordPatterns) {
      uint16_t shifted =
          ((pattern.pattern << root) | (pattern.pattern >> (12 - root))) &
          0xFFF;

      if ((mask & shifted) == shifted) {
        int patternBits = __builtin_popcount(shifted);
        int maskBits = __builtin_popcount(mask);
        int extraBits = maskBits - patternBits;

        // 基本的な信頼度の計算
        float confidence = pattern.weight * (1.0f / (1.0f + extraBits * 0.3f));

        // ルートが押されているかを確認
        bool rootIsPresent = (mask & (1 << root)) != 0;
        if (rootIsPresent) {
          confidence *= 1.2f;
        } else {
          confidence *= 0.4f;
        }

        // 調性に基づく確信度調整
        confidence *= adjustConfidenceByKey(root, pattern.name, currentKey);

        // コード名を生成
        // ベース音を取得
        int bassNote = -1;
        for (int i = 0; i < 128; ++i) {
          if (noteStates[i]) {
            bassNote = i % 12;
            break;
          }
        }

        std::string name = formatChordName(root, pattern.name,
                                           root != bassNote ? bassNote : -1);
        candidates.push_back({name, mask, confidence});
      }
    }
  }

  // 信頼度で降順ソート
  std::sort(
      candidates.begin(), candidates.end(),
      [](const auto &a, const auto &b) { return a.confidence > b.confidence; });

  // 指定された数の結果のみ返す
  if (candidates.size() > maxResults) {
    candidates.resize(maxResults);
  }

  // 候補が見つからない場合
  if (candidates.empty()) {
    candidates.push_back({"???", mask, 0.0});
  }

  return candidates;
}

// 拡張されたコード検出関数
ExtendedChordResult Chordlock::detectExtended(uint32_t now_ms) {
  // 基本的なコード検出を実行
  ChordCandidate bestCandidate = detect(now_ms);

  // 拡張結果の初期化
  ExtendedChordResult result;
  result.name = bestCandidate.name;
  result.mask = bestCandidate.mask;
  result.confidence = bestCandidate.confidence;

  // ルート音とベース音を抽出
  result.root = -1;
  result.bass = -1;

  // コード名からルート音を抽出（例: "C#m7" -> C#）
  static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};
  for (int i = 0; i < 12; i++) {
    if (result.name.find(noteNames[i]) == 0) {
      result.root = i;
      // ベース音を抽出
      size_t slashPos = result.name.find('/');
      if (slashPos != std::string::npos) {
        std::string bassStr = result.name.substr(slashPos + 1);
        for (int j = 0; j < 12; j++) {
          if (bassStr == noteNames[j]) {
            result.bass = j;
            break;
          }
        }
      } else {
        // ベース音が指定されていない場合は、ルート音と同じ
        result.bass = result.root;
      }
      break;
    }
  }

  // 押されている音符を収集
  std::vector<int> pitchClasses;
  for (int i = 0; i < 12; ++i) {
    if (result.mask & (1 << i)) {
      pitchClasses.push_back(i);
      result.notes.push_back(noteName(i));
    }
  }

  // コードと認定されるかの判定
  // 信頼度が閾値以上、かつ少なくとも3つの異なる音が押されている場合
  result.isValidChord = (result.confidence >= 0.6f && pitchClasses.size() >= 3);

  // 代替候補の取得
  result.alternatives = detectAlternatives(now_ms, 4);
  // 最良候補を除外（既に同じ結果がメインで表示されるため）
  if (!result.alternatives.empty() &&
      result.alternatives[0].name == result.name) {
    result.alternatives.erase(result.alternatives.begin());
  }

  return result;
}

// グローバルインスタンス（重要）
static Chordlock chordlock;

// WASMバインディング
extern "C" {
void noteOn(uint8_t note, double time_ms) {
  chordlock.noteOn(note, (uint32_t)time_ms);
}

void noteOff(uint8_t note) { chordlock.noteOff(note); }

void setVelocity(uint8_t note, uint8_t velocity) {
  chordlock.setVelocity(note, velocity);
}

void setVelocitySensitivity(bool enabled) {
  chordlock.setVelocitySensitivity(enabled);
}

void setOnChordDetection(bool enabled) {
  chordlock.setOnChordDetection(enabled);
}

void setKey(uint8_t key) { chordlock.setKey(key); }

void reset() { chordlock.reset(); }

const char *detect(double now_ms) {
  static std::string result;
  auto candidate = chordlock.detect((uint32_t)now_ms);
  result = candidate.name;
  return result.c_str();
}

// detectChord関数を追加（detectのエイリアス）
const char *detectChord(double now_ms) { return detect(now_ms); }

const char *detectWithConfidence(double now_ms, float *confidence_out) {
  static std::string result;
  auto candidate = chordlock.detect((uint32_t)now_ms);
  result = candidate.name;
  *confidence_out = candidate.confidence;
  return result.c_str();
}

int detectAlternatives(double now_ms, char **results, float *confidences,
                       int maxResults) {
  auto candidates = chordlock.detectAlternatives((uint32_t)now_ms, maxResults);

  // 結果を配列にコピー
  static std::vector<std::string> resultStrings;
  resultStrings.resize(candidates.size());

  for (size_t i = 0; i < candidates.size(); i++) {
    resultStrings[i] = candidates[i].name;
    results[i] = const_cast<char *>(resultStrings[i].c_str());
    confidences[i] = candidates[i].confidence;
  }

  return static_cast<int>(candidates.size());
}

// 拡張API
bool detectExtended(double now_ms, char **chordName, bool *isValidChord,
                    char **notes, int *notesCount, char **alternatives,
                    float *altConfidences, int *altCount) {
  static std::string resultName;
  static std::vector<std::string> noteStrings;
  static std::vector<std::string> altStrings;

  auto extResult = chordlock.detectExtended((uint32_t)now_ms);

  // コード名を返す
  resultName = extResult.name;
  *chordName = const_cast<char *>(resultName.c_str());

  // コードと認定されるかのフラグを設定
  *isValidChord = extResult.isValidChord;

  // 構成音を返す
  noteStrings = extResult.notes;
  *notesCount = static_cast<int>(noteStrings.size());
  for (int i = 0; i < *notesCount; i++) {
    notes[i] = const_cast<char *>(noteStrings[i].c_str());
  }

  // 代替候補を返す
  altStrings.resize(extResult.alternatives.size());
  *altCount = static_cast<int>(extResult.alternatives.size());

  for (int i = 0; i < *altCount; i++) {
    altStrings[i] = extResult.alternatives[i].name;
    alternatives[i] = const_cast<char *>(altStrings[i].c_str());
    altConfidences[i] = extResult.alternatives[i].confidence;
  }

  return true;
}

// ルート音とベース音を取得
void getChordRootAndBass(double now_ms, int *root, int *bass) {
  auto extResult = chordlock.detectExtended((uint32_t)now_ms);
  *root = extResult.root;
  *bass = extResult.bass;
}
}
