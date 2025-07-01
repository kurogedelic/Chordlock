#!/usr/bin/env python3

# Validate test_set.txt for music theory correctness

note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

def analyze_chord(notes_str, expected):
    """Analyze a chord and return the correct name based on music theory"""
    notes = [int(n) for n in notes_str.split(',')]
    pitches = [note % 12 for note in notes]
    
    # Don't sort - use lowest pitch as root
    root = min(pitches)
    
    if len(pitches) == 2:
        # Power chord (root + fifth)
        other = [p for p in pitches if p != root][0]
        interval = (other - root) % 12
        if interval == 7:
            return note_names[root] + '5'
        else:
            return f'Unknown interval: {interval}'
    
    elif len(pitches) == 3:
        # Triad analysis - find intervals from root
        others = [p for p in pitches if p != root]
        intervals = [(p - root) % 12 for p in others]
        intervals.sort()
        third_interval, fifth_interval = intervals
        
        if third_interval == 4 and fifth_interval == 7:  # Major triad
            return note_names[root]
        elif third_interval == 3 and fifth_interval == 7:  # Minor triad  
            return note_names[root] + 'm'
        elif third_interval == 4 and fifth_interval == 8:  # Augmented
            return note_names[root] + 'aug'
        elif third_interval == 3 and fifth_interval == 6:  # Diminished
            return note_names[root] + 'dim'
        elif third_interval == 2 and fifth_interval == 7:  # sus2
            return note_names[root] + 'sus2'
        elif third_interval == 5 and fifth_interval == 7:  # sus4
            return note_names[root] + 'sus4'
        else:
            return f'Unknown triad: ({third_interval}, {fifth_interval})'
    
    else:
        # Complex chord - just return expected for now
        return expected

# Test cases from test_set.txt that are basic triads/power chords
test_cases = [
    ('60,64,67', 'C'),      # C major
    ('60,63,67', 'Cm'),     # C minor
    ('60,62,67', 'Csus2'),  # C sus2
    ('60,65,67', 'Csus4'),  # C sus4
    ('60,67', 'C5'),        # C power chord
    ('62,65,69', 'D'),      # D major
    ('62,65,68', 'Dm'),     # D minor  
    ('62,64,69', 'Dsus2'),  # D sus2
    ('62,65,70', 'Dsus4'),  # D sus4
    ('62,69', 'D5'),        # D power chord
    ('64,67,71', 'E'),      # CHECKING: E + G + B
    ('64,68,71', 'E'),      # E major should be E + G# + B  
    ('65,69,72', 'F'),      # F major
    ('65,68,71', 'Fm'),     # F minor
    ('67,71,74', 'G'),      # G major
    ('67,70,74', 'Gm'),     # G minor
    ('69,72,76', 'A'),      # CHECKING: A + C + E
    ('69,73,76', 'A'),      # A major should be A + C# + E
    ('69,72,75', 'Am'),     # A minor
]

print("🔍 Validating test_set.txt for music theory correctness")
print("=" * 55)

errors = []
for notes_str, expected in test_cases:
    actual = analyze_chord(notes_str, expected)
    if actual != expected:
        errors.append(f'{notes_str} -> Expected: {expected}, Should be: {actual}')
        notes = [int(n) for n in notes_str.split(',')]
        pitches = [note % 12 for note in notes]
        pitch_names = [note_names[p] for p in pitches]
        print(f'❌ {notes_str} ({pitch_names}): Expected "{expected}", Should be "{actual}"')
    else:
        print(f'✅ {notes_str}: {expected}')

print(f"\n📊 Summary: {len(errors)} errors found")
if errors:
    print("\n🚨 CRITICAL: test_set.txt contains music theory errors!")
    print("These test cases have wrong expected results:")
    for error in errors:
        print(f"  {error}")