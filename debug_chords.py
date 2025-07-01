#!/usr/bin/env python3

note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

def debug_chord(notes_str):
    notes = [int(n) for n in notes_str.split(',')]
    pitches = [note % 12 for note in notes]
    print(f'{notes_str}: notes={notes}, pitches={pitches}')
    print(f'  Pitch names: {[note_names[p] for p in pitches]}')
    
    if len(pitches) == 3:
        root = min(pitches)
        others = [p for p in pitches if p != root]
        intervals = [(p - root) % 12 for p in others]
        print(f'  Root: {note_names[root]}, intervals: {intervals}')
        
        # Major triad: 1 - 3 - 5 (intervals 4, 7)
        # Minor triad: 1 - b3 - 5 (intervals 3, 7)
        intervals.sort()
        third, fifth = intervals
        if third == 3 and fifth == 7:
            chord_type = "minor"
        elif third == 4 and fifth == 7:
            chord_type = "major"
        else:
            chord_type = f"unknown ({third}, {fifth})"
        print(f'  -> {note_names[root]} {chord_type}')

# Debug problem cases
print("Debugging chord interval calculations:")
debug_chord('62,65,69')  # D + F + A - expected D major, but F-D = 3 semitones (minor 3rd)
debug_chord('65,69,72')  # F + A + C - expected F major, but A-F = 4 semitones (major 3rd), C-F = 7
debug_chord('67,71,74')  # G + B + D - expected G major, but B-G = 4 semitones, D-G = 7
debug_chord('64,67,71')  # E + G + B - expected E, but G-E = 3 semitones (minor)
debug_chord('69,72,76')  # A + C + E - expected A, but C-A = 3 semitones (minor)