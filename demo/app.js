/**
 * Main Application - Coordinates all components for real-time audio analysis
 */
class ChordlockAudioDemo {
    constructor() {
        // Core modules
        this.audioAnalyzer = null;
        this.noteDetector = null;
        this.spectrumVisualizer = null;
        this.chordDisplay = null;
        this.chordlock = null;
        
        // UI elements
        this.audioFileInput = document.getElementById('audioFile');
        this.playBtn = document.getElementById('playBtn');
        this.pauseBtn = document.getElementById('pauseBtn');
        this.stopBtn = document.getElementById('stopBtn');
        this.progressBar = document.getElementById('progressBar');
        this.audioTimeDisplay = document.getElementById('audioTime');
        this.loadingOverlay = document.getElementById('loading');
        
        // Settings elements
        this.sensitivitySlider = document.getElementById('sensitivity');
        this.smoothingSlider = document.getElementById('smoothing');
        this.thresholdSlider = document.getElementById('threshold');
        this.sensitivityValue = document.getElementById('sensitivityValue');
        this.smoothingValue = document.getElementById('smoothingValue');
        this.thresholdValue = document.getElementById('thresholdValue');
        
        // Stats elements
        this.processingTimeDisplay = document.getElementById('processingTime');
        this.activeNotesDisplay = document.getElementById('activeNotes');
        this.sampleRateDisplay = document.getElementById('sampleRate');
        this.fftSizeDisplay = document.getElementById('fftSize');
        
        // Key analysis elements
        this.currentKeyDisplay = document.getElementById('currentKey');
        this.keySignatureDisplay = document.getElementById('keySignature');
        this.keyConfidenceDisplay = document.getElementById('keyConfidence');
        this.keyDetectionModeSelect = document.getElementById('keyDetectionMode');
        this.manualKeySelector = document.getElementById('manualKeySelector');
        this.manualKeySelect = document.getElementById('manualKey');
        this.manualModeSelect = document.getElementById('manualMode');
        this.applyManualKeyBtn = document.getElementById('applyManualKey');
        this.chordFunctionDisplay = document.getElementById('chordFunctionValue');
        this.chordRoleDisplay = document.getElementById('chordRoleValue');
        
        // Performance tracking
        this.lastProcessingTime = 0;
        this.frameCount = 0;
        this.totalProcessingTime = 0;
        
        // Audio file info
        this.audioInfo = null;
        
        // Musical chord progression stabilization
        this.lastChordUpdateTime = 0;
        this.minChordChangeInterval = 800; // 800ms minimum between chord changes
        this.chordTransitionModel = this.createChordTransitionModel();
        this.chordHistory = []; // Recent chord history for context
        this.maxChordHistory = 5;
        
        // Initialize
        this.init();
    }

    async init() {
        try {
            console.log('Initializing Chordlock Audio Demo...');
            
            // Initialize core modules
            await this.initializeModules();
            
            // Load Chordlock WASM
            await this.loadChordlock();
            
            // Setup event listeners
            this.setupEventListeners();
            
            // Hide loading overlay
            this.hideLoading();
            
            console.log('Chordlock Audio Demo initialized successfully');
            
        } catch (error) {
            console.error('Failed to initialize demo:', error);
            this.showError('Failed to initialize audio demo: ' + error.message);
        }
    }

    async initializeModules() {
        // Initialize audio analyzer
        this.audioAnalyzer = new AudioAnalyzer();
        await this.audioAnalyzer.initialize();
        
        // Initialize note detector
        this.noteDetector = new NoteDetector();
        
        // Initialize chroma-based chord detector
        this.chromaDetector = new ChromaChordDetector();
        
        // Initialize key analyzer
        this.keyAnalyzer = new KeyAnalyzer();
        
        // Initialize spectrum visualizer
        this.spectrumVisualizer = new SpectrumVisualizer('spectrumCanvas');
        
        // Initialize chord display
        this.chordDisplay = new ChordDisplay();
        
        // Set up audio analyzer callbacks
        this.audioAnalyzer.onDataUpdate = (data) => this.processAudioData(data);
        this.audioAnalyzer.onPlayStateChange = (isPlaying) => this.updatePlayState(isPlaying);
    }

    async loadChordlock() {
        try {
            console.log('Loading ChordlockModule...');
            
            // Load ChordlockModule (which is defined in chordlock.js)
            if (typeof ChordlockModule === 'function') {
                const module = await ChordlockModule();
                
                // Initialize Chordlock
                module._chordlock_init();
                
                console.log('Available module methods:', Object.keys(module).filter(k => k.includes('chordlock')));
                console.log('Module ccall available:', typeof module.ccall);
                
                // Create real Chordlock wrapper with proper detection
                this.chordlock = {
                    module: module,
                    reset: () => {
                        try {
                            module._chordlock_clear_all_notes();
                            console.log('Chordlock reset successful');
                        } catch (e) {
                            console.log('Reset failed:', e);
                        }
                    },
                    noteOn: (midiNote, velocity = 80) => {
                        try {
                            module._chordlock_note_on(midiNote, velocity);
                            console.log(`Chordlock noteOn: ${midiNote}, velocity: ${velocity}`);
                        } catch (e) {
                            console.log('NoteOn failed:', e);
                        }
                    },
                    noteOff: (midiNote) => {
                        try {
                            module._chordlock_note_off(midiNote);
                        } catch (e) {
                            console.log('NoteOff failed:', e);
                        }
                    },
                    detectChord: () => {
                        try {
                            // Try using ccall for string results
                            if (module.ccall) {
                                const jsonResult = module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [1]);
                                console.log('Chordlock raw JSON:', jsonResult);
                                
                                if (jsonResult) {
                                    const parsed = JSON.parse(jsonResult);
                                    return {
                                        chordName: parsed.chord || 'Unknown',
                                        confidence: parsed.confidence || 0,
                                        hasValidChord: parsed.hasValidChord || false
                                    };
                                }
                                
                                return { chordName: 'Unknown', confidence: 0, hasValidChord: false };
                            } else {
                                console.log('No ccall available, using mock');
                                return { chordName: 'Unknown', confidence: 0.5, hasValidChord: false };
                            }
                        } catch (error) {
                            console.error('detectChord error:', error);
                            return { chordName: 'Error', confidence: 0, hasValidChord: false };
                        }
                    },
                    detectChordDetailed: (maxResults = 5) => {
                        try {
                            if (module.ccall) {
                                const jsonResult = module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [maxResults]);
                                console.log('Chordlock detailed JSON:', jsonResult);
                                
                                if (jsonResult) {
                                    const parsed = JSON.parse(jsonResult);
                                    return parsed; // Return the full JSON object
                                }
                                
                                return null;
                            } else {
                                return null;
                            }
                        } catch (error) {
                            console.error('detectChordDetailed error:', error);
                            return null;
                        }
                    }
                };
                
                console.log('Chordlock WASM loaded with mock detection');
                return true;
            } else {
                throw new Error('ChordlockModule not found');
            }
        } catch (error) {
            console.error('Failed to load Chordlock:', error);
            throw new Error('Failed to initialize Chordlock WASM: ' + error.message);
        }
    }

    setupEventListeners() {
        // Audio file input
        this.audioFileInput.addEventListener('change', (e) => this.handleFileUpload(e));
        
        // Playback controls
        this.playBtn.addEventListener('click', () => this.play());
        this.pauseBtn.addEventListener('click', () => this.pause());
        this.stopBtn.addEventListener('click', () => this.stop());
        
        // Settings sliders
        this.sensitivitySlider.addEventListener('input', (e) => this.updateSensitivity(e.target.value));
        this.smoothingSlider.addEventListener('input', (e) => this.updateSmoothing(e.target.value));
        this.thresholdSlider.addEventListener('input', (e) => this.updateThreshold(e.target.value));
        
        // Key analysis controls
        this.keyDetectionModeSelect.addEventListener('change', (e) => this.updateKeyDetectionMode(e.target.value));
        this.applyManualKeyBtn.addEventListener('click', () => this.applyManualKey());
        
        // Window resize
        window.addEventListener('resize', () => this.handleResize());
    }

    async handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;
        
        try {
            console.log(`Loading audio file: ${file.name}`);
            this.showLoading('Loading audio file...');
            
            // Load audio file
            this.audioInfo = await this.audioAnalyzer.loadAudioFile(file);
            
            // Update UI
            this.playBtn.disabled = false;
            this.stopBtn.disabled = false;
            this.sampleRateDisplay.textContent = `${this.audioInfo.sampleRate}Hz`;
            this.audioTimeDisplay.textContent = `00:00 / ${this.formatTime(this.audioInfo.duration)}`;
            
            this.hideLoading();
            console.log('Audio file loaded successfully');
            
        } catch (error) {
            console.error('Failed to load audio file:', error);
            this.showError('Failed to load audio file: ' + error.message);
            this.hideLoading();
        }
    }

    play() {
        if (this.audioAnalyzer) {
            this.audioAnalyzer.play();
        }
    }

    pause() {
        if (this.audioAnalyzer) {
            this.audioAnalyzer.pause();
        }
    }

    stop() {
        if (this.audioAnalyzer) {
            this.audioAnalyzer.stop();
        }
        
        // Clear displays
        this.chordDisplay.clear();
        this.spectrumVisualizer.clear();
        this.resetStats();
    }

    processAudioData(data) {
        const startTime = performance.now();
        
        try {
            // Update spectrum visualization
            this.spectrumVisualizer.update(data.frequencyData, data.sampleRate);
            
            // Step 1: Get velocity-aware notes directly from FFT (NEW APPROACH)
            const velocityAwareNotes = this.chromaDetector.computeVelocityAwareNotes(data.frequencyData, data.sampleRate);
            
            // Step 2: Filter and prepare notes for Chordlock with proper velocities
            const chordlockNotes = this.prepareNotesForChordlock(velocityAwareNotes);
            
            // Step 3: Send velocity-weighted notes to Chordlock
            const chordlockResult = this.sendVelocityNotesToChordlock(chordlockNotes);
            
            // Step 4: Fallback to chroma method if velocity method fails
            let fallbackResult = null;
            if (!chordlockResult || chordlockResult.chordName === 'Unknown') {
                const chromaVector = this.chromaDetector.computeChromaVector(data.frequencyData, data.sampleRate);
                const chromaActiveNotes = this.chromaToActiveNotes(chromaVector);
                fallbackResult = this.sendNotesToChordlock(chromaActiveNotes);
            }
            
            // Step 5: Key analysis
            const chromaVector = this.chromaDetector.computeChromaVector(data.frequencyData, data.sampleRate);
            const currentChordProgression = this.getRecentChordProgression();
            const keyAnalysis = this.keyAnalyzer.analyzeKey(chromaVector, currentChordProgression);
            
            // Step 6: Update display with best available result
            const finalResult = chordlockResult && chordlockResult.chordName !== 'Unknown' ? chordlockResult : fallbackResult;
            
            if (finalResult && finalResult.chordName !== 'Unknown') {
                // Add chord to progression history
                this.addToChordProgression(finalResult.chordName);
                this.updateChordDisplayWithChordlock(finalResult, [], keyAnalysis);
            } else {
                // Last resort: use chroma-only detection
                const chromaResult = this.chromaDetector.applyStabilityFilter(
                    this.chromaDetector.matchChordTemplates(chromaVector)
                );
                const detailedAnalysis = this.chromaDetector.getDetailedAnalysis(chromaVector);
                if (chromaResult?.name) {
                    this.addToChordProgression(chromaResult.name);
                }
                this.updateChordDisplay(chromaResult, detailedAnalysis, keyAnalysis);
            }
            
            // Step 7: Update key display
            this.updateKeyDisplay(keyAnalysis);
            
            // Update statistics with velocity-aware notes
            this.updateStats(chordlockNotes, performance.now() - startTime);
            
        } catch (error) {
            console.error('Audio processing error:', error);
        }
    }
    
    prepareNotesForChordlock(velocityAwareNotes) {
        const threshold = this.activeNotesThreshold || 0.05;
        
        // Filter by threshold and limit to reasonable chord size
        const filteredNotes = velocityAwareNotes
            .filter(note => note.strength > threshold)
            .slice(0, 6); // Max 6 notes for reasonable chords
        
        // Convert to format expected by Chordlock
        return filteredNotes.map(note => ({
            midiNote: note.midiNote,
            noteName: this.midiNoteToName(note.midiNote),
            strength: note.strength,
            velocity: note.velocity, // This is the key improvement - real velocity!
            frequency: note.frequency,
            octave: note.octave,
            chromaClass: note.chromaClass
        }));
    }
    
    sendVelocityNotesToChordlock(velocityNotes) {
        if (!this.chordlock) {
            console.log('Chordlock not available for velocity notes');
            return null;
        }
        
        try {
            // Debug: Log velocity information (more frequently for debugging)
            if (Math.random() < 0.3) {
                console.log('Velocity-aware notes to Chordlock:', 
                    velocityNotes.map(n => `${n.noteName} (vel: ${n.velocity}, str: ${n.strength.toFixed(3)})`));
            }
            
            // Clear previous notes
            this.chordlock.reset();
            
            // Send notes with their calculated velocities
            velocityNotes.forEach(note => {
                this.chordlock.noteOn(note.midiNote, note.velocity);
            });
            
            // Get chord detection result
            const chordResult = this.chordlock.detectChord();
            const detailedResult = this.chordlock.detectChordDetailed(5);
            
            if (Math.random() < 0.1) {
                console.log('Velocity-based Chordlock result:', chordResult);
            }
            
            return {
                chordName: chordResult.chordName,
                confidence: chordResult.confidence,
                hasValidChord: chordResult.hasValidChord,
                detailed: detailedResult,
                activeNotes: velocityNotes,
                method: 'velocity-aware'
            };
            
        } catch (error) {
            console.error('Velocity Chordlock detection error:', error);
            return null;
        }
    }
    
    chromaToActiveNotes(chromaVector) {
        const activeNotes = [];
        const threshold = this.activeNotesThreshold || 0.05; // Use UI threshold
        
        // Debug: Log chroma vector (less frequent)
        if (Math.random() < 0.1) { // Only 10% of the time to reduce spam
            console.log('Chroma vector:', chromaVector.map((val, i) => 
                `${['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][i]}: ${val.toFixed(3)}`
            ));
        }
        
        // Sort by strength and take only the strongest notes
        const chromaWithIndex = chromaVector.map((strength, index) => ({ strength, index }))
            .filter(item => item.strength > threshold)
            .sort((a, b) => b.strength - a.strength);
        
        // Limit to top 4 notes for cleaner chord detection
        const topNotes = chromaWithIndex.slice(0, 4);
        
        // Convert chroma strengths to MIDI notes
        for (const item of topNotes) {
            const { strength, index } = item;
            
            // Use middle octave (C4 = 60) as base
            const midiNote = 60 + index; // C4, C#4, D4, etc.
            const velocity = Math.max(1, Math.round(strength * 127)); // Ensure non-zero velocity
            
            activeNotes.push({
                midiNote: midiNote,
                noteName: this.midiNoteToName(midiNote),
                strength: strength,
                velocity: velocity
            });
        }
        
        if (Math.random() < 0.1) { // Log occasionally
            console.log('Converted to active notes:', activeNotes.length, 'notes');
        }
        return activeNotes;
    }
    
    midiNoteToName(midiNote) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor(midiNote / 12) - 1;
        const noteIndex = midiNote % 12;
        return noteNames[noteIndex] + octave;
    }
    
    sendNotesToChordlock(activeNotes) {
        if (!this.chordlock) {
            console.log('Chordlock not available, using fallback');
            return null;
        }
        
        try {
            // Debug: Log input notes
            console.log('Active notes to Chordlock:', activeNotes.map(n => `${n.noteName} (${n.velocity})`));
            
            // Clear previous notes
            this.chordlock.reset();
            
            // Send active notes to Chordlock
            activeNotes.forEach(note => {
                console.log(`Sending noteOn: ${note.midiNote} (${note.noteName}) velocity: ${note.velocity}`);
                this.chordlock.noteOn(note.midiNote, note.velocity);
            });
            
            // Get chord detection result
            const chordResult = this.chordlock.detectChord();
            const detailedResult = this.chordlock.detectChordDetailed(5);
            
            console.log('Chordlock result:', chordResult);
            console.log('Chordlock detailed:', detailedResult);
            
            return {
                chordName: chordResult.chordName,
                confidence: chordResult.confidence,
                detailed: detailedResult,
                activeNotes: activeNotes
            };
            
        } catch (error) {
            console.error('Chordlock detection error:', error);
            return null;
        }
    }
    
    
    shouldUpdateChordWithHysteresis(newResult) {
        // Initialize hysteresis state if needed
        if (!this.lastChordResult) {
            this.stableFrameCount = 0;
            this.lastChordResult = newResult;
            return true;
        }
        
        const CHANGE_THRESHOLD = 0.08; // 8% confidence difference required
        const MIN_STABLE_FRAMES = 8;   // 133ms at 60fps
        
        // Check if this is the same chord as before
        const isSameChord = (this.lastChordResult.chordName === newResult.chordName);
        
        if (isSameChord) {
            // Same chord: increment stability counter
            this.stableFrameCount++;
            return this.stableFrameCount >= MIN_STABLE_FRAMES;
        } else {
            // Different chord: check confidence difference
            const confidenceDiff = newResult.confidence - this.lastChordResult.confidence;
            
            if (confidenceDiff > CHANGE_THRESHOLD) {
                // Significant confidence improvement: allow change but require stability
                this.stableFrameCount = 1;
                return false; // Don't update yet, wait for stability
            } else {
                // Insufficient confidence difference: ignore this change
                this.stableFrameCount = Math.max(0, this.stableFrameCount - 1);
                return false;
            }
        }
    }
    
    parseChordlockJsonDetailed(detailedJson) {
        if (!detailedJson || !detailedJson.detailedCandidates) {
            return [];
        }
        
        const candidates = detailedJson.detailedCandidates.map((candidate, index) => ({
            rank: index + 1,
            name: candidate.name,
            confidence: candidate.confidence,
            root: candidate.root,
            isInversion: candidate.isInversion,
            matchScore: candidate.matchScore
        }));
        
        return candidates.slice(0, 5); // Top 5 candidates
    }
    
    parseChordlockDetailed(detailedString) {
        const candidates = [];
        const lines = detailedString.split('\n');
        
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

    updatePlayState(isPlaying) {
        this.playBtn.disabled = isPlaying;
        this.pauseBtn.disabled = !isPlaying;
        
        if (!isPlaying) {
            this.audioTimeDisplay.textContent = this.audioInfo ? 
                `00:00 / ${this.formatTime(this.audioInfo.duration)}` : '00:00 / 00:00';
            this.progressBar.value = 0;
        }
    }

    updateSensitivity(value) {
        const sensitivity = parseFloat(value);
        this.noteDetector.setSensitivity(sensitivity);
        // Also update chroma detector sensitivity
        if (this.chromaDetector) {
            this.chromaDetector.minChordDuration = 0.3 - (sensitivity - 1.0) * 0.1;
            this.chromaDetector.requiredStability = Math.max(1, Math.round(4 - sensitivity));
        }
        this.sensitivityValue.textContent = sensitivity.toFixed(1);
    }

    updateSmoothing(value) {
        const smoothing = parseFloat(value);
        this.noteDetector.setSmoothing(smoothing);
        this.audioAnalyzer.setSmoothing(smoothing);
        // Update chroma detector smoothing
        if (this.chromaDetector) {
            this.chromaDetector.chromaSmoothing = smoothing;
        }
        this.smoothingValue.textContent = smoothing.toFixed(1);
    }

    updateThreshold(value) {
        const threshold = parseFloat(value);
        this.noteDetector.setThreshold(threshold);
        // Update active notes threshold for Chordlock
        this.activeNotesThreshold = threshold;
        this.thresholdValue.textContent = threshold.toFixed(2);
    }

    updateStats(activeNotes, processingTime) {
        this.frameCount++;
        this.totalProcessingTime += processingTime;
        
        // Update processing time (rolling average over last 60 frames)
        const windowSize = 60;
        if (this.frameCount % windowSize === 0) {
            const avgProcessingTime = this.totalProcessingTime / windowSize;
            this.processingTimeDisplay.textContent = `${avgProcessingTime.toFixed(2)}ms`;
            this.totalProcessingTime = 0;
        }
        
        // Update active notes count
        this.activeNotesDisplay.textContent = activeNotes.length.toString();
    }

    resetStats() {
        this.frameCount = 0;
        this.totalProcessingTime = 0;
        this.processingTimeDisplay.textContent = '—';
        this.activeNotesDisplay.textContent = '0';
    }

    formatTime(seconds) {
        const mins = Math.floor(seconds / 60);
        const secs = Math.floor(seconds % 60);
        return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }

    showLoading(message = 'Loading...') {
        this.loadingOverlay.style.display = 'flex';
        this.loadingOverlay.querySelector('p').textContent = message;
    }

    hideLoading() {
        this.loadingOverlay.style.display = 'none';
    }

    showError(message) {
        alert(message); // Simple error display - could be enhanced with custom modal
    }

    handleResize() {
        // Resize visualizations
        if (this.spectrumVisualizer) {
            this.spectrumVisualizer.resize();
        }
        if (this.chordDisplay) {
            this.chordDisplay.resize();
        }
    }

    // Export functionality
    exportCurrentState() {
        const state = {
            currentChord: this.chordDisplay.currentChord,
            confidence: this.chordDisplay.currentConfidence,
            activeNotes: this.chordDisplay.activeNotes,
            chromaticProfile: this.chordDisplay.getChromaticDistribution(),
            settings: {
                sensitivity: this.noteDetector.sensitivity,
                smoothing: this.noteDetector.smoothingFactor,
                threshold: this.noteDetector.threshold
            },
            timestamp: new Date().toISOString()
        };
        
        return JSON.stringify(state, null, 2);
    }

    exportImages() {
        return {
            spectrum: this.spectrumVisualizer.exportImage(),
            piano: this.chordDisplay.exportPianoImage()
        };
    }

    // Debug functionality
    getDebugInfo() {
        return {
            audioAnalyzer: {
                isInitialized: this.audioAnalyzer?.isInitialized,
                isPlaying: this.audioAnalyzer?.isPlaying,
                sampleRate: this.audioAnalyzer?.audioContext?.sampleRate,
                fftSize: this.audioAnalyzer?.fftSize
            },
            noteDetector: {
                threshold: this.noteDetector.threshold,
                sensitivity: this.noteDetector.sensitivity,
                smoothing: this.noteDetector.smoothingFactor,
                activeNotes: this.noteDetector.noteStates.filter(state => state).length
            },
            chordlock: {
                loaded: !!this.chordlock,
                version: this.chordlock?.chordlock_get_version?.() || 'unknown'
            },
            performance: {
                frameCount: this.frameCount,
                avgProcessingTime: this.totalProcessingTime / Math.max(this.frameCount, 1)
            }
        };
    }

    // Musical Chord Transition Model
    createChordTransitionModel() {
        // Simplified chord transition probabilities based on music theory
        // Key relationships: tonic, dominant, subdominant, and relative chords
        const transitions = {
            // Major chords
            'C': { 'F': 0.3, 'G': 0.3, 'Am': 0.2, 'Dm': 0.15, 'Em': 0.05 },
            'F': { 'C': 0.4, 'G': 0.2, 'Dm': 0.2, 'Am': 0.1, 'Bb': 0.1 },
            'G': { 'C': 0.5, 'F': 0.2, 'Am': 0.15, 'Em': 0.1, 'Dm': 0.05 },
            
            // Minor chords
            'Am': { 'F': 0.3, 'C': 0.25, 'G': 0.2, 'Dm': 0.15, 'Em': 0.1 },
            'Dm': { 'G': 0.3, 'C': 0.25, 'F': 0.2, 'Am': 0.15, 'Bb': 0.1 },
            'Em': { 'Am': 0.3, 'C': 0.25, 'D': 0.2, 'G': 0.15, 'F': 0.1 },
            
            // 7th chords
            'G7': { 'C': 0.6, 'F': 0.2, 'Am': 0.15, 'Dm': 0.05 },
            'D7': { 'G': 0.6, 'C': 0.2, 'Em': 0.15, 'Am': 0.05 },
            'F7': { 'Bb': 0.6, 'C': 0.2, 'Dm': 0.15, 'Gm': 0.05 }
        };
        
        return transitions;
    }

    // Evaluate chord change based on musical context
    musicallyValidateChordChange(newChord, oldChord, confidence) {
        const currentTime = performance.now();
        
        // Rule 1: Minimum time interval between changes
        if (currentTime - this.lastChordUpdateTime < this.minChordChangeInterval) {
            return false;
        }
        
        // Rule 2: Require minimum confidence for any change
        if (confidence < 0.4) {
            return false;
        }
        
        // Rule 3: If no previous chord, allow change
        if (!oldChord) {
            this.lastChordUpdateTime = currentTime;
            return true;
        }
        
        // Rule 4: Same chord - always allow (no change needed)
        if (newChord === oldChord) {
            return true;
        }
        
        // Rule 5: Check transition probability
        const rootChord = this.extractRootChord(oldChord);
        const newRootChord = this.extractRootChord(newChord);
        
        if (this.chordTransitionModel[rootChord] && this.chordTransitionModel[rootChord][newRootChord]) {
            const transitionProbability = this.chordTransitionModel[rootChord][newRootChord];
            const confidenceBonus = Math.max(0, confidence - 0.5) * 2; // Bonus for high confidence
            
            if (transitionProbability + confidenceBonus > 0.3) {
                this.lastChordUpdateTime = currentTime;
                return true;
            }
        }
        
        // Rule 6: Allow change if confidence is very high (>0.8)
        if (confidence > 0.8) {
            this.lastChordUpdateTime = currentTime;
            return true;
        }
        
        return false;
    }

    // Extract root chord from complex chord names
    extractRootChord(chordName) {
        if (!chordName) return '';
        
        // Handle slash chords (e.g., "Am/C" -> "Am")
        const slashIndex = chordName.indexOf('/');
        const baseChord = slashIndex > -1 ? chordName.substring(0, slashIndex) : chordName;
        
        // Extract root note and quality (e.g., "Am7" -> "Am", "C" -> "C")
        const match = baseChord.match(/^([A-G][b#]?)(m|maj|dim|aug)?/);
        if (match) {
            const root = match[1];
            const quality = match[2] || '';
            return root + quality;
        }
        
        return baseChord;
    }

    // Key analysis helper methods
    getRecentChordProgression() {
        return this.progressionHistory.slice(-8); // Last 8 chords
    }

    addToChordProgression(chordName) {
        this.progressionHistory.push(chordName);
        if (this.progressionHistory.length > this.maxProgressionHistory) {
            this.progressionHistory.shift();
        }
    }

    updateKeyDisplay(keyAnalysis) {
        if (keyAnalysis && keyAnalysis.key) {
            this.currentKeyDisplay.textContent = `${keyAnalysis.key} ${keyAnalysis.mode}`;
            this.keySignatureDisplay.textContent = this.keyAnalyzer.getKeySignature(keyAnalysis.key, keyAnalysis.mode);
            this.keyConfidenceDisplay.textContent = `${(keyAnalysis.confidence * 100).toFixed(1)}%`;
        } else {
            this.currentKeyDisplay.textContent = '—';
            this.keySignatureDisplay.textContent = '';
            this.keyConfidenceDisplay.textContent = '—';
        }
    }

    updateKeyDetectionMode(mode) {
        this.keyAnalyzer.keyDetectionMode = mode;
        
        // Show/hide manual key selector
        if (mode === 'manual') {
            this.manualKeySelector.style.display = 'block';
        } else {
            this.manualKeySelector.style.display = 'none';
            this.keyAnalyzer.enableAutoKeyDetection();
        }
    }

    applyManualKey() {
        const keyName = this.manualKeySelect.value;
        const mode = this.manualModeSelect.value;
        this.keyAnalyzer.setManualKey(keyName, mode);
        this.updateKeyDisplay({ key: keyName, mode: mode, confidence: 1.0 });
    }

    updateChordDisplayWithChordlock(chordlockResult, chromaAnalysis, keyAnalysis = null) {
        // Use musical chord progression validation instead of simple hysteresis
        const currentDisplayedChord = this.chordDisplay.currentChordElement.textContent;
        const shouldUpdateChord = this.musicallyValidateChordChange(
            chordlockResult.chordName, 
            currentDisplayedChord, 
            chordlockResult.confidence
        );
        
        if (!shouldUpdateChord) {
            // Keep displaying the previous stable chord
            if (Math.random() < 0.05) { // Occasional debug log
                console.log(`Musical validation rejected: ${currentDisplayedChord} -> ${chordlockResult.chordName} (conf: ${chordlockResult.confidence.toFixed(3)})`);
            }
            return;
        }
        
        // Update current chord from Chordlock
        this.chordDisplay.currentChordElement.textContent = chordlockResult.chordName;
        this.chordDisplay.confidenceElement.textContent = `${(chordlockResult.confidence * 100).toFixed(1)}%`;
        
        // Update chord function analysis
        if (keyAnalysis && keyAnalysis.key) {
            const chordFunction = this.keyAnalyzer.getChordFunction(chordlockResult.chordName);
            this.chordFunctionDisplay.textContent = chordFunction.function;
            this.chordRoleDisplay.textContent = chordFunction.role;
        } else {
            this.chordFunctionDisplay.textContent = '—';
            this.chordRoleDisplay.textContent = '—';
        }
        
        // Set confidence class
        const confidenceClass = this.getConfidenceClass(chordlockResult.confidence);
        this.chordDisplay.currentChordElement.className = `chord-name ${confidenceClass} chordlock-result`;
        
        // Parse JSON detailed results for candidates
        const chordlockCandidates = this.parseChordlockJsonDetailed(chordlockResult.detailed);
        
        // Update candidates list with Chordlock results
        this.chordDisplay.candidatesListElement.innerHTML = chordlockCandidates.map((candidate, index) => `
            <div class="candidate-item ${index === 0 ? 'top-candidate' : ''} chordlock-candidate">
                <span class="candidate-name">${candidate.name}</span>
                <span class="candidate-score">${(candidate.confidence * 100).toFixed(1)}%</span>
            </div>
        `.trim()).join('');
        
        // Update piano visualization with active notes
        if (chordlockResult.activeNotes) {
            this.chordDisplay.activeNotes = chordlockResult.activeNotes;
            this.chordDisplay.drawPiano();
        }
        
        // Store for hysteresis comparison
        this.lastChordResult = chordlockResult;
    }

    updateChordDisplay(chromaResult, detailedAnalysis, keyAnalysis = null) {
        if (!chromaResult) return;
        
        // Update current chord
        this.chordDisplay.currentChordElement.textContent = chromaResult.name || 'Unknown';
        this.chordDisplay.confidenceElement.textContent = `${(chromaResult.confidence * 100).toFixed(1)}%`;
        
        // Update chord function analysis
        if (keyAnalysis && keyAnalysis.key && chromaResult.name) {
            const chordFunction = this.keyAnalyzer.getChordFunction(chromaResult.name);
            this.chordFunctionDisplay.textContent = chordFunction.function;
            this.chordRoleDisplay.textContent = chordFunction.role;
        } else {
            this.chordFunctionDisplay.textContent = '—';
            this.chordRoleDisplay.textContent = '—';
        }
        
        // Set confidence class
        const confidenceClass = this.getConfidenceClass(chromaResult.confidence);
        this.chordDisplay.currentChordElement.className = `chord-name ${confidenceClass}`;
        
        // Update candidates list
        this.chordDisplay.candidatesListElement.innerHTML = detailedAnalysis.map((candidate, index) => `
            <div class="candidate-item ${index === 0 ? 'top-candidate' : ''}">
                <span class="candidate-name">${candidate.name}</span>
                <span class="candidate-score">${(candidate.confidence * 100).toFixed(1)}%</span>
            </div>
        `.trim()).join('');
    }
}

// Initialize the application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.chordlockDemo = new ChordlockAudioDemo();
});

// Export for debugging
window.ChordlockAudioDemo = ChordlockAudioDemo;