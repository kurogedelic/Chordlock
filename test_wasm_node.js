const fs = require('fs');

// Load the WebAssembly module
const wasmBuffer = fs.readFileSync('./web/chordlock.wasm');

async function testWebAssembly() {
    try {
        console.log('Loading WebAssembly module...');
        
        // Import the module
        const ChordlockModule = require('./web/chordlock.js');
        
        const module = await ChordlockModule();
        
        console.log('Initializing Chordlock...');
        module.ccall('chordlock_init', null, [], []);
        
        console.log('Testing CM7 vs Cmaj7 canonical naming...\n');
        
        // Test CM7
        const cm7Result = module.ccall('chordlock_chord_name_to_notes_json', 'string', ['string', 'number'], ['CM7', 4]);
        const cm7Data = JSON.parse(cm7Result);
        console.log('CM7 result:', cm7Result);
        
        // Test Cmaj7  
        const cmaj7Result = module.ccall('chordlock_chord_name_to_notes_json', 'string', ['string', 'number'], ['Cmaj7', 4]);
        const cmaj7Data = JSON.parse(cmaj7Result);
        console.log('Cmaj7 result:', cmaj7Result);
        
        // Compare
        console.log('\n=== COMPARISON ===');
        console.log('CM7 canonical name:', cm7Data.chord);
        console.log('Cmaj7 canonical name:', cmaj7Data.chord);
        console.log('Are they the same?', cm7Data.chord === cmaj7Data.chord ? 'YES ✅' : 'NO ❌');
        
        if (cm7Data.chord === cmaj7Data.chord) {
            console.log('\n✅ SUCCESS: WebAssembly canonical naming is working correctly!');
            process.exit(0);
        } else {
            console.log('\n❌ PROBLEM: WebAssembly canonical naming is not working correctly!');
            console.log('Expected both to return "Cmaj7"');
            process.exit(1);
        }
        
    } catch (error) {
        console.error('Error testing WebAssembly:', error);
        process.exit(1);
    }
}

testWebAssembly();