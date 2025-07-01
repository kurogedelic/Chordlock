/**
 * Advanced Key Analysis Framework
 * Implements multiple key estimation approaches for context-aware chord analysis
 */
class KeyAnalyzer {
    constructor() {
        // Krumhansl-Schmuckler Key Profiles (Major/Minor)
        this.keyProfiles = this.createKeyProfiles();
        
        // Current key state
        this.currentKey = null;
        this.currentMode = null; // 'major' or 'minor'
        this.keyConfidence = 0;
        this.manualKeyOverride = null;
        
        // Key estimation history for stability
        this.keyHistory = [];
        this.maxKeyHistory = 20; // 20 frames for stability analysis
        
        // Chord function analysis
        this.chordFunctions = this.createChordFunctionMapping();
        this.progressionHistory = [];
        this.maxProgressionHistory = 8;
        
        // Key change detection
        this.lastKeyChangeTime = 0;
        this.minKeyChangeInterval = 5000; // 5 seconds minimum between key changes
        
        // Mode: 'auto', 'manual', 'first-chord', 'progression-analysis'
        this.keyDetectionMode = 'auto';
    }

    // Krumhansl-Schmuckler Key Profiles (研究に基づく重み)
    createKeyProfiles() {
        // Major key profile (C major as reference)
        const majorProfile = [
            6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88
        ];
        
        // Minor key profile (A minor as reference)  
        const minorProfile = [
            6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17
        ];
        
        const profiles = { major: {}, minor: {} };
        
        // Generate all 24 key profiles (12 major + 12 minor)
        for (let tonic = 0; tonic < 12; tonic++) {
            const keyName = this.noteIndexToName(tonic);
            
            // Rotate profile to match tonic
            profiles.major[keyName] = this.rotateArray(majorProfile, tonic);
            profiles.minor[keyName] = this.rotateArray(minorProfile, tonic);
        }
        
        return profiles;
    }

    // Chord function mapping (Roman numeral analysis)
    createChordFunctionMapping() {
        return {
            major: {
                // I, ii, iii, IV, V, vi, vii°
                functions: ['I', 'ii', 'iii', 'IV', 'V', 'vi', 'vii°'],
                roles: ['tonic', 'subdominant', 'tonic', 'subdominant', 'dominant', 'tonic', 'dominant']
            },
            minor: {
                // i, ii°, III, iv, v/V, VI, VII
                functions: ['i', 'ii°', 'III', 'iv', 'v', 'VI', 'VII'],
                roles: ['tonic', 'subdominant', 'tonic', 'subdominant', 'dominant', 'tonic', 'dominant']
            }
        };
    }

    // Main key estimation method
    analyzeKey(chromaVector, chordProgression = null) {
        let estimatedKey = null;
        
        switch (this.keyDetectionMode) {
            case 'manual':
                estimatedKey = this.manualKeyOverride;
                break;
                
            case 'first-chord':
                estimatedKey = this.estimateFromFirstChord(chordProgression);
                break;
                
            case 'progression-analysis':
                estimatedKey = this.estimateFromProgression(chordProgression);
                break;
                
            case 'auto':
            default:
                // Krumhansl-Schmuckler with progression context
                const ksEstimate = this.krumhanslSchmucklerAnalysis(chromaVector);
                const progressionEstimate = this.estimateFromProgression(chordProgression);
                estimatedKey = this.combineEstimates(ksEstimate, progressionEstimate);
                break;
        }
        
        // Apply stability filtering
        const stableKey = this.applyKeyStabilityFilter(estimatedKey);
        
        // Update internal state
        if (stableKey && this.shouldUpdateKey(stableKey)) {
            this.currentKey = stableKey.key;
            this.currentMode = stableKey.mode;
            this.keyConfidence = stableKey.confidence;
            this.lastKeyChangeTime = performance.now();
        }
        
        return {
            key: this.currentKey,
            mode: this.currentMode,
            confidence: this.keyConfidence,
            detectionMethod: this.keyDetectionMode
        };
    }

    // Krumhansl-Schmuckler Algorithm Implementation
    krumhanslSchmucklerAnalysis(chromaVector) {
        if (!chromaVector || chromaVector.length !== 12) {
            return null;
        }
        
        let bestKey = null;
        let bestMode = null;
        let maxCorrelation = -1;
        
        // Test all 24 possible keys (12 major + 12 minor)
        for (const [keyName, profile] of Object.entries(this.keyProfiles.major)) {
            const correlation = this.calculateCorrelation(chromaVector, profile);
            if (correlation > maxCorrelation) {
                maxCorrelation = correlation;
                bestKey = keyName;
                bestMode = 'major';
            }
        }
        
        for (const [keyName, profile] of Object.entries(this.keyProfiles.minor)) {
            const correlation = this.calculateCorrelation(chromaVector, profile);
            if (correlation > maxCorrelation) {
                maxCorrelation = correlation;
                bestKey = keyName;
                bestMode = 'minor';
            }
        }
        
        return {
            key: bestKey,
            mode: bestMode,
            confidence: Math.max(0, Math.min(1, (maxCorrelation + 1) / 2)), // Normalize to 0-1
            method: 'krumhansl-schmuckler'
        };
    }

    // Estimate key from first chord (simple heuristic)
    estimateFromFirstChord(chordProgression) {
        if (!chordProgression || chordProgression.length === 0) {
            return null;
        }
        
        const firstChord = chordProgression[0];
        const rootNote = this.extractRootNote(firstChord);
        const chordQuality = this.extractChordQuality(firstChord);
        
        // Simple heuristic: assume first chord is tonic
        const mode = chordQuality.includes('m') ? 'minor' : 'major';
        
        return {
            key: rootNote,
            mode: mode,
            confidence: 0.6, // Moderate confidence for this heuristic
            method: 'first-chord'
        };
    }

    // Estimate key from chord progression analysis
    estimateFromProgression(chordProgression) {
        if (!chordProgression || chordProgression.length < 3) {
            return null;
        }
        
        // Analyze recent progression for common patterns
        const recentChords = chordProgression.slice(-5); // Last 5 chords
        
        // Look for common progressions that strongly indicate key
        const keyEvidence = {};
        
        for (let i = 0; i < recentChords.length - 1; i++) {
            const current = recentChords[i];
            const next = recentChords[i + 1];
            
            // Analyze specific progressions
            const progressionAnalysis = this.analyzeChordProgression(current, next);
            
            for (const [key, evidence] of Object.entries(progressionAnalysis)) {
                keyEvidence[key] = (keyEvidence[key] || 0) + evidence;
            }
        }
        
        // Find most likely key
        let bestKey = null;
        let maxEvidence = 0;
        
        for (const [key, evidence] of Object.entries(keyEvidence)) {
            if (evidence > maxEvidence) {
                maxEvidence = evidence;
                bestKey = key;
            }
        }
        
        if (bestKey) {
            const [keyName, mode] = bestKey.split('-');
            return {
                key: keyName,
                mode: mode,
                confidence: Math.min(1, maxEvidence / 3), // Normalize
                method: 'progression-analysis'
            };
        }
        
        return null;
    }

    // Analyze specific chord progressions for key evidence
    analyzeChordProgression(chord1, chord2) {
        const evidence = {};
        
        // V-I progressions (dominant to tonic) - strong key indicators
        const dominantToTonicPatterns = [
            ['G', 'C'], ['G7', 'C'], ['D', 'G'], ['D7', 'G'],
            ['A', 'D'], ['A7', 'D'], ['E', 'A'], ['E7', 'A'],
            ['B', 'E'], ['B7', 'E'], ['F#', 'B'], ['F#7', 'B'],
            ['C#', 'F#'], ['C#7', 'F#']
        ];
        
        for (const [dom, tonic] of dominantToTonicPatterns) {
            if (this.chordsMatch(chord1, dom) && this.chordsMatch(chord2, tonic)) {
                evidence[`${tonic}-major`] = 2.0; // Strong evidence
            }
        }
        
        // iv-I progressions in major keys
        const subdominantToTonicMajor = [
            ['F', 'C'], ['C', 'G'], ['G', 'D'], ['D', 'A'],
            ['A', 'E'], ['E', 'B'], ['B', 'F#'], ['F#', 'C#']
        ];
        
        for (const [sub, tonic] of subdominantToTonicMajor) {
            if (this.chordsMatch(chord1, sub) && this.chordsMatch(chord2, tonic)) {
                evidence[`${tonic}-major`] = 1.5;
            }
        }
        
        return evidence;
    }

    // Combine multiple estimation methods
    combineEstimates(ksEstimate, progressionEstimate) {
        if (!ksEstimate && !progressionEstimate) return null;
        if (!progressionEstimate) return ksEstimate;
        if (!ksEstimate) return progressionEstimate;
        
        // Weight estimates based on confidence and method reliability
        const ksWeight = ksEstimate.confidence * 0.7; // K-S is more reliable but slower
        const progWeight = progressionEstimate.confidence * 0.8; // Progression analysis is more immediate
        
        if (ksEstimate.key === progressionEstimate.key && ksEstimate.mode === progressionEstimate.mode) {
            // Agreement - high confidence
            return {
                key: ksEstimate.key,
                mode: ksEstimate.mode,
                confidence: Math.min(1, ksWeight + progWeight),
                method: 'combined'
            };
        } else {
            // Disagreement - use higher confidence estimate
            return ksWeight > progWeight ? ksEstimate : progressionEstimate;
        }
    }

    // Apply temporal stability filtering
    applyKeyStabilityFilter(newEstimate) {
        if (!newEstimate) return null;
        
        // Add to history
        this.keyHistory.unshift(newEstimate);
        if (this.keyHistory.length > this.maxKeyHistory) {
            this.keyHistory.pop();
        }
        
        // Require consistency over multiple frames for key changes
        if (this.keyHistory.length < 5) {
            return newEstimate;
        }
        
        // Count occurrences of each key in recent history
        const keyCounts = {};
        for (const estimate of this.keyHistory.slice(0, 8)) {
            const keyString = `${estimate.key}-${estimate.mode}`;
            keyCounts[keyString] = (keyCounts[keyString] || 0) + 1;
        }
        
        // Find most common key in recent history
        let mostCommonKey = null;
        let maxCount = 0;
        for (const [keyString, count] of Object.entries(keyCounts)) {
            if (count > maxCount) {
                maxCount = count;
                mostCommonKey = keyString;
            }
        }
        
        // Require majority agreement for key changes
        if (maxCount >= 5) {
            const [key, mode] = mostCommonKey.split('-');
            return {
                key: key,
                mode: mode,
                confidence: newEstimate.confidence,
                method: newEstimate.method
            };
        }
        
        return null;
    }

    // Check if key update should be applied
    shouldUpdateKey(newKeyEstimate) {
        const currentTime = performance.now();
        
        // Always allow first key detection
        if (!this.currentKey) {
            return true;
        }
        
        // Same key - no change needed
        if (this.currentKey === newKeyEstimate.key && this.currentMode === newKeyEstimate.mode) {
            return false;
        }
        
        // Respect minimum interval between key changes
        if (currentTime - this.lastKeyChangeTime < this.minKeyChangeInterval) {
            return false;
        }
        
        // Require high confidence for key changes
        return newKeyEstimate.confidence > 0.7;
    }

    // Get chord function in current key
    getChordFunction(chordName) {
        if (!this.currentKey || !this.currentMode) {
            return { function: '?', role: 'unknown' };
        }
        
        const rootNote = this.extractRootNote(chordName);
        const chordQuality = this.extractChordQuality(chordName);
        
        // Calculate scale degree
        const tonicIndex = this.noteNameToIndex(this.currentKey);
        const chordRootIndex = this.noteNameToIndex(rootNote);
        const scaleDegree = (chordRootIndex - tonicIndex + 12) % 12;
        
        // Convert scale degree to function
        const mapping = this.chordFunctions[this.currentMode];
        const degreeToIndex = [0, 2, 4, 5, 7, 9, 11]; // Major scale intervals
        
        let functionIndex = degreeToIndex.indexOf(scaleDegree);
        if (functionIndex === -1) {
            // Handle chromatic chords
            return { function: this.getChromaticFunction(scaleDegree), role: 'chromatic' };
        }
        
        return {
            function: mapping.functions[functionIndex],
            role: mapping.roles[functionIndex],
            scaleDegree: functionIndex + 1
        };
    }

    // Utility methods
    calculateCorrelation(vector1, vector2) {
        if (vector1.length !== vector2.length) return 0;
        
        const n = vector1.length;
        let sum1 = 0, sum2 = 0, sum1Sq = 0, sum2Sq = 0, pSum = 0;
        
        for (let i = 0; i < n; i++) {
            sum1 += vector1[i];
            sum2 += vector2[i];
            sum1Sq += vector1[i] * vector1[i];
            sum2Sq += vector2[i] * vector2[i];
            pSum += vector1[i] * vector2[i];
        }
        
        const num = pSum - (sum1 * sum2 / n);
        const den = Math.sqrt((sum1Sq - sum1 * sum1 / n) * (sum2Sq - sum2 * sum2 / n));
        
        return den === 0 ? 0 : num / den;
    }

    rotateArray(array, steps) {
        const result = array.slice();
        const len = result.length;
        steps = steps % len;
        return result.slice(steps).concat(result.slice(0, steps));
    }

    noteNameToIndex(noteName) {
        const noteMap = { 'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11 };
        return noteMap[noteName] || 0;
    }

    noteIndexToName(index) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        return noteNames[index % 12];
    }

    extractRootNote(chordName) {
        const match = chordName.match(/^([A-G][b#]?)/);
        return match ? match[1] : 'C';
    }

    extractChordQuality(chordName) {
        return chordName.replace(/^([A-G][b#]?)/, '') || '';
    }

    chordsMatch(chord1, chord2) {
        return this.extractRootNote(chord1) === this.extractRootNote(chord2);
    }

    getChromaticFunction(scaleDegree) {
        const chromaticMap = {
            1: 'bII', 3: 'bIII', 6: 'bVI', 8: 'bVII', 10: 'bVII'
        };
        return chromaticMap[scaleDegree] || '?';
    }

    // Manual key override
    setManualKey(keyName, mode) {
        this.manualKeyOverride = { key: keyName, mode: mode, confidence: 1.0, method: 'manual' };
        this.keyDetectionMode = 'manual';
        this.currentKey = keyName;
        this.currentMode = mode;
        this.keyConfidence = 1.0;
    }

    // Reset to automatic key detection
    enableAutoKeyDetection() {
        this.keyDetectionMode = 'auto';
        this.manualKeyOverride = null;
    }

    // Get current key info for display
    getCurrentKeyInfo() {
        return {
            key: this.currentKey,
            mode: this.currentMode,
            confidence: this.keyConfidence,
            keySignature: this.getKeySignature(this.currentKey, this.currentMode),
            detectionMode: this.keyDetectionMode
        };
    }

    getKeySignature(keyName, mode) {
        // Simplified key signature calculation
        const majorKeySignatures = {
            'C': '', 'G': '♯', 'D': '♯♯', 'A': '♯♯♯', 'E': '♯♯♯♯',
            'B': '♯♯♯♯♯', 'F#': '♯♯♯♯♯♯', 'F': '♭', 'Bb': '♭♭',
            'Eb': '♭♭♭', 'Ab': '♭♭♭♭', 'Db': '♭♭♭♭♭', 'Gb': '♭♭♭♭♭♭'
        };
        
        if (mode === 'major') {
            return majorKeySignatures[keyName] || '';
        } else {
            // For minor keys, find relative major
            const minorToMajor = {
                'A': 'C', 'E': 'G', 'B': 'D', 'F#': 'A', 'C#': 'E', 'G#': 'B',
                'D#': 'F#', 'D': 'F', 'G': 'Bb', 'C': 'Eb', 'F': 'Ab', 'Bb': 'Db', 'Eb': 'Gb'
            };
            const relativeMajor = minorToMajor[keyName];
            return relativeMajor ? majorKeySignatures[relativeMajor] || '' : '';
        }
    }
}