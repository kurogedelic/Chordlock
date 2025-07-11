/**
 * ChordLock JavaScript Implementation
 * Enhanced with Web MIDI API and Virtual Piano
 */

class ChordLock {
    constructor() {
        this.chordPatterns = new Map();
        this.initializeChordDatabase();
        
        // MIDI variables
        this.midiAccess = null;
        this.activeNotes = new Set();
        this.sustainTimeout = null;
        
        // Piano variables
        this.pianoKeys = new Map();
        this.currentOctave = 4;
        
        this.initializePiano();
        this.checkMIDISupport();
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
        const triads = [
            { intervals: [0, 4, 7], symbol: "", name: "Major" },
            { intervals: [0, 3, 7], symbol: "m", name: "Minor" },
            { intervals: [0, 3, 6], symbol: "dim", name: "Diminished" },
            { intervals: [0, 4, 8], symbol: "aug", name: "Augmented" }
        ];
        
        triads.forEach(chord => {
            // First inversion
            const firstInv = [0, 3, 8];
            this.chordPatterns.set(firstInv.join(','), {
                name: chord.name,
                symbol: chord.symbol,
                intervals: firstInv,
                rootPosition: false,
                inversion: 1,
                originalIntervals: chord.intervals
            });
            
            // Second inversion
            const secondInv = [0, 5, 9];
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
    
    // Piano functionality
    initializePiano() {
        const piano = document.getElementById('piano');
        if (!piano) return;
        
        const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const whiteKeys = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
        const blackKeys = ['C#', 'D#', 'F#', 'G#', 'A#'];
        
        // Create white keys first
        whiteKeys.forEach((note, index) => {
            const key = document.createElement('div');
            const midiNote = this.currentOctave * 12 + notes.indexOf(note);
            
            key.className = 'piano-key white';
            key.textContent = note;
            key.dataset.note = midiNote;
            
            key.addEventListener('mousedown', () => this.playNote(midiNote));
            key.addEventListener('mouseup', () => this.stopNote(midiNote));
            key.addEventListener('mouseleave', () => this.stopNote(midiNote));
            
            piano.appendChild(key);
            this.pianoKeys.set(midiNote, key);
        });
        
        // Create black keys and position them
        const blackKeyPositions = [0.5, 1.5, 3.5, 4.5, 5.5]; // Positions between white keys
        blackKeys.forEach((note, index) => {
            const key = document.createElement('div');
            const midiNote = this.currentOctave * 12 + notes.indexOf(note);
            
            key.className = 'piano-key black';
            key.textContent = note;
            key.dataset.note = midiNote;
            
            key.addEventListener('mousedown', () => this.playNote(midiNote));
            key.addEventListener('mouseup', () => this.stopNote(midiNote));
            key.addEventListener('mouseleave', () => this.stopNote(midiNote));
            
            // Position black key between white keys
            key.style.position = 'absolute';
            key.style.left = `${blackKeyPositions[index] * 42 + 20}px`;
            
            piano.appendChild(key);
            this.pianoKeys.set(midiNote, key);
        });
        
        // Set piano container position relative for black key positioning
        piano.style.position = 'relative';
    }
    
    playNote(midiNote) {
        this.activeNotes.add(midiNote);
        this.updatePianoDisplay();
        this.updateActiveNotes();
        this.scheduleChordIdentification();
    }
    
    stopNote(midiNote) {
        this.activeNotes.delete(midiNote);
        this.updatePianoDisplay();
        this.updateActiveNotes();
        this.scheduleChordIdentification();
    }
    
    updatePianoDisplay() {
        this.pianoKeys.forEach((key, midiNote) => {
            if (this.activeNotes.has(midiNote)) {
                key.classList.add('active');
            } else {
                key.classList.remove('active');
            }
        });
    }
    
    updateActiveNotes() {
        const activeNotesDiv = document.getElementById('active-notes');
        if (!activeNotesDiv) return;
        
        activeNotesDiv.innerHTML = '';
        
        if (this.activeNotes.size === 0) {
            const placeholder = document.createElement('span');
            placeholder.style.opacity = '0.6';
            placeholder.style.fontStyle = 'italic';
            placeholder.textContent = 'Click piano keys or connect MIDI device...';
            activeNotesDiv.appendChild(placeholder);
            return;
        }
        
        const sortedNotes = Array.from(this.activeNotes).sort((a, b) => a - b);
        
        sortedNotes.forEach(note => {
            const noteSpan = document.createElement('span');
            noteSpan.className = 'active-note';
            noteSpan.textContent = `${this.midiToNoteName(note)} (${note})`;
            activeNotesDiv.appendChild(noteSpan);
        });
    }
    
    scheduleChordIdentification() {
        if (this.sustainTimeout) {
            clearTimeout(this.sustainTimeout);
        }
        
        const autoIdentify = document.getElementById('auto-identify')?.checked;
        if (!autoIdentify || this.activeNotes.size < 2) {
            return;
        }
        
        this.sustainTimeout = setTimeout(() => {
            if (this.activeNotes.size >= 2) {
                const noteArray = Array.from(this.activeNotes);
                document.getElementById('midi-notes').value = noteArray.join(',');
                identifyChord();
            }
        }, 300);
    }
    
    // MIDI functionality
    async checkMIDISupport() {
        const supportElement = document.getElementById('midi-support');
        if (navigator.requestMIDIAccess) {
            supportElement.textContent = 'Available';
            supportElement.style.color = 'var(--success-color)';
        } else {
            supportElement.textContent = 'Not supported';
            supportElement.style.color = 'var(--error-color)';
        }
    }
    
    async connectMIDI() {
        if (!navigator.requestMIDIAccess) {
            alert('Web MIDI API not supported in this browser. Please use Chrome, Edge, or Opera.');
            return;
        }
        
        try {
            document.getElementById('midi-status-text').textContent = 'Connecting...';
            this.midiAccess = await navigator.requestMIDIAccess();
            
            this.setupMIDI();
            document.getElementById('midi-indicator').classList.add('connected');
            document.getElementById('midi-status-text').textContent = 'MIDI Connected';
            document.getElementById('midi-connect-btn').textContent = 'Disconnect';
            document.getElementById('midi-connect-btn').onclick = () => this.disconnectMIDI();
            
            console.log('MIDI access granted');
            
        } catch (error) {
            console.error('Failed to access MIDI:', error);
            document.getElementById('midi-status-text').textContent = 'MIDI Failed';
            alert('Failed to access MIDI devices. Please ensure your MIDI device is connected.');
        }
    }
    
    setupMIDI() {
        if (!this.midiAccess) return;
        
        for (const input of this.midiAccess.inputs.values()) {
            console.log('MIDI Input:', input.name);
            input.onmidimessage = (event) => this.handleMIDIMessage(event);
        }
        
        this.midiAccess.onstatechange = (event) => {
            console.log('MIDI device state changed:', event.port.name, event.port.state);
        };
    }
    
    handleMIDIMessage(event) {
        const [command, note, velocity] = event.data;
        
        const isNoteOn = (command & 0xF0) === 0x90 && velocity > 0;
        const isNoteOff = (command & 0xF0) === 0x80 || ((command & 0xF0) === 0x90 && velocity === 0);
        
        if (isNoteOn) {
            this.playNote(note);
        } else if (isNoteOff) {
            this.stopNote(note);
        }
    }
    
    disconnectMIDI() {
        if (this.midiAccess) {
            for (const input of this.midiAccess.inputs.values()) {
                input.onmidimessage = null;
            }
            this.midiAccess = null;
        }
        
        this.activeNotes.clear();
        this.updatePianoDisplay();
        this.updateActiveNotes();
        
        document.getElementById('midi-indicator').classList.remove('connected');
        document.getElementById('midi-status-text').textContent = 'MIDI Disconnected';
        document.getElementById('midi-connect-btn').textContent = 'Connect MIDI';
        document.getElementById('midi-connect-btn').onclick = () => this.connectMIDI();
    }
    
    // Chord identification
    normalizeToSemitones(midiNotes) {
        if (!midiNotes || midiNotes.length === 0) return [];
        
        let semitones = midiNotes.map(note => note % 12);
        semitones = [...new Set(semitones)];
        semitones.sort((a, b) => a - b);
        
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
        
        const bassNote = midiNotes.reduce((min, note) => note < min ? note : min);
        
        if (chordInfo.inversion === 1) {
            return bassNote - 4 + (bassNote - 4 < 0 ? 12 : 0);
        } else if (chordInfo.inversion === 2) {
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
        
        const rootMidi = this.detectRoot(midiNotes, intervals, chordInfo);
        const rootName = this.midiToNoteName(rootMidi);
        
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
        document.getElementById('note-names').textContent = result.noteNames.join(', ');
        document.getElementById('intervals').textContent = result.intervals.join(', ');
        document.getElementById('confidence').textContent = Math.round(result.confidence * 100) + '%';
        document.getElementById('processing-time').textContent = result.processingTime.toFixed(3) + 'ms';
        
        document.getElementById('chord-result').style.display = 'block';
        
    } catch (error) {
        alert('Error identifying chord: ' + error.message);
    }
}

function clearAll() {
    document.getElementById('midi-notes').value = '';
    document.getElementById('chord-result').style.display = 'none';
    chordLock.activeNotes.clear();
    chordLock.updatePianoDisplay();
    chordLock.updateActiveNotes();
}

function connectMIDI() {
    chordLock.connectMIDI();
}

// Initialize on page load
window.addEventListener('load', () => {
    // Auto-identify default chord
    identifyChord();
    
    // Handle octave selection
    const octaveSelect = document.getElementById('octave-select');
    if (octaveSelect) {
        octaveSelect.addEventListener('change', (e) => {
            chordLock.currentOctave = parseInt(e.target.value);
            chordLock.initializePiano();
        });
    }
});