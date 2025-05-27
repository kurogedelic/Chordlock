#pragma once
#include "chord_definitions.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct ChordCandidate {
  std::string name;
  uint16_t mask;
  float confidence; // 0.0-1.0の信頼度
};

// 拡張されたコード検出結果の構造体
struct ExtendedChordResult {
  std::string name;                         // コード名
  uint16_t mask;                            // 音符マスク
  float confidence;                         // 信頼度 (0.0-1.0)
  bool isValidChord;                        // 有効なコードと認識されたか
  std::vector<std::string> notes;           // 構成音の名前
  std::vector<ChordCandidate> alternatives; // 代替候補
  int root;                                 // ルート音 (0-11, C=0)
  int bass;                                 // ベース音 (0-11, C=0)
};

class Chordlock {
public:
  Chordlock();
  void noteOn(uint8_t note, uint32_t time_ms);
  void noteOff(uint8_t note);
  ChordCandidate detect(uint32_t now_ms);

  // 追加機能
  std::vector<ChordCandidate> detectAlternatives(uint32_t now_ms,
                                                 int maxResults = 3);
  void setVelocitySensitivity(bool enabled) { velocitySensitive = enabled; }
  void setVelocity(uint8_t note, uint8_t velocity);
  void reset(); // すべての状態をリセット

  // 拡張されたコード検出関数
  ExtendedChordResult detectExtended(uint32_t now_ms);

  // コンテキスト考慮のための機能
  void setKey(int key) { currentKey = key; } // 現在の調を設定
  int getKey() const { return currentKey; }

  // ONコード検知の設定
  void setOnChordDetection(bool enabled) { detectOnChords = enabled; }
  bool getOnChordDetection() const { return detectOnChords; }

  // コード表記の改善
  std::string formatChordName(int root, const std::string &type,
                          int bass = -1);
  std::string noteName(int n);

private:
  bool noteStates[128] = {false};
  uint32_t lastOnTime[128] = {0};
  uint8_t velocities[128] = {0};  // 各ノートのベロシティ
  bool velocitySensitive = false; // ベロシティを考慮するかどうか
  bool detectOnChords = true;    // ONコードを検知するかどうか
  int currentKey = 0;             // 現在の調 (0=C, 1=C#, etc.)

  // 最近検出されたコード履歴（コンテキスト考慮用）
  static const int MAX_CHORD_HISTORY = 8;
  ChordCandidate chordHistory[MAX_CHORD_HISTORY];
  int historyIndex = 0;

  uint16_t buildChordMask();

  std::vector<ChordPattern> chordPatterns;
  void initializeChordPatterns();

  // 調性に基づく確信度調整
  float adjustConfidenceByKey(int root, const std::string &type, int key);

  // コード進行の一般的なパターンに基づく確信度調整
  float adjustConfidenceByProgression(int root, const std::string &type);
};
