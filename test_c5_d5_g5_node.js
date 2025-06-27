const fs = require('fs');

async function testC5D5G5() {
    try {
        console.log('Loading WebAssembly module...');
        
        // Import the module from demo directory
        const ChordlockModule = require('./demo/chordlock.js');
        
        const module = await ChordlockModule();
        
        console.log('Initializing Chordlock...');
        module.ccall('chordlock_init', null, [], []);
        
        console.log('Testing C5, D5, G5 chord detection (MIDI 72, 74, 79)...\n');
        
        // Clear all notes first
        module.ccall('chordlock_clear_all_notes', null, [], []);
        
        // Set the notes: C5=72, D5=74, G5=79
        module.ccall('chordlock_note_on', null, ['number', 'number'], [72, 80]);
        module.ccall('chordlock_note_on', null, ['number', 'number'], [74, 80]);
        module.ccall('chordlock_note_on', null, ['number', 'number'], [79, 80]);
        
        // Get current mask
        const mask = module.ccall('chordlock_get_current_mask', 'number', [], []);
        console.log('Current pitch mask:', mask, '(0x' + mask.toString(16) + ')');
        console.log('Expected mask for C(0), D(2), G(7): 0x0085 = 133\n');
        
        // Detect chord with basic method
        const result = module.ccall('chordlock_detect_chord', 'string', [], []);
        console.log('Basic detection result:', result);
        
        // Parse and display the result
        try {
            const parsed = JSON.parse(result);
            console.log('\n=== PRIMARY DETECTION ===');
            console.log('Chord:', parsed.chord);
            console.log('Confidence:', parsed.confidence);
            console.log('Valid chord:', parsed.hasValidChord);
            
            if (parsed.alternatives && parsed.alternatives.length > 0) {
                console.log('\n=== ALTERNATIVES ===');
                parsed.alternatives.forEach((alt, index) => {
                    if (typeof alt === 'object') {
                        console.log(`${index + 1}. ${alt.name} (confidence: ${alt.confidence})`);
                    } else {
                        console.log(`${index + 1}. ${alt}`);
                    }
                });
            }
        } catch(e) {
            console.error('Failed to parse basic result:', e);
        }
        
        // Detect with detailed analysis
        console.log('\n=== DETAILED ANALYSIS ===');
        const detailedResult = module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
        console.log('Detailed result:', detailedResult);
        
        try {
            const detailedParsed = JSON.parse(detailedResult);
            if (detailedParsed.detailedCandidates && detailedParsed.detailedCandidates.length > 0) {
                console.log('\n=== DETAILED CANDIDATES ===');
                detailedParsed.detailedCandidates.forEach((candidate, index) => {
                    console.log(`${index + 1}. ${candidate.name}`);
                    console.log(`   Confidence: ${candidate.confidence}`);
                    console.log(`   Root: ${candidate.root}`);
                    console.log(`   Type: ${candidate.interpretationType}`);
                    console.log(`   Match Score: ${candidate.matchScore}`);
                    if (candidate.extraNotes && candidate.extraNotes.length > 0) {
                        console.log(`   Extra Notes: ${candidate.extraNotes}`);
                    }
                    if (candidate.missingNotes && candidate.missingNotes.length > 0) {
                        console.log(`   Missing Notes: ${candidate.missingNotes}`);
                    }
                    console.log('');
                });
            }
        } catch(e) {
            console.error('Failed to parse detailed result:', e);
        }
        
        // Test reverse lookup for comparison
        console.log('\n=== REVERSE LOOKUP COMPARISON ===');
        const csus2Result = module.ccall('chordlock_chord_name_to_notes_json', 'string', ['string', 'number'], ['Csus2', 5]);
        const gomit3Result = module.ccall('chordlock_chord_name_to_notes_json', 'string', ['string', 'number'], ['Gomit3', 5]);
        
        console.log('Csus2 notes:', csus2Result);
        console.log('Gomit3 notes:', gomit3Result);
        
        // Cleanup
        module.ccall('chordlock_cleanup', null, [], []);
        
        console.log('\nTest completed.');
        
    } catch (error) {
        console.error('Error testing chord:', error);
        process.exit(1);
    }
}

testC5D5G5();