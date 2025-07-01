/**
 * Chroma-based Chord Detection - Industry standard approach
 * Based on research by Fujishima (1999) and Bartsch & Wakefield (2005)
 */
class ChromaChordDetector {
    constructor() {
        // Chroma vector templates for major/minor triads
        this.chordTemplates = this.createChordTemplates();
        
        // Tuning and analysis parameters
        this.tuningFreq = 440.0; // A4 = 440Hz
        this.chromaSmoothing = 0.6; // Reduced from 0.8 for stronger smoothing
        this.minChordDuration = 0.2; // 200ms minimum
        
        // Enhanced temporal smoothing
        this.previousChroma = new Array(12).fill(0);
        this.currentChord = null;
        this.chordStabilityCounter = 0;
        this.requiredStability = 8; // Increased from 3 to 8 frames (133ms at 60fps)
        
        // Multi-frame chroma integration for musical stability
        this.chromaHistory = [];
        this.maxHistoryFrames = 10; // 167ms worth at 60fps
        this.temporalWeights = [0.3, 0.2, 0.15, 0.12, 0.08, 0.06, 0.04, 0.03, 0.01, 0.01]; // Exponential decay
        
        // Chord change hysteresis parameters
        this.CHANGE_THRESHOLD = 0.08; // 8% confidence difference required for chord change
        this.lastConfidence = 0;
        this.MIN_STABLE_FRAMES = 8;
    }
    
    createChordTemplates() {
        const templates = {};
        
        // Major chord templates (root, major third, perfect fifth)
        const majorPattern = [1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0];
        const minorPattern = [1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0];
        const dom7Pattern = [1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0];
        const min7Pattern = [1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0];
        
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        
        // Generate all 12 major and minor chords
        for (let root = 0; root < 12; root++) {
            templates[noteNames[root]] = this.rotatePattern(majorPattern, root);
            templates[noteNames[root] + 'm'] = this.rotatePattern(minorPattern, root);
            templates[noteNames[root] + '7'] = this.rotatePattern(dom7Pattern, root);
            templates[noteNames[root] + 'm7'] = this.rotatePattern(min7Pattern, root);
        }
        
        return templates;
    }
    
    rotatePattern(pattern, steps) {
        const rotated = new Array(12);
        for (let i = 0; i < 12; i++) {
            rotated[i] = pattern[(i - steps + 12) % 12];
        }
        return rotated;
    }
    
    /**
     * Main chord detection method
     */
    detectChord(frequencyData, sampleRate) {
        // Step 1: Convert FFT to Chroma vector
        const chromaVector = this.computeChromaVector(frequencyData, sampleRate);
        
        // Step 2: Apply temporal smoothing
        const smoothedChroma = this.applyTemporalSmoothing(chromaVector);
        
        // Step 3: Match against chord templates
        const chordCandidate = this.matchChordTemplates(smoothedChroma);
        
        // Step 4: Apply stability filtering
        const stableChord = this.applyStabilityFilter(chordCandidate);
        
        return stableChord;
    }
    
    computeChromaVector(frequencyData, sampleRate) {
        const chroma = new Array(12).fill(0);
        const chromaVelocities = new Array(12).fill(0); // Track raw velocities
        const nyquist = sampleRate / 2;
        const binWidth = nyquist / frequencyData.length;
        
        // Enhanced frequency band masking: focus on fundamental range (60Hz-350Hz for bass, up to 2kHz for mids)
        const bassBinMin = Math.floor(60 / binWidth);    // Bass fundamentals
        const bassBinMax = Math.floor(350 / binWidth);   // Bass fundamental limit
        const midBinMax = Math.floor(2000 / binWidth);   // Reduce high-frequency noise
        
        // Process bass range with higher weight
        for (let bin = bassBinMin; bin < Math.min(bassBinMax, frequencyData.length); bin++) {
            const frequency = bin * binWidth;
            const magnitude = frequencyData[bin] / 255.0;
            
            if (magnitude < 0.05) continue;
            
            const midiNote = this.frequencyToMidiNote(frequency);
            if (midiNote < 21 || midiNote > 108) continue;
            
            const chromaClass = midiNote % 12;
            const octave = Math.floor(midiNote / 12);
            const octaveWeight = this.getImprovedOctaveWeight(octave, frequency);
            const fundamentalWeight = this.getFundamentalWeight(frequency, bin, frequencyData);
            
            // Extra weight for bass range (fundamental frequencies)
            const bassWeight = 1.5;
            const weightedMagnitude = magnitude * octaveWeight * fundamentalWeight * bassWeight;
            
            chroma[chromaClass] += weightedMagnitude;
            chromaVelocities[chromaClass] = Math.max(chromaVelocities[chromaClass], magnitude);
        }
        
        // Process mid range with normal weight
        for (let bin = bassBinMax; bin < Math.min(midBinMax, frequencyData.length); bin++) {
            const frequency = bin * binWidth;
            const magnitude = frequencyData[bin] / 255.0;
            
            if (magnitude < 0.05) continue;
            
            const midiNote = this.frequencyToMidiNote(frequency);
            if (midiNote < 21 || midiNote > 108) continue;
            
            const chromaClass = midiNote % 12;
            const octave = Math.floor(midiNote / 12);
            const octaveWeight = this.getImprovedOctaveWeight(octave, frequency);
            const fundamentalWeight = this.getFundamentalWeight(frequency, bin, frequencyData);
            
            const weightedMagnitude = magnitude * octaveWeight * fundamentalWeight;
            
            chroma[chromaClass] += weightedMagnitude;
            chromaVelocities[chromaClass] = Math.max(chromaVelocities[chromaClass], magnitude);
        }
        
        // Apply chroma enhancement (reduce noise, emphasize strong notes)
        this.enhanceChromaVector(chroma);
        
        // Apply IIR temporal smoothing: α * current + (1-α) * previous
        const α = this.chromaSmoothing; // 0.6 for stronger smoothing
        for (let i = 0; i < 12; i++) {
            chroma[i] = α * chroma[i] + (1 - α) * this.previousChroma[i];
        }
        
        // Store for next frame
        this.previousChroma = chroma.slice();
        this.lastChromaVelocities = chromaVelocities;
        
        // Multi-frame integration for musical stability
        const stabilizedChroma = this.integrateMultiFrameChroma(chroma);
        
        // Normalize chroma vector
        const sum = stabilizedChroma.reduce((a, b) => a + b, 0);
        if (sum > 0) {
            for (let i = 0; i < 12; i++) {
                stabilizedChroma[i] /= sum;
            }
        }
        
        return stabilizedChroma;
    }
    
    // Multi-frame chroma integration for musical stability
    integrateMultiFrameChroma(currentChroma) {
        // Add current frame to history
        this.chromaHistory.unshift(currentChroma.slice());
        
        // Maintain history size
        if (this.chromaHistory.length > this.maxHistoryFrames) {
            this.chromaHistory.pop();
        }
        
        // If we don't have enough history, return current frame
        if (this.chromaHistory.length < 3) {
            return currentChroma;
        }
        
        // Weighted integration of historical frames
        const integratedChroma = new Array(12).fill(0);
        
        for (let frameIndex = 0; frameIndex < this.chromaHistory.length; frameIndex++) {
            const weight = this.temporalWeights[frameIndex] || 0.01;
            const frameChroma = this.chromaHistory[frameIndex];
            
            for (let i = 0; i < 12; i++) {
                integratedChroma[i] += frameChroma[i] * weight;
            }
        }
        
        return integratedChroma;
    }
    
    // New method: Get velocity-aware notes directly from FFT
    computeVelocityAwareNotes(frequencyData, sampleRate) {
        const notes = new Map(); // midiNote -> {strength, velocity, frequency}
        const nyquist = sampleRate / 2;
        const binWidth = nyquist / frequencyData.length;
        
        // Find spectral peaks first
        const peaks = this.findSpectralPeaksWithVelocity(frequencyData, binWidth);
        
        // Convert peaks to MIDI notes with proper velocity calculation
        for (const peak of peaks) {
            const midiNote = this.frequencyToMidiNote(peak.frequency);
            
            if (midiNote < 21 || midiNote > 108) continue;
            
            // Calculate velocity based on magnitude and frequency characteristics
            const velocity = this.calculateVelocityFromMagnitude(peak, frequencyData, midiNote);
            
            // If we already have this note, keep the stronger one
            if (!notes.has(midiNote) || notes.get(midiNote).velocity < velocity) {
                notes.set(midiNote, {
                    midiNote: midiNote,
                    frequency: peak.frequency,
                    strength: peak.magnitude,
                    velocity: velocity,
                    octave: Math.floor(midiNote / 12),
                    chromaClass: midiNote % 12
                });
            }
        }
        
        return Array.from(notes.values())
            .sort((a, b) => b.velocity - a.velocity) // Sort by velocity
            .slice(0, 8); // Top 8 notes max
    }
    
    findSpectralPeaksWithVelocity(frequencyData, binWidth) {
        const peaks = [];
        
        // Adaptive threshold: median + 6dB for noise floor adaptation
        const adaptiveThreshold = this.calculateAdaptiveThreshold(frequencyData);
        const minPeakDistance = 3; // Increased from 2 to reduce high-frequency false peaks
        
        for (let i = minPeakDistance; i < frequencyData.length - minPeakDistance; i++) {
            const current = frequencyData[i];
            
            if (current < adaptiveThreshold) continue;
            
            // Check if it's a local maximum with increased distance
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
                
                if (frequency >= 60 && frequency <= 4000) {
                    peaks.push({
                        frequency,
                        magnitude,
                        bin: i,
                        peakWidth: this.calculatePeakWidth(frequencyData, i),
                        prominence: this.calculatePeakProminence(frequencyData, i)
                    });
                }
            }
        }
        
        return peaks.sort((a, b) => b.magnitude - a.magnitude);
    }
    
    calculateAdaptiveThreshold(frequencyData) {
        // Calculate median for noise floor estimation
        const sortedData = [...frequencyData].sort((a, b) => a - b);
        const median = sortedData[Math.floor(sortedData.length / 2)];
        
        // Temporary: Lower threshold for debugging
        const adaptiveThreshold = Math.max(10, median * 1.5);
        
        return adaptiveThreshold;
    }
    
    calculateVelocityFromMagnitude(peak, frequencyData, midiNote) {
        // Back to simple linear scaling with temporal smoothing for stability
        let velocity = peak.magnitude * 127;
        
        // Octave weighting for musical realism
        const octave = Math.floor(midiNote / 12);
        if (octave >= 3 && octave <= 5) {
            velocity *= 1.0; // No adjustment for middle range
        } else if (octave >= 2 && octave <= 6) {
            velocity *= 0.9; // Slight reduction for outer range
        } else {
            velocity *= 0.7; // More reduction for extreme ranges
        }
        
        // Prominence impact (how much it stands out)
        velocity *= Math.min(1.3, peak.prominence);
        
        // Peak width impact (narrower peaks are often more fundamental)
        if (peak.peakWidth < 3) {
            velocity *= 1.1;
        }
        
        const finalVelocity = Math.max(1, Math.min(127, Math.round(velocity)));
        
        // Debug log for velocity values
        if (Math.random() < 0.02) { // Log 2% of calculations
            console.log(`Velocity calc: mag=${peak.magnitude.toFixed(3)}, final=${finalVelocity}, note=${midiNote}`);
        }
        
        return finalVelocity;
    }
    
    calculatePeakWidth(frequencyData, centerBin) {
        let width = 1;
        const centerValue = frequencyData[centerBin];
        const threshold = centerValue * 0.5; // Half-maximum width
        
        // Find width to the left
        for (let i = centerBin - 1; i >= 0 && frequencyData[i] > threshold; i--) {
            width++;
        }
        
        // Find width to the right
        for (let i = centerBin + 1; i < frequencyData.length && frequencyData[i] > threshold; i++) {
            width++;
        }
        
        return width;
    }
    
    calculatePeakProminence(frequencyData, centerBin) {
        const centerValue = frequencyData[centerBin];
        
        // Find minimum valleys on both sides
        let leftMin = centerValue;
        let rightMin = centerValue;
        
        for (let i = centerBin - 1; i >= Math.max(0, centerBin - 20); i--) {
            leftMin = Math.min(leftMin, frequencyData[i]);
        }
        
        for (let i = centerBin + 1; i < Math.min(frequencyData.length, centerBin + 20); i++) {
            rightMin = Math.min(rightMin, frequencyData[i]);
        }
        
        const prominence = centerValue - Math.max(leftMin, rightMin);
        return Math.max(1.0, prominence / 255.0);
    }
    
    getImprovedOctaveWeight(octave, frequency) {
        // Enhanced octave weighting favoring musical range
        if (octave >= 3 && octave <= 5) return 1.0;    // C3-B5: Primary range
        if (octave >= 2 && octave <= 6) return 0.8;    // C2-B6: Secondary range  
        if (octave >= 1 && octave <= 7) return 0.5;    // C1-B7: Tertiary range
        return 0.2; // Very high/low frequencies
    }
    
    getFundamentalWeight(frequency, bin, frequencyData) {
        // Check if this frequency is likely a fundamental (not harmonic)
        let fundamentalScore = 1.0;
        
        // Gradual harmonic decay using exponential model: 0.85^harmonic_index
        // Look for subharmonics (frequency/2, frequency/3, etc.)
        for (let divisor = 2; divisor <= 4; divisor++) {
            const subharmonicFreq = frequency / divisor;
            const subharmonicBin = Math.round(subharmonicFreq * frequencyData.length * 2 / 44100);
            
            if (subharmonicBin > 0 && subharmonicBin < frequencyData.length) {
                const subharmonicMagnitude = frequencyData[subharmonicBin] / 255.0;
                if (subharmonicMagnitude > 0.1) {
                    // Gradual decay instead of harsh 0.7 penalty
                    const harmonicIndex = divisor - 1;
                    fundamentalScore *= Math.pow(0.85, harmonicIndex);
                }
            }
        }
        
        return fundamentalScore;
    }
    
    enhanceChromaVector(chroma) {
        // Apply non-linear enhancement to emphasize strong notes
        for (let i = 0; i < 12; i++) {
            if (chroma[i] > 0.1) {
                chroma[i] = Math.pow(chroma[i], 0.7); // Compress weak signals
            } else {
                chroma[i] *= 0.5; // Suppress very weak signals
            }
        }
    }
    
    frequencyToMidiNote(frequency) {
        return Math.round(12 * Math.log2(frequency / this.tuningFreq) + 69);
    }
    
    getOctaveWeight(octave) {
        // Give more weight to mid-range frequencies (like piano middle register)
        if (octave >= 3 && octave <= 5) return 1.0;
        if (octave >= 2 && octave <= 6) return 0.7;
        return 0.3;
    }
    
    applyTemporalSmoothing(chromaVector) {
        const smoothed = new Array(12);
        
        for (let i = 0; i < 12; i++) {
            smoothed[i] = this.chromaSmoothing * this.previousChroma[i] + 
                         (1 - this.chromaSmoothing) * chromaVector[i];
        }
        
        this.previousChroma = smoothed.slice();
        return smoothed;
    }
    
    matchChordTemplates(chromaVector) {
        let bestMatch = null;
        let bestScore = -1;
        
        for (const [chordName, template] of Object.entries(this.chordTemplates)) {
            const score = this.computeTemplateMatch(chromaVector, template);
            
            if (score > bestScore) {
                bestScore = score;
                bestMatch = {
                    name: chordName,
                    confidence: score
                };
            }
        }
        
        // Only return chord if confidence is above threshold
        if (bestMatch && bestMatch.confidence > 0.6) {
            return bestMatch;
        }
        
        return null;
    }
    
    computeTemplateMatch(chroma, template) {
        // Cosine similarity between chroma vector and template
        let dotProduct = 0;
        let chromaMagnitude = 0;
        let templateMagnitude = 0;
        
        for (let i = 0; i < 12; i++) {
            dotProduct += chroma[i] * template[i];
            chromaMagnitude += chroma[i] * chroma[i];
            templateMagnitude += template[i] * template[i];
        }
        
        const magnitude = Math.sqrt(chromaMagnitude * templateMagnitude);
        return magnitude > 0 ? dotProduct / magnitude : 0;
    }
    
    applyStabilityFilter(chordCandidate) {
        if (!chordCandidate) {
            this.chordStabilityCounter = 0;
            return this.currentChord;
        }
        
        // Check if same chord as previous detection
        if (this.currentChord && this.currentChord.name === chordCandidate.name) {
            this.chordStabilityCounter++;
        } else {
            this.chordStabilityCounter = 1;
        }
        
        // Only update if chord is stable for required duration
        if (this.chordStabilityCounter >= this.requiredStability) {
            this.currentChord = chordCandidate;
        }
        
        return this.currentChord;
    }
    
    /**
     * Get detailed analysis including all chord candidates
     */
    getDetailedAnalysis(chromaVector) {
        const candidates = [];
        
        for (const [chordName, template] of Object.entries(this.chordTemplates)) {
            const score = this.computeTemplateMatch(chromaVector, template);
            candidates.push({
                name: chordName,
                confidence: score
            });
        }
        
        // Sort by confidence
        candidates.sort((a, b) => b.confidence - a.confidence);
        
        return candidates.slice(0, 5); // Top 5 candidates
    }
    
    /**
     * Debug method to visualize chroma vector
     */
    getChromaVisualization(chromaVector) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        
        return chromaVector.map((value, index) => ({
            note: noteNames[index],
            strength: value
        }));
    }
}

// Export for use in other modules
window.ChromaChordDetector = ChromaChordDetector;