/**
 * Audio Analyzer - Web Audio API based real-time audio analysis
 */
class AudioAnalyzer {
    constructor() {
        this.audioContext = null;
        this.source = null;
        this.analyser = null;
        this.gainNode = null;
        this.audioBuffer = null;
        
        this.fftSize = 4096;
        this.smoothingTimeConstant = 0.8;
        this.minDecibels = -90;
        this.maxDecibels = -10;
        
        this.frequencyData = null;
        this.timeData = null;
        
        this.isInitialized = false;
        this.isPlaying = false;
        
        // Callbacks
        this.onDataUpdate = null;
        this.onPlayStateChange = null;
    }

    async initialize() {
        try {
            // Create audio context
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            
            // Create analyser node
            this.analyser = this.audioContext.createAnalyser();
            this.analyser.fftSize = this.fftSize;
            this.analyser.smoothingTimeConstant = this.smoothingTimeConstant;
            this.analyser.minDecibels = this.minDecibels;
            this.analyser.maxDecibels = this.maxDecibels;
            
            // Create gain node for volume control
            this.gainNode = this.audioContext.createGain();
            
            // Initialize data arrays
            this.frequencyData = new Uint8Array(this.analyser.frequencyBinCount);
            this.timeData = new Uint8Array(this.analyser.fftSize);
            
            this.isInitialized = true;
            console.log('Audio Analyzer initialized');
            
            return true;
        } catch (error) {
            console.error('Failed to initialize Audio Analyzer:', error);
            return false;
        }
    }

    async loadAudioFile(file) {
        if (!this.isInitialized) {
            throw new Error('AudioAnalyzer not initialized');
        }

        try {
            // Stop current playback
            this.stop();
            
            // Read file as array buffer
            const arrayBuffer = await file.arrayBuffer();
            
            // Decode audio data
            this.audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
            
            // Don't create source here - create it when play() is called
            
            console.log(`Audio loaded: ${file.name}`);
            console.log(`Duration: ${this.audioBuffer.duration.toFixed(2)}s`);
            console.log(`Sample Rate: ${this.audioBuffer.sampleRate}Hz`);
            console.log(`Channels: ${this.audioBuffer.numberOfChannels}`);
            
            return {
                duration: this.audioBuffer.duration,
                sampleRate: this.audioBuffer.sampleRate,
                channels: this.audioBuffer.numberOfChannels
            };
            
        } catch (error) {
            console.error('Failed to load audio file:', error);
            throw error;
        }
    }

    play() {
        if (this.isPlaying) return;
        
        try {
            // Create new source node for each play
            this.createNewSource();
            
            // Resume audio context if suspended
            if (this.audioContext.state === 'suspended') {
                this.audioContext.resume();
            }
            
            this.source.start(0);
            this.isPlaying = true;
            
            // Start analysis loop
            this.startAnalysis();
            
            if (this.onPlayStateChange) {
                this.onPlayStateChange(true);
            }
            
            console.log('Audio playback started');
        } catch (error) {
            console.error('Failed to start playback:', error);
        }
    }
    
    createNewSource() {
        if (!this.audioBuffer) {
            throw new Error('No audio buffer available');
        }
        
        // Disconnect old source if exists
        if (this.source) {
            try {
                this.source.disconnect();
            } catch (e) {
                // Source might already be disconnected
            }
        }
        
        // Create new buffer source
        this.source = this.audioContext.createBufferSource();
        this.source.buffer = this.audioBuffer;
        
        // Connect audio graph: source -> gain -> analyser -> destination
        this.source.connect(this.gainNode);
        this.gainNode.connect(this.analyser);
        this.analyser.connect(this.audioContext.destination);
        
        // Set up event handlers
        this.source.onended = () => {
            this.isPlaying = false;
            if (this.onPlayStateChange) {
                this.onPlayStateChange(false);
            }
        };
    }

    pause() {
        if (!this.isPlaying) return;
        
        try {
            this.audioContext.suspend();
            console.log('Audio playback paused');
        } catch (error) {
            console.error('Failed to pause playback:', error);
        }
    }

    resume() {
        if (this.audioContext.state === 'suspended') {
            this.audioContext.resume();
            console.log('Audio playback resumed');
        }
    }

    stop() {
        if (this.source) {
            try {
                this.source.stop();
            } catch (error) {
                // Source might already be stopped
            }
            this.source = null;
        }
        
        this.isPlaying = false;
        this.stopAnalysis();
        
        if (this.onPlayStateChange) {
            this.onPlayStateChange(false);
        }
        
        console.log('Audio playback stopped');
    }

    startAnalysis() {
        if (this.analysisRunning) return;
        
        this.analysisRunning = true;
        const analyze = () => {
            if (!this.analysisRunning || !this.isPlaying) return;
            
            // Get frequency and time domain data
            this.analyser.getByteFrequencyData(this.frequencyData);
            this.analyser.getByteTimeDomainData(this.timeData);
            
            // Call data update callback
            if (this.onDataUpdate) {
                this.onDataUpdate({
                    frequencyData: this.frequencyData,
                    timeData: this.timeData,
                    sampleRate: this.audioContext.sampleRate,
                    fftSize: this.fftSize
                });
            }
            
            // Continue analysis
            requestAnimationFrame(analyze);
        };
        
        analyze();
    }

    stopAnalysis() {
        this.analysisRunning = false;
    }

    setVolume(volume) {
        if (this.gainNode) {
            this.gainNode.gain.value = Math.max(0, Math.min(1, volume));
        }
    }

    setSmoothing(smoothing) {
        if (this.analyser) {
            this.smoothingTimeConstant = smoothing;
            this.analyser.smoothingTimeConstant = smoothing;
        }
    }

    // Get current playback time (approximate)
    getCurrentTime() {
        if (!this.audioContext || !this.isPlaying) return 0;
        return this.audioContext.currentTime;
    }

    // Frequency to MIDI note conversion
    frequencyToMidiNote(frequency) {
        // A4 = 440Hz = MIDI note 69
        return Math.round(12 * Math.log2(frequency / 440) + 69);
    }

    // Get frequency for a given FFT bin
    getFrequencyForBin(bin) {
        return (bin * this.audioContext.sampleRate) / (2 * this.analyser.frequencyBinCount);
    }

    // Get the strongest frequencies
    getStrongestFrequencies(count = 10, threshold = 30) {
        if (!this.frequencyData) return [];
        
        const peaks = [];
        
        // Find peaks in the frequency data
        for (let i = 1; i < this.frequencyData.length - 1; i++) {
            const current = this.frequencyData[i];
            const prev = this.frequencyData[i - 1];
            const next = this.frequencyData[i + 1];
            
            // Check if it's a local maximum above threshold
            if (current > threshold && current > prev && current > next) {
                const frequency = this.getFrequencyForBin(i);
                const midiNote = this.frequencyToMidiNote(frequency);
                
                // Only consider notes in musical range (C1 to C8)
                if (midiNote >= 24 && midiNote <= 108) {
                    peaks.push({
                        frequency: frequency,
                        magnitude: current,
                        midiNote: midiNote,
                        bin: i
                    });
                }
            }
        }
        
        // Sort by magnitude and return top results
        return peaks
            .sort((a, b) => b.magnitude - a.magnitude)
            .slice(0, count);
    }

    destroy() {
        this.stop();
        
        if (this.audioContext) {
            this.audioContext.close();
            this.audioContext = null;
        }
        
        this.isInitialized = false;
        console.log('Audio Analyzer destroyed');
    }
}

// Export for use in other modules
window.AudioAnalyzer = AudioAnalyzer;