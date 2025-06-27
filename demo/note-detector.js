/**
 * Note Detector - FFT to MIDI note conversion and processing
 */
class NoteDetector {
    constructor() {
        this.threshold = 0.2;           // Higher threshold to reduce noise
        this.sensitivity = 1.0;         // Sensitivity multiplier
        this.smoothingFactor = 0.9;     // More aggressive smoothing
        
        this.noteNames = [
            'C', 'C#', 'D', 'D#', 'E', 'F', 
            'F#', 'G', 'G#', 'A', 'A#', 'B'
        ];
        
        // Previous frame data for smoothing
        this.previousNotes = new Float32Array(128);
        this.currentNotes = new Float32Array(128);
        
        // Note onset detection - more conservative
        this.noteStates = new Array(128).fill(false);
        this.noteOnsetThreshold = 0.3;  // Higher threshold
        this.noteOffsetThreshold = 0.1; // Higher offset threshold
        
        // Advanced harmonic analysis
        this.useHarmonicAnalysis = true;
        this.harmonicWeights = [1.0, 0.3, 0.15, 0.08]; // Reduced harmonic weights
        
        // Chord-focused detection
        this.maxSimultaneousNotes = 6;  // Limit to reasonable chord size
        this.fundamentalBias = 2.0;     // Bias toward fundamental frequencies
        this.chordStability = 0.85;     // Require stability over time
        
        // Frequency masking to avoid overtones
        this.maskRadius = 2;            // Mask nearby frequencies
        this.lastStableChord = [];
        this.chordStabilityFrames = 0;
        this.requiredStabilityFrames = 5; // Require 5 frames of stability
    }

    /**
     * Process frequency data and extract active notes
     */
    processFrequencyData(frequencyData, sampleRate, fftSize) {
        const nyquist = sampleRate / 2;
        const binWidth = nyquist / (frequencyData.length);
        
        // Reset current notes array
        this.currentNotes.fill(0);
        
        // Step 1: Find peak candidates with better peak detection
        const peakCandidates = this.findSpectralPeaks(frequencyData, binWidth);
        
        // Step 2: Convert peaks to MIDI notes with fundamental bias
        const noteCandidates = this.peaksToNotes(peakCandidates);
        
        // Step 3: Apply harmonic grouping to reduce overtones
        const filteredNotes = this.applyHarmonicGrouping(noteCandidates);
        
        // Step 4: Limit to most prominent notes (chord-sized)
        const prominentNotes = this.selectProminentNotes(filteredNotes);
        
        // Step 5: Apply stability filtering
        const stableNotes = this.applyStabilityFilter(prominentNotes);
        
        // Copy current to previous for next frame
        this.previousNotes.set(this.currentNotes);
        
        return stableNotes;
    }
    
    findSpectralPeaks(frequencyData, binWidth) {
        const peaks = [];
        const minPeakHeight = this.threshold * 255;
        const minPeakDistance = 3; // Minimum distance between peaks
        
        for (let i = minPeakDistance; i < frequencyData.length - minPeakDistance; i++) {
            const current = frequencyData[i];
            
            if (current < minPeakHeight) continue;
            
            // Check if it's a local maximum
            let isPeak = true;
            for (let j = 1; j <= minPeakDistance; j++) {
                if (frequencyData[i-j] >= current || frequencyData[i+j] >= current) {
                    isPeak = false;
                    break;
                }
            }
            
            if (isPeak) {
                const frequency = i * binWidth;
                const magnitude = current / 255.0;
                
                // Only consider musical frequency range
                if (frequency >= 80 && frequency <= 2000) {
                    peaks.push({
                        frequency,
                        magnitude,
                        bin: i
                    });
                }
            }
        }
        
        return peaks.sort((a, b) => b.magnitude - a.magnitude);
    }
    
    peaksToNotes(peaks) {
        const notes = [];
        
        for (const peak of peaks) {
            const midiNote = this.frequencyToMidiNote(peak.frequency);
            
            // Only consider reasonable MIDI range
            if (midiNote >= 36 && midiNote <= 84) { // C2 to C6
                const noteStrength = peak.magnitude * this.fundamentalBias;
                
                notes.push({
                    midiNote: midiNote,
                    noteName: this.midiNoteToName(midiNote),
                    frequency: peak.frequency,
                    strength: Math.min(1.0, noteStrength),
                    velocity: Math.round(noteStrength * 127),
                    isOnset: false // Will be determined later
                });
            }
        }
        
        return notes;
    }
    
    applyHarmonicGrouping(notes) {
        const groupedNotes = [];
        const usedNotes = new Set();
        
        // Sort by strength (strongest first)
        notes.sort((a, b) => b.strength - a.strength);
        
        for (const note of notes) {
            if (usedNotes.has(note.midiNote)) continue;
            
            // Check if this note is likely a harmonic of a stronger note
            let isHarmonic = false;
            
            for (const otherNote of groupedNotes) {
                const ratio = note.frequency / otherNote.frequency;
                
                // Check for harmonic relationships (2:1, 3:1, 4:1, 5:1)
                if (Math.abs(ratio - 2) < 0.1 || 
                    Math.abs(ratio - 3) < 0.15 || 
                    Math.abs(ratio - 4) < 0.2 ||
                    Math.abs(ratio - 5) < 0.25) {
                    isHarmonic = true;
                    break;
                }
            }
            
            if (!isHarmonic) {
                groupedNotes.push(note);
                usedNotes.add(note.midiNote);
            }
        }
        
        return groupedNotes;
    }
    
    selectProminentNotes(notes) {
        // Sort by strength and take only the top notes
        const sorted = notes.sort((a, b) => b.strength - a.strength);
        return sorted.slice(0, this.maxSimultaneousNotes);
    }
    
    applyStabilityFilter(currentNotes) {
        // Compare with previous stable chord
        const currentChordPattern = currentNotes.map(n => n.midiNote % 12).sort();
        const lastChordPattern = this.lastStableChord.map(n => n.midiNote % 12).sort();
        
        // Check if chord pattern is similar (allowing some variation)
        const similarity = this.calculateChordSimilarity(currentChordPattern, lastChordPattern);
        
        if (similarity > this.chordStability) {
            this.chordStabilityFrames++;
        } else {
            this.chordStabilityFrames = 0;
        }
        
        // Only return notes if we have sufficient stability
        if (this.chordStabilityFrames >= this.requiredStabilityFrames) {
            this.lastStableChord = currentNotes;
            
            // Update note states and onset detection
            const activeNotes = [];
            for (const note of currentNotes) {
                const wasActive = this.noteStates[note.midiNote];
                this.noteStates[note.midiNote] = true;
                
                activeNotes.push({
                    ...note,
                    isOnset: !wasActive
                });
            }
            
            // Clear inactive notes
            for (let i = 0; i < this.noteStates.length; i++) {
                if (!currentNotes.some(n => n.midiNote === i)) {
                    this.noteStates[i] = false;
                }
            }
            
            return activeNotes;
        }
        
        return this.lastStableChord; // Return last stable chord
    }
    
    calculateChordSimilarity(chord1, chord2) {
        if (chord1.length === 0 && chord2.length === 0) return 1.0;
        if (chord1.length === 0 || chord2.length === 0) return 0.0;
        
        const intersection = chord1.filter(note => chord2.includes(note));
        const union = [...new Set([...chord1, ...chord2])];
        
        return intersection.length / union.length;
    }

    /**
     * Analyze the strength of a specific MIDI note in the frequency data
     */
    analyzeNoteStrength(midiNote, frequencyData, binWidth) {
        const fundamentalFreq = this.midiNoteToFrequency(midiNote);
        let totalStrength = 0;
        
        if (this.useHarmonicAnalysis) {
            // Analyze fundamental + harmonics
            for (let harmonic = 1; harmonic <= this.harmonicWeights.length; harmonic++) {
                const harmonicFreq = fundamentalFreq * harmonic;
                const harmonicBin = Math.round(harmonicFreq / binWidth);
                
                if (harmonicBin < frequencyData.length) {
                    // Use a small window around the harmonic frequency
                    const windowSize = 2;
                    let maxMagnitude = 0;
                    
                    for (let i = -windowSize; i <= windowSize; i++) {
                        const bin = harmonicBin + i;
                        if (bin >= 0 && bin < frequencyData.length) {
                            maxMagnitude = Math.max(maxMagnitude, frequencyData[bin]);
                        }
                    }
                    
                    // Normalize magnitude (0-255 -> 0-1) and apply harmonic weight
                    const normalizedMagnitude = maxMagnitude / 255.0;
                    const weight = this.harmonicWeights[harmonic - 1] || 0;
                    totalStrength += normalizedMagnitude * weight;
                }
            }
        } else {
            // Simple fundamental frequency analysis
            const fundamentalBin = Math.round(fundamentalFreq / binWidth);
            if (fundamentalBin < frequencyData.length) {
                totalStrength = frequencyData[fundamentalBin] / 255.0;
            }
        }
        
        return Math.min(1.0, totalStrength);
    }

    /**
     * Apply temporal smoothing to reduce noise
     */
    applySmoothening() {
        for (let i = 0; i < this.currentNotes.length; i++) {
            this.currentNotes[i] = 
                this.smoothingFactor * this.previousNotes[i] + 
                (1 - this.smoothingFactor) * this.currentNotes[i];
        }
    }

    /**
     * Convert MIDI note number to frequency in Hz
     */
    midiNoteToFrequency(midiNote) {
        // A4 = 440Hz = MIDI note 69
        return 440 * Math.pow(2, (midiNote - 69) / 12);
    }

    /**
     * Convert MIDI note number to note name
     */
    midiNoteToName(midiNote) {
        const octave = Math.floor(midiNote / 12) - 1;
        const noteIndex = midiNote % 12;
        return this.noteNames[noteIndex] + octave;
    }

    /**
     * Convert frequency to MIDI note number
     */
    frequencyToMidiNote(frequency) {
        return Math.round(12 * Math.log2(frequency / 440) + 69);
    }

    /**
     * Get notes grouped by chromatic class (C, C#, D, etc.)
     */
    getChromaticProfile(activeNotes) {
        const chromatic = new Array(12).fill(0);
        
        activeNotes.forEach(note => {
            const chromaticClass = note.midiNote % 12;
            chromatic[chromaticClass] = Math.max(chromatic[chromaticClass], note.strength);
        });
        
        return chromatic.map((strength, index) => ({
            note: this.noteNames[index],
            strength: strength
        }));
    }

    /**
     * Apply pitch class weighting for chord detection
     */
    getWeightedPitchClasses(activeNotes) {
        const pitchClasses = new Array(12).fill(0);
        
        activeNotes.forEach(note => {
            const pitchClass = note.midiNote % 12;
            // Weight by strength and octave position (lower octaves get more weight)
            const octaveWeight = Math.max(0.3, 1.0 - (Math.floor(note.midiNote / 12) - 4) * 0.1);
            pitchClasses[pitchClass] += note.strength * octaveWeight;
        });
        
        return pitchClasses;
    }

    /**
     * Configuration setters
     */
    setThreshold(threshold) {
        this.threshold = Math.max(0.01, Math.min(1.0, threshold));
        this.noteOnsetThreshold = this.threshold * 1.5;
        this.noteOffsetThreshold = this.threshold * 0.5;
    }

    setSensitivity(sensitivity) {
        this.sensitivity = Math.max(0.1, Math.min(3.0, sensitivity));
    }

    setSmoothing(smoothing) {
        this.smoothingFactor = Math.max(0.0, Math.min(0.95, smoothing));
    }

    setHarmonicAnalysis(enabled) {
        this.useHarmonicAnalysis = enabled;
    }

    /**
     * Reset all state
     */
    reset() {
        this.previousNotes.fill(0);
        this.currentNotes.fill(0);
        this.noteStates.fill(false);
    }

    /**
     * Get statistics about current detection state
     */
    getStats() {
        const activeCount = this.noteStates.filter(state => state).length;
        const averageStrength = activeCount > 0 ? 
            this.currentNotes.reduce((sum, strength) => sum + strength, 0) / activeCount : 0;
        
        return {
            activeNotes: activeCount,
            averageStrength: averageStrength,
            threshold: this.threshold,
            sensitivity: this.sensitivity,
            smoothing: this.smoothingFactor
        };
    }
}

// Export for use in other modules
window.NoteDetector = NoteDetector;