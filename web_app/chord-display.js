/**
 * Chord Display - Real-time chord detection and display management
 */
class ChordDisplay {
    constructor() {
        // DOM elements
        this.currentChordElement = document.getElementById('currentChord');
        this.confidenceElement = document.getElementById('confidence');
        this.candidatesListElement = document.getElementById('candidatesList');
        this.pianoCanvas = document.getElementById('pianoCanvas');
        this.pianoCtx = this.pianoCanvas.getContext('2d');
        
        // Piano visualization settings
        this.setupPianoCanvas();
        
        // Chord detection state
        this.currentChord = null;
        this.currentConfidence = 0;
        this.candidates = [];
        this.activeNotes = [];
        
        // Visual settings
        this.whiteKeyColor = '#ffffff';
        this.blackKeyColor = '#333333';
        this.activeWhiteKeyColor = '#667eea';
        this.activeBlackKeyColor = '#5a6fd8';
        this.keyBorderColor = '#cccccc';
        
        // Piano layout
        this.whiteKeys = [0, 2, 4, 5, 7, 9, 11]; // C, D, E, F, G, A, B
        this.blackKeys = [1, 3, 6, 8, 10]; // C#, D#, F#, G#, A#
        this.noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        
        // Initialize display
        this.clear();
        this.drawPiano();
    }

    setupPianoCanvas() {
        // Set up high DPI display support
        const dpr = window.devicePixelRatio || 1;
        const rect = this.pianoCanvas.getBoundingClientRect();
        
        this.pianoCanvas.width = rect.width * dpr;
        this.pianoCanvas.height = rect.height * dpr;
        this.pianoCanvas.style.width = rect.width + 'px';
        this.pianoCanvas.style.height = rect.height + 'px';
        
        this.pianoCtx.scale(dpr, dpr);
        this.pianoWidth = rect.width;
        this.pianoHeight = rect.height;
        
        // Piano dimensions
        this.whiteKeyWidth = this.pianoWidth / (7 * 3); // 3 octaves
        this.whiteKeyHeight = this.pianoHeight - 20;
        this.blackKeyWidth = this.whiteKeyWidth * 0.6;
        this.blackKeyHeight = this.whiteKeyHeight * 0.6;
    }

    clear() {
        this.currentChordElement.textContent = '—';
        this.confidenceElement.textContent = '—';
        this.candidatesListElement.innerHTML = `
            <div class="candidate-item">
                <span class="candidate-name">No audio detected</span>
                <span class="candidate-score">—</span>
            </div>
        `;
        this.activeNotes = [];
        this.drawPiano();
    }

    updateChordDetection(activeNotes, chordlockInstance) {
        this.activeNotes = activeNotes;
        
        if (activeNotes.length === 0) {
            this.clear();
            return;
        }

        try {
            // Clear previous notes in Chordlock
            chordlockInstance.reset();
            
            // Send active notes to Chordlock
            activeNotes.forEach(note => {
                chordlockInstance.noteOn(note.midiNote, note.velocity);
            });
            
            // Get chord detection results
            const chordResult = chordlockInstance.detectChord();
            const detailedResults = chordlockInstance.detectChordDetailed(5);
            
            // Update display
            this.updateChordDisplay(chordResult, detailedResults);
            this.updateCandidatesList(detailedResults);
            this.drawPiano();
            
        } catch (error) {
            console.error('Chord detection error:', error);
            this.displayError('Detection Error');
        }
    }

    updateChordDisplay(chordResult, detailedResults) {
        if (chordResult && chordResult.chordName && chordResult.chordName !== 'Unknown') {
            this.currentChord = chordResult.chordName;
            this.currentConfidence = chordResult.confidence || 0;
            
            this.currentChordElement.textContent = this.currentChord;
            this.confidenceElement.textContent = `${(this.currentConfidence * 100).toFixed(1)}%`;
            
            // Add visual feedback for confidence level
            const confidenceClass = this.getConfidenceClass(this.currentConfidence);
            this.currentChordElement.className = `chord-name ${confidenceClass}`;
            
        } else if (this.activeNotes.length === 1) {
            // Single note
            const note = this.activeNotes[0];
            this.currentChord = note.noteName;
            this.currentConfidence = note.strength;
            
            this.currentChordElement.textContent = note.noteName;
            this.confidenceElement.textContent = `${(note.strength * 100).toFixed(1)}%`;
            this.currentChordElement.className = 'chord-name single-note';
            
        } else {
            this.currentChord = null;
            this.currentConfidence = 0;
            this.currentChordElement.textContent = '?';
            this.confidenceElement.textContent = '—';
            this.currentChordElement.className = 'chord-name unknown';
        }
    }

    updateCandidatesList(detailedResults) {
        if (!detailedResults || detailedResults.length === 0) {
            this.candidatesListElement.innerHTML = `
                <div class="candidate-item">
                    <span class="candidate-name">No candidates</span>
                    <span class="candidate-score">—</span>
                </div>
            `;
            return;
        }

        // Parse detailed results (assuming it's a formatted string)
        const candidates = this.parseDetailedResults(detailedResults);
        
        this.candidatesListElement.innerHTML = candidates.map((candidate, index) => `
            <div class="candidate-item ${index === 0 ? 'top-candidate' : ''}">
                <span class="candidate-name">${candidate.name}</span>
                <span class="candidate-score">${candidate.score}</span>
            </div>
        `.trim()).join('');
    }

    parseDetailedResults(detailedResults) {
        // Parse the detailed results string format
        const candidates = [];
        const lines = detailedResults.split('\n');
        
        lines.forEach(line => {
            const match = line.match(/^(\d+)\.\s*(.+?)\s*\(([^)]+)\)$/);
            if (match) {
                const [, rank, chordName, scoreInfo] = match;
                candidates.push({
                    rank: parseInt(rank),
                    name: chordName.trim(),
                    score: scoreInfo.trim()
                });
            }
        });
        
        return candidates.slice(0, 5); // Top 5 candidates
    }

    getConfidenceClass(confidence) {
        if (confidence >= 0.8) return 'high-confidence';
        if (confidence >= 0.6) return 'medium-confidence';
        if (confidence >= 0.4) return 'low-confidence';
        return 'very-low-confidence';
    }

    drawPiano() {
        // Clear canvas
        this.pianoCtx.clearRect(0, 0, this.pianoWidth, this.pianoHeight);
        
        // Draw white keys first
        this.drawWhiteKeys();
        
        // Draw black keys on top
        this.drawBlackKeys();
        
        // Draw note labels
        this.drawNoteLabels();
    }

    drawWhiteKeys() {
        const octaves = 3;
        const startOctave = 3; // C3 to C6
        
        for (let octave = 0; octave < octaves; octave++) {
            for (let i = 0; i < this.whiteKeys.length; i++) {
                const keyIndex = octave * this.whiteKeys.length + i;
                const x = keyIndex * this.whiteKeyWidth;
                const y = 10;
                
                const pitchClass = this.whiteKeys[i];
                const midiNote = (startOctave + octave) * 12 + pitchClass;
                const isActive = this.isNoteActive(midiNote);
                
                // Key color
                this.pianoCtx.fillStyle = isActive ? this.activeWhiteKeyColor : this.whiteKeyColor;
                this.pianoCtx.fillRect(x, y, this.whiteKeyWidth - 1, this.whiteKeyHeight);
                
                // Key border
                this.pianoCtx.strokeStyle = this.keyBorderColor;
                this.pianoCtx.lineWidth = 1;
                this.pianoCtx.strokeRect(x, y, this.whiteKeyWidth - 1, this.whiteKeyHeight);
                
                // Active indicator (dot)
                if (isActive) {
                    this.pianoCtx.fillStyle = '#ffffff';
                    this.pianoCtx.beginPath();
                    this.pianoCtx.arc(
                        x + this.whiteKeyWidth / 2,
                        y + this.whiteKeyHeight - 20,
                        4,
                        0,
                        2 * Math.PI
                    );
                    this.pianoCtx.fill();
                }
            }
        }
    }

    drawBlackKeys() {
        const octaves = 3;
        const startOctave = 3;
        const blackKeyPositions = [0.7, 1.7, 3.7, 4.7, 5.7]; // Relative positions within octave
        
        for (let octave = 0; octave < octaves; octave++) {
            for (let i = 0; i < this.blackKeys.length; i++) {
                const x = (octave * 7 + blackKeyPositions[i]) * this.whiteKeyWidth - this.blackKeyWidth / 2;
                const y = 10;
                
                const pitchClass = this.blackKeys[i];
                const midiNote = (startOctave + octave) * 12 + pitchClass;
                const isActive = this.isNoteActive(midiNote);
                
                // Key color
                this.pianoCtx.fillStyle = isActive ? this.activeBlackKeyColor : this.blackKeyColor;
                this.pianoCtx.fillRect(x, y, this.blackKeyWidth, this.blackKeyHeight);
                
                // Key border
                this.pianoCtx.strokeStyle = this.keyBorderColor;
                this.pianoCtx.lineWidth = 1;
                this.pianoCtx.strokeRect(x, y, this.blackKeyWidth, this.blackKeyHeight);
                
                // Active indicator (white dot)
                if (isActive) {
                    this.pianoCtx.fillStyle = '#ffffff';
                    this.pianoCtx.beginPath();
                    this.pianoCtx.arc(
                        x + this.blackKeyWidth / 2,
                        y + this.blackKeyHeight - 15,
                        3,
                        0,
                        2 * Math.PI
                    );
                    this.pianoCtx.fill();
                }
            }
        }
    }

    drawNoteLabels() {
        const octaves = 3;
        const startOctave = 3;
        
        this.pianoCtx.fillStyle = '#666666';
        this.pianoCtx.font = '10px sans-serif';
        this.pianoCtx.textAlign = 'center';
        
        for (let octave = 0; octave < octaves; octave++) {
            for (let i = 0; i < this.whiteKeys.length; i++) {
                const keyIndex = octave * this.whiteKeys.length + i;
                const x = keyIndex * this.whiteKeyWidth + this.whiteKeyWidth / 2;
                const y = 10 + this.whiteKeyHeight - 5;
                
                const pitchClass = this.whiteKeys[i];
                const octaveNum = startOctave + octave;
                const noteName = this.noteNames[pitchClass] + octaveNum;
                
                this.pianoCtx.fillText(noteName, x, y);
            }
        }
    }

    isNoteActive(midiNote) {
        return this.activeNotes.some(note => note.midiNote === midiNote);
    }

    displayError(message) {
        this.currentChordElement.textContent = message;
        this.currentChordElement.className = 'chord-name error';
        this.confidenceElement.textContent = '—';
    }

    // Get chromatic distribution for visualization
    getChromaticDistribution() {
        const chromatic = new Array(12).fill(0);
        
        this.activeNotes.forEach(note => {
            const pitchClass = note.midiNote % 12;
            chromatic[pitchClass] = Math.max(chromatic[pitchClass], note.strength);
        });
        
        return chromatic.map((strength, index) => ({
            note: this.noteNames[index],
            strength: strength
        }));
    }

    // Export current piano state as image
    exportPianoImage() {
        return this.pianoCanvas.toDataURL('image/png');
    }

    // Resize handler
    resize() {
        this.setupPianoCanvas();
        this.drawPiano();
    }
}

// Add CSS classes for confidence levels
const confidenceStyles = `
    .chord-name.high-confidence { color: #38a169; }
    .chord-name.medium-confidence { color: #ed8936; }
    .chord-name.low-confidence { color: #e53e3e; }
    .chord-name.very-low-confidence { color: #a0aec0; }
    .chord-name.single-note { color: #805ad5; }
    .chord-name.unknown { color: #718096; }
    .chord-name.error { color: #e53e3e; }
    .candidate-item.top-candidate { background: #f0fff4; border-left: 3px solid #38a169; }
`;

// Inject styles
const styleSheet = document.createElement('style');
styleSheet.textContent = confidenceStyles;
document.head.appendChild(styleSheet);

// Export for use in other modules
window.ChordDisplay = ChordDisplay;