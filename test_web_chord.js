const fs = require('fs');

// Load the WASM module
const wasmBuffer = fs.readFileSync('./demo/chordlock.wasm');

async function testChord() {
    try {
        // Create a minimal Module object
        const Module = {
            wasmBinary: wasmBuffer,
            onRuntimeInitialized: () => {
                console.log('WASM initialized');
                
                // Test the chord C5, D5, G5 (MIDI 72, 74, 79)
                console.log('Testing C5, D5, G5 (MIDI 72, 74, 79)...');
                
                // Initialize Chordlock
                Module._chordlock_init();
                
                // Clear all notes
                Module._chordlock_clear_all_notes();
                
                // Set notes
                Module._chordlock_note_on(72, 80);  // C5
                Module._chordlock_note_on(74, 80);  // D5  
                Module._chordlock_note_on(79, 80);  // G5
                
                // Get mask
                const mask = Module._chordlock_get_current_mask();
                console.log('Current mask:', mask, '(0x' + mask.toString(16) + ')');
                
                // Detect chord
                const resultPtr = Module._chordlock_detect_chord();
                const result = Module.UTF8ToString(resultPtr);
                console.log('Detection result:', result);
                
                // Detailed detection
                const detailedResultPtr = Module._chordlock_detect_chord_detailed(5);
                const detailedResult = Module.UTF8ToString(detailedResultPtr);
                console.log('Detailed result:', detailedResult);
                
                // Parse and show the primary chord
                try {
                    const parsed = JSON.parse(result);
                    console.log('Primary chord detected:', parsed.chord);
                    console.log('Confidence:', parsed.confidence);
                    console.log('Alternatives:', parsed.alternatives);
                } catch(e) {
                    console.error('Failed to parse result:', e);
                }
                
                // Cleanup
                Module._chordlock_cleanup();
            }
        };
        
        // Load the JS wrapper which will initialize the WASM
        require('./demo/chordlock.js');
        
    } catch (error) {
        console.error('Error:', error);
    }
}

testChord();