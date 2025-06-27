/**
 * Simple test script for Chordlock MCP package
 */

const { ChordlockMCPTools } = require('./dist/tools');

async function testMCP() {
  console.log('🧪 Testing Chordlock MCP package...\n');
  
  const tools = new ChordlockMCPTools();
  
  try {
    // Initialize
    console.log('⏳ Initializing Chordlock engine...');
    await tools.initialize();
    console.log('✅ Engine initialized\n');
    
    // Test 1: Chord detection
    console.log('🎯 Test 1: Chord Detection');
    const detectionResult = await tools.detectChord({ notes: [60, 64, 67, 71] });
    console.log('Input: C,E,G,B (MIDI: 60,64,67,71)');
    console.log('Result:', JSON.stringify(detectionResult, null, 2));
    console.log();
    
    // Test 2: Chord to notes
    console.log('🎵 Test 2: Chord to Notes');
    const notesResult = await tools.chordToNotes({ chordName: 'Dm7', rootOctave: 4 });
    console.log('Input: Dm7');
    console.log('Result:', JSON.stringify(notesResult, null, 2));
    console.log();
    
    // Test 3: Similar chords
    console.log('🔍 Test 3: Similar Chords');
    const similarResult = await tools.findSimilarChords({ chordName: 'F#m', maxResults: 5 });
    console.log('Input: F#m');
    console.log('Result:', JSON.stringify(similarResult, null, 2));
    console.log();
    
    // Test 4: Chord progression
    console.log('🎼 Test 4: Chord Progression');
    const progressionResult = await tools.chordProgression({ 
      chords: ['C', 'Am', 'F', 'G'], 
      rootOctave: 4,
      beatsPerChord: 4
    });
    console.log('Input: C-Am-F-G progression');
    console.log('Result:', JSON.stringify(progressionResult, null, 2));
    console.log();
    
    console.log('🎉 All tests completed successfully!');
    
  } catch (error) {
    console.error('❌ Test failed:', error);
  } finally {
    tools.cleanup();
  }
}

testMCP();