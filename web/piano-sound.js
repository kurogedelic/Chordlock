/**
 * Simple Piano Sound Engine using Web Audio API
 * Generates basic sine wave tones for piano notes
 */
class PianoSound {
    constructor() {
        this.audioContext = null;
        this.gainNode = null;
        this.activeNotes = new Map(); // noteNumber -> { oscillator, gainNode }
        this.isMuted = false;
        this.masterVolume = 0.3;
        this.initialized = false;
    }

    async initialize() {
        if (this.initialized) return;
        
        try {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            
            // Create master gain node
            this.gainNode = this.audioContext.createGain();
            this.gainNode.connect(this.audioContext.destination);
            this.gainNode.gain.setValueAtTime(this.masterVolume, this.audioContext.currentTime);
            
            this.initialized = true;
            console.log('Piano sound engine initialized');
        } catch (error) {
            console.error('Failed to initialize audio context:', error);
        }
    }

    // Convert MIDI note number to frequency
    noteToFrequency(noteNumber) {
        return 440 * Math.pow(2, (noteNumber - 69) / 12);
    }

    // Play a note
    async playNote(noteNumber, velocity = 80) {
        if (!this.initialized) await this.initialize();
        if (this.isMuted || !this.audioContext) return;

        // Stop existing note if playing
        this.stopNote(noteNumber);

        try {
            const frequency = this.noteToFrequency(noteNumber);
            const volume = (velocity / 127) * this.masterVolume;

            // Create oscillator for the note
            const oscillator = this.audioContext.createOscillator();
            const noteGain = this.audioContext.createGain();

            // Simple piano-like sound using multiple harmonics
            oscillator.type = 'sine';
            oscillator.frequency.setValueAtTime(frequency, this.audioContext.currentTime);

            // Volume envelope (attack/decay)
            noteGain.gain.setValueAtTime(0, this.audioContext.currentTime);
            noteGain.gain.linearRampToValueAtTime(volume, this.audioContext.currentTime + 0.01); // Quick attack
            noteGain.gain.exponentialRampToValueAtTime(volume * 0.7, this.audioContext.currentTime + 0.3); // Slight decay

            // Connect audio graph
            oscillator.connect(noteGain);
            noteGain.connect(this.gainNode);

            // Start the oscillator
            oscillator.start(this.audioContext.currentTime);

            // Store active note
            this.activeNotes.set(noteNumber, { oscillator, gainNode: noteGain });

        } catch (error) {
            console.error('Error playing note:', error);
        }
    }

    // Stop a note
    stopNote(noteNumber) {
        const note = this.activeNotes.get(noteNumber);
        if (!note) return;

        try {
            // Fade out and stop
            const { oscillator, gainNode } = note;
            const currentTime = this.audioContext.currentTime;
            
            gainNode.gain.cancelScheduledValues(currentTime);
            gainNode.gain.setValueAtTime(gainNode.gain.value, currentTime);
            gainNode.gain.linearRampToValueAtTime(0, currentTime + 0.1);
            
            oscillator.stop(currentTime + 0.1);
            
            // Clean up
            setTimeout(() => {
                this.activeNotes.delete(noteNumber);
            }, 150);
            
        } catch (error) {
            console.error('Error stopping note:', error);
            this.activeNotes.delete(noteNumber);
        }
    }

    // Stop all notes
    stopAllNotes() {
        for (const noteNumber of this.activeNotes.keys()) {
            this.stopNote(noteNumber);
        }
    }

    // Toggle mute
    setMuted(muted) {
        this.isMuted = muted;
        if (muted) {
            this.stopAllNotes();
        }
        
        if (this.gainNode) {
            const targetVolume = muted ? 0 : this.masterVolume;
            this.gainNode.gain.linearRampToValueAtTime(targetVolume, this.audioContext.currentTime + 0.1);
        }
    }

    // Set master volume
    setVolume(volume) {
        this.masterVolume = Math.max(0, Math.min(1, volume));
        if (this.gainNode && !this.isMuted) {
            this.gainNode.gain.linearRampToValueAtTime(this.masterVolume, this.audioContext.currentTime + 0.1);
        }
    }

    // Play a chord (array of note numbers)
    playChord(noteNumbers, velocity = 80) {
        noteNumbers.forEach(note => this.playNote(note, velocity));
    }

    // Stop a chord
    stopChord(noteNumbers) {
        noteNumbers.forEach(note => this.stopNote(note));
    }

    // Get audio context state
    getState() {
        return {
            initialized: this.initialized,
            muted: this.isMuted,
            volume: this.masterVolume,
            activeNotes: this.activeNotes.size,
            contextState: this.audioContext ? this.audioContext.state : 'none'
        };
    }
}

// Create global instance
window.pianoSound = new PianoSound();