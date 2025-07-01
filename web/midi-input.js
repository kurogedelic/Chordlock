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

        // Auto-connect to first available device if explicitly requested
        if (this.inputDevices.size > 0 && !this.isEnabled) {
            console.log(`Found ${this.inputDevices.size} MIDI devices, waiting for user selection`);
            // Don't auto-connect, let user choose
        }
    }

    // Connect to a specific MIDI device
    connectDevice(deviceId) {
        console.log('Attempting to connect to device:', deviceId);
        
        const deviceInfo = this.inputDevices.get(deviceId);
        if (!deviceInfo) {
            console.warn('Device not found:', deviceId);
            return false;
        }
        
        if (!deviceInfo.connected) {
            console.warn('Device not connected:', deviceInfo.name);
            return false;
        }

        // Disconnect previous device
        this.disconnect();

        const device = deviceInfo.device;
        
        // Open the device explicitly
        if (device.open) {
            device.open().then(() => {
                console.log('Device opened successfully:', device.name);
            }).catch(err => {
                console.error('Failed to open device:', err);
            });
        }
        
        // Set up message handler with enhanced debugging
        device.onmidimessage = (event) => {
            console.log('Raw MIDI received from device:', device.name);
            this.handleMidiMessage(event);
        };

        // Enhanced state change monitoring
        device.onstatechange = (event) => {
            console.log(`Device state changed: ${event.port.name} is ${event.port.state}`);
            if (event.port.state === 'disconnected') {
                this.handleDeviceDisconnection(event.port.id);
            }
        };

        this.isEnabled = true;
        this.connectedDeviceId = deviceId;
        console.log(`✅ Connected to MIDI device: ${device.name} (${device.manufacturer})`);
        console.log('Device info:', {
            name: device.name,
            manufacturer: device.manufacturer,
            version: device.version,
            state: device.state,
            connection: device.connection,
            type: device.type
        });
        
        // Test the connection
        this.testConnection(device);
        
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
        console.log(`MIDI device ${event.port.state}: ${event.port.name} (ID: ${event.port.id})`);
        console.log('Device details:', {
            name: event.port.name,
            manufacturer: event.port.manufacturer,
            state: event.port.state,
            connection: event.port.connection,
            type: event.port.type
        });
        
        this.scanDevices();
        
        // Update UI if there's a callback
        if (this.onDeviceChange) {
            this.onDeviceChange(event);
        }
    }

    // Handle incoming MIDI messages
    handleMidiMessage(event) {
        const [status, note, velocity] = event.data;
        const messageType = status & 0xF0;
        const channel = status & 0x0F;

        // Enhanced debug logging
        console.log('🎹 MIDI Message Received:', {
            timestamp: event.timeStamp,
            raw: Array.from(event.data),
            status: '0x' + status.toString(16).padStart(2, '0'),
            messageType: '0x' + messageType.toString(16).padStart(2, '0'),
            channel: channel + 1, // Human-readable channel (1-16)
            note: note,
            noteName: this.noteToName(note),
            velocity: velocity
        });

        // Update debug display if available
        if (window.updateMIDIDebug) {
            window.updateMIDIDebug(`${this.noteToName(note)} ${messageType === 0x90 && velocity > 0 ? 'ON' : 'OFF'} (ch${channel + 1})`);
        }

        switch (messageType) {
            case 0x90: // Note On
                if (velocity > 0) {
                    console.log(`🎵 Note ON: ${this.noteToName(note)} (${note}) vel=${velocity}`);
                    this.handleNoteOn(note, velocity);
                } else {
                    console.log(`🎵 Note OFF: ${this.noteToName(note)} (${note}) via note-on vel=0`);
                    this.handleNoteOff(note);
                }
                break;
                
            case 0x80: // Note Off
                console.log(`🎵 Note OFF: ${this.noteToName(note)} (${note})`);
                this.handleNoteOff(note);
                break;
                
            case 0xB0: // Control Change
                console.log(`🎛️ Control Change: CC${note}=${velocity} (ch${channel + 1})`);
                break;
                
            case 0xC0: // Program Change
                console.log(`🎨 Program Change: ${note} (ch${channel + 1})`);
                break;
                
            case 0xE0: // Pitch Bend
                const pitchValue = (velocity << 7) | note;
                console.log(`🎸 Pitch Bend: ${pitchValue} (ch${channel + 1})`);
                break;
                
            default:
                console.log(`❓ Unhandled MIDI message type: 0x${messageType.toString(16).padStart(2, '0')}`);
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

    // Convert MIDI note number to note name
    noteToName(noteNumber) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor(noteNumber / 12) - 1;
        const note = noteNames[noteNumber % 12];
        return `${note}${octave}`;
    }

    // Test connection by sending a test message or monitoring
    testConnection(device) {
        console.log('Testing MIDI connection for:', device.name);
        console.log('Waiting for MIDI messages... Try playing some notes on your MIDI device.');
        
        // Set a timeout to report if no messages received
        setTimeout(() => {
            if (this.activeNotes.size === 0) {
                console.warn('⚠️ No MIDI messages received in 5 seconds. Please check:');
                console.warn('1. MIDI device is powered on and connected');
                console.warn('2. Device is sending on correct MIDI channel');
                console.warn('3. Device is configured to send note messages');
            }
        }, 5000);
    }

    // Handle device disconnection
    handleDeviceDisconnection(deviceId) {
        if (this.connectedDeviceId === deviceId) {
            console.log('Connected device was disconnected');
            this.isEnabled = false;
            this.connectedDeviceId = null;
            this.activeNotes.clear();
        }
    }

    // Get detailed status information
    getStatus() {
        return {
            supported: !!navigator.requestMIDIAccess,
            initialized: !!this.midiAccess,
            enabled: this.isEnabled,
            deviceCount: this.inputDevices.size,
            activeNotes: this.activeNotes.size,
            connectedDevice: this.connectedDeviceId,
            midiAccess: this.midiAccess ? {
                inputs: this.midiAccess.inputs.size,
                outputs: this.midiAccess.outputs.size,
                sysexEnabled: this.midiAccess.sysexEnabled
            } : null
        };
    }
}

// Create global instance
window.midiInput = new MidiInput();