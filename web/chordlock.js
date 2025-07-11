/**
 * ChordLock JavaScript Implementation
 * Simplified chord identification for web demo
 */

class ChordLock {
    constructor() {
        this.chordPatterns = new Map();
        this.initializeChordDatabase();
    }
    
    initializeChordDatabase() {
        // Basic triads
        this.addChord([0, 4, 7], "Major", "");
        this.addChord([0, 3, 7], "Minor", "m");
        this.addChord([0, 3, 6], "Diminished", "dim");
        this.addChord([0, 4, 8], "Augmented", "aug");
        
        // Suspended chords
        this.addChord([0, 5, 7], "Sus4", "sus4");
        this.addChord([0, 2, 7], "Sus2", "sus2");
        
        // Seventh chords
        this.addChord([0, 4, 7, 10], "Dominant 7th", "7");
        this.addChord([0, 4, 7, 11], "Major 7th", "M7");
        this.addChord([0, 3, 7, 10], "Minor 7th", "m7");
        this.addChord([0, 3, 6, 10], "Minor 7♭5", "m7♭5");
        this.addChord([0, 3, 6, 9], "Diminished 7th", "dim7");
        
        // Extended chords
        this.addChord([0, 4, 7, 14], "Add9", "add9");
        this.addChord([0, 4, 7, 9], "6th", "6");
        this.addChord([0, 3, 7, 9], "Minor 6th", "m6");
        this.addChord([0, 4, 7, 10, 14], "9th", "9");
        this.addChord([0, 4, 7, 11, 14], "Major 9th", "M9");
        this.addChord([0, 3, 7, 10, 14], "Minor 9th", "m9");
        
        // Altered chords
        this.addChord([0, 4, 7, 10, 13], "7♭9", "7♭9");
        this.addChord([0, 4, 7, 10, 15], "7♯9", "7♯9");
        this.addChord([0, 4, 7, 10, 17], "7♯11", "7♯11");
        this.addChord([0, 4, 7, 10, 21], "7♯13", "7♯13");
        
        // Inversions (slash chords)
        this.addInversions();
    }
    
    addChord(intervals, name, symbol) {
        const key = intervals.join(',');
        this.chordPatterns.set(key, { name, symbol, intervals, rootPosition: true });
    }
    
    addInversions() {
        // Add first and second inversions for triads
        const triads = [
            { intervals: [0, 4, 7], symbol: "", name: "Major" },
            { intervals: [0, 3, 7], symbol: "m", name: "Minor" },
            { intervals: [0, 3, 6], symbol: "dim", name: "Diminished" },
            { intervals: [0, 4, 8], symbol: "aug", name: "Augmented" }
        ];
        
        triads.forEach(chord => {
            // First inversion (3rd in bass)
            const firstInv = [0, 3, 8]; // E-G-C for C major
            this.chordPatterns.set(firstInv.join(','), {
                name: chord.name,
                symbol: chord.symbol,
                intervals: firstInv,
                rootPosition: false,
                inversion: 1,
                originalIntervals: chord.intervals
            });
            
            // Second inversion (5th in bass)
            const secondInv = [0, 5, 9]; // G-C-E for C major  
            this.chordPatterns.set(secondInv.join(','), {
                name: chord.name,
                symbol: chord.symbol,
                intervals: secondInv,
                rootPosition: false,
                inversion: 2,
                originalIntervals: chord.intervals
            });
        });
    }
    
    normalizeToSemitones(midiNotes) {
        if (!midiNotes || midiNotes.length === 0) return [];
        
        // Convert to semitone classes and sort
        let semitones = midiNotes.map(note => note % 12);
        semitones = [...new Set(semitones)]; // Remove duplicates
        semitones.sort((a, b) => a - b);
        
        // Convert to intervals starting from lowest note
        const root = semitones[0];
        return semitones.map(note => (note - root + 12) % 12);
    }
    
    midiToNoteName(midiNote) {
        const noteNames = ['C', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B'];
        return noteNames[midiNote % 12];
    }
    
    detectRoot(midiNotes, intervals, chordInfo) {
        if (chordInfo.rootPosition) {
            return midiNotes.reduce((min, note) => note < min ? note : min);
        }
        
        // For inversions, calculate the theoretical root
        const bassNote = midiNotes.reduce((min, note) => note < min ? note : min);
        const bassClass = bassNote % 12;
        
        // Find the root based on inversion
        if (chordInfo.inversion === 1) {
            // First inversion: bass is 3rd, root is 4 semitones down
            return bassNote - 4 + (bassNote - 4 < 0 ? 12 : 0);
        } else if (chordInfo.inversion === 2) {
            // Second inversion: bass is 5th, root is 7 semitones down  
            return bassNote - 7 + (bassNote - 7 < 0 ? 12 : 0);
        }
        
        return bassNote;
    }
    
    identifyChord(midiNotes) {
        const startTime = performance.now();
        
        if (!midiNotes || midiNotes.length === 0) {
            return {
                chordName: "Unknown",
                confidence: 0,
                noteNames: [],
                intervals: [],
                processingTime: 0,
                isSlashChord: false
            };
        }
        
        const intervals = this.normalizeToSemitones(midiNotes);
        const intervalKey = intervals.join(',');
        
        let chordInfo = this.chordPatterns.get(intervalKey);
        let confidence = 1.0;
        
        if (!chordInfo) {
            // Try to find partial match
            confidence = 0.7;
            chordInfo = this.findBestMatch(intervals);
        }
        
        if (!chordInfo) {
            const endTime = performance.now();
            return {
                chordName: "Unknown",
                confidence: 0,
                noteNames: midiNotes.map(note => this.midiToNoteName(note)),
                intervals: intervals,
                processingTime: (endTime - startTime) * 1000,
                isSlashChord: false
            };
        }
        
        // Determine root note
        const rootMidi = this.detectRoot(midiNotes, intervals, chordInfo);
        const rootName = this.midiToNoteName(rootMidi);
        
        // Generate chord name
        let chordName = rootName + chordInfo.symbol;
        const isSlashChord = !chordInfo.rootPosition && chordInfo.inversion;
        
        if (isSlashChord) {
            const bassNote = midiNotes.reduce((min, note) => note < min ? note : min);
            const bassName = this.midiToNoteName(bassNote);
            chordName += `/${bassName}`;
        }
        
        const endTime = performance.now();
        
        return {
            chordName: chordName,
            confidence: confidence,
            noteNames: midiNotes.map(note => this.midiToNoteName(note)),
            intervals: intervals,
            processingTime: (endTime - startTime) * 1000,
            isSlashChord: isSlashChord,
            rootNote: rootName,
            symbol: chordInfo.symbol
        };
    }
    
    findBestMatch(intervals) {
        let bestMatch = null;
        let bestScore = 0;
        
        for (const [key, chord] of this.chordPatterns) {
            const chordIntervals = chord.intervals;
            const commonIntervals = intervals.filter(interval => 
                chordIntervals.includes(interval)
            );
            
            const score = commonIntervals.length / Math.max(intervals.length, chordIntervals.length);
            
            if (score > bestScore && score >= 0.6) {
                bestScore = score;
                bestMatch = chord;
            }
        }
        
        return bestMatch;
    }
}

// Global ChordLock instance
const chordLock = new ChordLock();

// UI Functions
function setNotes(notes) {
    document.getElementById('midi-notes').value = notes;
    identifyChord();
}

function identifyChord() {
    const input = document.getElementById('midi-notes').value;
    
    if (!input.trim()) {
        alert('Please enter MIDI notes (e.g., 60,64,67)');
        return;
    }
    
    try {
        const midiNotes = input.split(',').map(n => parseInt(n.trim())).filter(n => !isNaN(n));
        
        if (midiNotes.length === 0) {
            alert('Please enter valid MIDI note numbers');
            return;
        }
        
        const result = chordLock.identifyChord(midiNotes);
        
        // Display results
        document.getElementById('chord-name').textContent = result.chordName;
        document.getElementById('input-notes').textContent = midiNotes.join(', ');
        document.getElementById('note-names').textContent = result.noteNames.join(', ');
        document.getElementById('intervals').textContent = result.intervals.join(', ');
        document.getElementById('confidence').textContent = Math.round(result.confidence * 100) + '%';
        document.getElementById('processing-time').textContent = result.processingTime.toFixed(3) + 'ms';
        
        document.getElementById('result').style.display = 'block';
        
    } catch (error) {
        alert('Error identifying chord: ' + error.message);
    }
}

function clearResult() {
    document.getElementById('midi-notes').value = '';
    document.getElementById('result').style.display = 'none';
}

// Auto-identify on page load
window.addEventListener('load', () => {
    identifyChord();
});