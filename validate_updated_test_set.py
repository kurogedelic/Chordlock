#!/usr/bin/env python3

# 更新されたtest_set.txtの音楽理論的正確性を検証

note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

def analyze_chord_theory(notes_str, expected):
    """音楽理論に基づいて和音を分析"""
    notes = [int(n) for n in notes_str.split(',')]
    pitches = [note % 12 for note in notes]
    
    # 最低音をベースとして分析
    bass = min(pitches)
    upper_pitches = [p for p in pitches if p != bass] if len(pitches) > 1 else []
    
    print(f"🎵 {notes_str} # {expected}")
    print(f"   音符: {[note_names[p] for p in pitches]}")
    
    if len(pitches) == 1:
        return f"{note_names[bass]} (単音)", True
    
    elif len(pitches) == 2:
        # パワーコード
        interval = (max(pitches) - bass) % 12
        if interval == 7:
            theory_result = f"{note_names[bass]}5"
            correct = (expected == theory_result)
            print(f"   理論: {theory_result} (完全5度)")
            return theory_result, correct
        else:
            theory_result = f"{note_names[bass]} + {interval}半音"
            print(f"   理論: {theory_result}")
            return theory_result, False
    
    elif len(pitches) == 3:
        # 三和音分析
        sorted_pitches = sorted(pitches)
        intervals = [(sorted_pitches[i] - sorted_pitches[0]) % 12 for i in range(1, 3)]
        
        third, fifth = intervals
        
        # 基本三和音パターン
        if third == 4 and fifth == 7:
            theory_result = note_names[sorted_pitches[0]]  # Major
        elif third == 3 and fifth == 7:
            theory_result = note_names[sorted_pitches[0]] + "m"  # Minor
        elif third == 4 and fifth == 8:
            theory_result = note_names[sorted_pitches[0]] + "aug"  # Augmented
        elif third == 3 and fifth == 6:
            theory_result = note_names[sorted_pitches[0]] + "dim"  # Diminished
        elif third == 2 and fifth == 7:
            theory_result = note_names[sorted_pitches[0]] + "sus2"  # sus2
        elif third == 5 and fifth == 7:
            theory_result = note_names[sorted_pitches[0]] + "sus4"  # sus4
        else:
            theory_result = f"{note_names[sorted_pitches[0]]} (間隔: {third}, {fifth})"
        
        # ベース音が違う場合はスラッシュコード
        if bass != sorted_pitches[0]:
            base_chord = theory_result
            theory_result = f"{base_chord}/{note_names[bass]}"
        
        correct = (expected == theory_result)
        print(f"   理論: {theory_result} (間隔: {third}, {fifth})")
        return theory_result, correct
    
    elif len(pitches) == 4:
        # 4音和音 (7thコードなど)
        sorted_pitches = sorted(pitches)
        intervals = [(sorted_pitches[i] - sorted_pitches[0]) % 12 for i in range(1, 4)]
        
        third, fifth, seventh = intervals
        
        # 基本的な4音和音パターン
        if third == 4 and fifth == 7 and seventh == 11:
            theory_result = note_names[sorted_pitches[0]] + "maj7"
        elif third == 4 and fifth == 7 and seventh == 10:
            theory_result = note_names[sorted_pitches[0]] + "7"
        elif third == 3 and fifth == 7 and seventh == 10:
            theory_result = note_names[sorted_pitches[0]] + "m7"
        elif third == 4 and fifth == 7 and seventh == 9:
            theory_result = note_names[sorted_pitches[0]] + "6"
        elif third == 4 and fifth == 7 and seventh == 2:  # add9 (2 = 14半音 mod 12)
            theory_result = note_names[sorted_pitches[0]] + "add9"
        elif third == 4 and fifth == 7 and seventh == 5:  # add11 (5 = 17半音 mod 12)
            theory_result = note_names[sorted_pitches[0]] + "add11"
        else:
            theory_result = f"{note_names[sorted_pitches[0]]} (間隔: {third}, {fifth}, {seventh})"
        
        # ベース音チェック
        if bass != sorted_pitches[0]:
            base_chord = theory_result
            theory_result = f"{base_chord}/{note_names[bass]}"
        
        correct = (expected == theory_result)
        print(f"   理論: {theory_result} (間隔: {third}, {fifth}, {seventh})")
        return theory_result, correct
    
    else:
        print(f"   理論: 複雑な和音 ({len(pitches)}音)")
        return f"複雑な和音", True

# test_set.txtを読み込んで検証
print("🔍 test_set.txt 音楽理論検証")
print("=" * 50)

errors = []
total = 0

with open('test_set.txt', 'r') as f:
    for line_num, line in enumerate(f, 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        
        if '#' in line:
            notes_part, expected_part = line.split('#', 1)
            notes_str = notes_part.strip()
            expected = expected_part.strip()
            
            if notes_str and expected:
                total += 1
                theory_result, is_correct = analyze_chord_theory(notes_str, expected)
                
                if not is_correct:
                    errors.append({
                        'line': line_num,
                        'notes': notes_str,
                        'expected': expected,
                        'theory': theory_result
                    })
                    print("   ❌ 音楽理論エラー")
                else:
                    print("   ✅ 正確")
                print()

print(f"📊 検証結果")
print(f"総テスト数: {total}")
print(f"正確: {total - len(errors)} ({((total - len(errors))/total*100):.1f}%)")
print(f"エラー: {len(errors)} ({(len(errors)/total*100):.1f}%)")

if errors:
    print(f"\n🚨 発見されたエラー:")
    for error in errors:
        print(f"  行{error['line']}: {error['notes']} # {error['expected']}")
        print(f"    → 正しくは: {error['theory']}")