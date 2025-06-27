const fs = require('fs');

async function compareBasicVsDetailed() {
    try {
        console.log('Loading WebAssembly module...');
        const ChordlockModule = require('./demo/chordlock.js');
        const module = await ChordlockModule();
        
        console.log('Initializing Chordlock...');
        module.ccall('chordlock_init', null, [], []);
        
        console.log('Testing C5, D5, G5 chord detection (MIDI 72, 74, 79)...\n');
        
        // Clear and set notes
        module.ccall('chordlock_clear_all_notes', null, [], []);
        module.ccall('chordlock_note_on', null, ['number', 'number'], [72, 80]);
        module.ccall('chordlock_note_on', null, ['number', 'number'], [74, 80]);
        module.ccall('chordlock_note_on', null, ['number', 'number'], [79, 80]);
        
        // Test BASIC detection (same as CLI)
        console.log('=== BASIC DETECTION (CLI method) ===');
        const basicResult = module.ccall('chordlock_detect_chord', 'string', [], []);
        const basicParsed = JSON.parse(basicResult);
        console.log('Primary Chord:', basicParsed.chord);
        console.log('Confidence:', basicParsed.confidence);
        console.log('Alternatives:');
        if (basicParsed.alternatives) {
            basicParsed.alternatives.forEach((alt, index) => {
                if (typeof alt === 'object') {
                    console.log(`  ${index + 1}. ${alt.name} (${alt.confidence})`);
                } else {
                    console.log(`  ${index + 1}. ${alt}`);
                }
            });
        }
        
        console.log('\n=== DETAILED DETECTION (web method) ===');
        const detailedResult = module.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
        const detailedParsed = JSON.parse(detailedResult);
        console.log('Primary Chord:', detailedParsed.chord);
        console.log('Confidence:', detailedParsed.confidence);
        console.log('Top Detailed Candidates:');
        if (detailedParsed.detailedCandidates) {
            detailedParsed.detailedCandidates.slice(0, 3).forEach((candidate, index) => {
                console.log(`  ${index + 1}. ${candidate.name} (${candidate.confidence}) [${candidate.interpretationType}]`);
            });
        }
        
        console.log('\n=== COMPARISON ===');
        console.log('CLI would show:', basicParsed.chord);
        console.log('Web detailed shows:', detailedParsed.chord);
        console.log('Are they different?', basicParsed.chord !== detailedParsed.chord ? 'YES ✅' : 'NO');
        
        if (basicParsed.chord !== detailedParsed.chord) {
            console.log('\n✅ CONFIRMED: Web version uses detailed analysis which prioritizes slash chords!');
            console.log('📋 EXPLANATION:');
            console.log('   - CLI uses detectChordWithAlternatives() → exact hash matches first');
            console.log('   - Web uses detectChordWithDetailedAnalysis() → slash chord analysis first');
            console.log('   - This explains why web shows "Gomit3/C" while CLI shows "Csus2"');
        }
        
        // Cleanup
        module.ccall('chordlock_cleanup', null, [], []);
        
    } catch (error) {
        console.error('Error:', error);
    }
}

compareBasicVsDetailed();