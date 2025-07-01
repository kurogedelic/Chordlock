#!/usr/bin/env python3

# 正確な音楽理論分析
note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

test_cases = [
    ('65,69,72', 'F'),  # F(5) + A(9) + C(0) 
    ('65,72', 'F5'),    # F(5) + C(0)
    ('69,72,75', 'Am'), # A(9) + C(0) + Eb(3)
    ('67,71,74', 'G'),  # G(7) + B(11) + D(2)
]

print("🎵 正確な音楽理論分析")
print("=" * 30)

for notes_str, expected in test_cases:
    notes = [int(n) for n in notes_str.split(',')]
    pitches = [note % 12 for note in notes]
    print(f'{notes_str} # {expected}')
    print(f'  MIDI notes: {notes}')
    print(f'  Pitch classes: {pitches}')
    print(f'  Note names: {[note_names[p] for p in pitches]}')
    
    if len(pitches) == 2:
        # Power chord analysis
        p1, p2 = sorted(pitches)
        interval = (p2 - p1) % 12
        print(f'  Interval: {interval} semitones')
        
        if interval == 7:
            root = note_names[p1]
            print(f'  → {root}5 (perfect 5th) ✅')
        else:
            print(f'  → Not a perfect 5th ❌')
    
    elif len(pitches) == 3:
        # Triad analysis - handle octave wrapping
        sorted_p = sorted(pitches)
        
        # Try different roots to find the best fit
        best_root = None
        best_type = None
        
        for root_pitch in pitches:
            others = [p for p in pitches if p != root_pitch]
            intervals = [(p - root_pitch) % 12 for p in others]
            intervals.sort()
            
            if len(intervals) == 2:
                third, fifth = intervals
                if third == 4 and fifth == 7:
                    best_root = root_pitch
                    best_type = "major"
                    break
                elif third == 3 and fifth == 7:
                    best_root = root_pitch  
                    best_type = "minor"
                    break
        
        if best_root is not None:
            root_name = note_names[best_root]
            chord_name = root_name + ("m" if best_type == "minor" else "")
            print(f'  → {chord_name} ({best_type}) ✅')
            
            if chord_name == expected:
                print(f'  ✅ テストケース正確')
            else:
                print(f'  ❌ テストケースエラー: 期待値 {expected}, 実際 {chord_name}')
        else:
            print(f'  → Unknown chord pattern ❌')
    
    print()