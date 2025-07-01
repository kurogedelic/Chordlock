/**
 * Web MIDI API Integration for Real-time MIDI Input
 * Handles MIDI device detection and real-time note events
 */
class MidiInput {
    constructor() {
        this.midiAccess = null;
        this.inputDevices = new Map();
        this.isEnabled = false;
        this.onNoteOn = null;
        this.onNoteOff = null;
        this.activeNotes = new Set();
    }

    // Initialize Web MIDI API
    async initialize() {
        if (!navigator.requestMIDIAccess) {
            throw new Error('Web MIDI API not supported in this browser');
        }

        try {
            this.midiAccess = await navigator.requestMIDIAccess();
            console.log('MIDI access granted');

            // Set up device change listeners
            this.midiAccess.onstatechange = (event) => {
                this.handleDeviceChange(event);
            };

            // Scan for existing devices
            this.scanDevices();
            return true;

        } catch (error) {
            console.error('Failed to get MIDI access:', error);
            throw error;
        }
    }

    // Scan for available MIDI input devices
    scanDevices() {
        this.inputDevices.clear();
        
        for (const input of this.midiAccess.inputs.values()) {
            this.inputDevices.set(input.id, {
                id: input.id,
                name: input.name,
                manufacturer: input.manufacturer,
                connected: input.state === 'connected',
                device: input
            });

            console.log(`Found MIDI input: ${input.name} (${input.manufacturer})`);
        }

        // Auto-connect to first available device
        if (this.inputDevices.size > 0 && !this.isEnabled) {
            const firstDevice = this.inputDevices.values().next().value;
            this.connectDevice(firstDevice.id);
        }
    }

    // Connect to a specific MIDI device
    connectDevice(deviceId) {
        const deviceInfo = this.inputDevices.get(deviceId);
        if (!deviceInfo || !deviceInfo.connected) {
            console.warn('Device not available:', deviceId);
            return false;
        }

        // Disconnect previous device
        this.disconnect();

        const device = deviceInfo.device;
        device.onmidimessage = (event) => {
            this.handleMidiMessage(event);
        };

        this.isEnabled = true;
        console.log(`Connected to MIDI device: ${device.name}`);
        return true;
    }

    // Disconnect from current device
    disconnect() {
        if (this.midiAccess) {
            for (const input of this.midiAccess.inputs.values()) {
                input.onmidimessage = null;
            }
        }
        this.isEnabled = false;
        this.activeNotes.clear();
        console.log('Disconnected from MIDI devices');
    }

    // Handle device connection changes
    handleDeviceChange(event) {
        console.log(`MIDI device ${event.port.state}: ${event.port.name}`);
        this.scanDevices();
    }

    // Handle incoming MIDI messages
    handleMidiMessage(event) {
        const [status, note, velocity] = event.data;
        const messageType = status & 0xF0;
        const channel = status & 0x0F;

        switch (messageType) {
            case 0x90: // Note On
                if (velocity > 0) {
                    this.handleNoteOn(note, velocity);
                } else {
                    this.handleNoteOff(note);
                }
                break;
                
            case 0x80: // Note Off
                this.handleNoteOff(note);
                break;
        }
    }

    // Handle note on events
    handleNoteOn(note, velocity) {
        this.activeNotes.add(note);
        
        if (this.onNoteOn) {
            this.onNoteOn(note, velocity);
        }

        // Also play the note through piano sound
        if (window.pianoSound) {
            window.pianoSound.playNote(note, velocity);
        }
    }

    // Handle note off events
    handleNoteOff(note) {
        this.activeNotes.delete(note);
        
        if (this.onNoteOff) {
            this.onNoteOff(note);
        }

        // Stop the note in piano sound
        if (window.pianoSound) {
            window.pianoSound.stopNote(note);
        }
    }

    // Get list of available devices
    getDevices() {
        return Array.from(this.inputDevices.values());
    }

    // Get current active notes
    getActiveNotes() {
        return Array.from(this.activeNotes);
    }

    // Get status information
    getStatus() {
        return {
            supported: !!navigator.requestMIDIAccess,
            initialized: !!this.midiAccess,
            enabled: this.isEnabled,
            deviceCount: this.inputDevices.size,
            activeNotes: this.activeNotes.size
        };
    }
}

// Create global instance
window.midiInput = new MidiInput();