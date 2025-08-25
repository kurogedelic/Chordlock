// ChordLock WebAssembly Application
let chordlock = null;
let selectedNotes = [];
let isLoading = true;

// Initialize ChordLock module
ChordLockModule().then(module => {
    chordlock = new module.ChordLock();
    isLoading = false;
    hideLoading();
    initializePiano();
    showVersion();
}).catch(error => {
    console.error('Failed to load ChordLock:', error);
    alert('Failed to load ChordLock engine. Please refresh the page.');
});

// Hide loading overlay
function hideLoading() {
    const overlay = document.getElementById('loading-overlay');
    overlay.style.opacity = '0';
    setTimeout(() => {
        overlay.style.display = 'none';
    }, 300);
}

// Show version info
function showVersion() {
    if (!chordlock) return;
    
    const version = chordlock.getVersion();
    console.log('ChordLock loaded:', version);
}

// Initialize piano keyboard
function initializePiano() {
    const piano = document.getElementById('piano');
    const noteNames = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B'];
    const whiteKeys = [0, 2, 4, 5, 7, 9, 11];
    const blackKeys = [1, 3, 6, 8, 10];
    
    // Create 2 octaves starting from C4 (MIDI 60)
    for (let octave = 0; octave < 2; octave++) {
        const baseNote = 60 + (octave * 12);
        
        // Create white keys first
        whiteKeys.forEach((noteIndex, i) => {
            const midiNote = baseNote + noteIndex;
            const key = createPianoKey(midiNote, noteNames[noteIndex], true, i + (octave * 7));
            piano.appendChild(key);
        });
    }
    
    // Add black keys with absolute positioning
    for (let octave = 0; octave < 2; octave++) {
        const baseNote = 60 + (octave * 12);
        
        blackKeys.forEach(noteIndex => {
            const midiNote = baseNote + noteIndex;
            const key = createPianoKey(midiNote, noteNames[noteIndex], false);
            
            // Calculate position
            let position = 0;
            if (noteIndex === 1) position = 30;  // C#
            else if (noteIndex === 3) position = 70;  // Eb
            else if (noteIndex === 6) position = 150; // F#
            else if (noteIndex === 8) position = 190; // Ab
            else if (noteIndex === 10) position = 230; // Bb
            
            position += octave * 280;
            key.style.left = position + 'px';
            
            piano.appendChild(key);
        });
    }
}

// Create a piano key element
function createPianoKey(midiNote, noteName, isWhite, whiteKeyIndex = 0) {
    const key = document.createElement('div');
    key.className = `piano-key ${isWhite ? 'white' : 'black'}`;
    key.dataset.midi = midiNote;
    
    if (isWhite) {
        key.style.position = 'relative';
    }
    
    const label = document.createElement('div');
    label.className = 'note-label';
    label.textContent = noteName;
    key.appendChild(label);
    
    // Add click handler
    key.addEventListener('click', () => toggleNote(midiNote));
    
    return key;
}

// Toggle note selection
function toggleNote(midiNote) {
    const index = selectedNotes.indexOf(midiNote);
    const key = document.querySelector(`.piano-key[data-midi="${midiNote}"]`);
    
    if (index > -1) {
        selectedNotes.splice(index, 1);
        key.classList.remove('active');
    } else {
        selectedNotes.push(midiNote);
        key.classList.add('active');
    }
    
    selectedNotes.sort((a, b) => a - b);
    updateSelectedNotesDisplay();
    analyzeChord();
}

// Update selected notes display
function updateSelectedNotesDisplay() {
    const display = document.getElementById('selected-notes-display');
    display.innerHTML = '';
    
    selectedNotes.forEach(note => {
        const noteName = chordlock ? chordlock.midiToNoteName(note) : `MIDI ${note}`;
        const badge = document.createElement('span');
        badge.className = 'note-badge';
        badge.textContent = noteName;
        display.appendChild(badge);
    });
}

// Analyze current chord
function analyzeChord() {
    if (!chordlock || selectedNotes.length === 0) {
        clearAnalysis();
        return;
    }
    
    const mode = document.getElementById('recognition-mode').value;
    let result;
    
    if (mode === 'jazz') {
        result = chordlock.identifyJazzChord(selectedNotes);
        displayJazzAnalysis(result);
    } else if (mode === 'advanced') {
        // Try polychord detection first
        const polychord = chordlock.detectPolychord(selectedNotes);
        if (polychord) {
            displayPolychord(polychord);
            return;
        }
        
        // Try quartal harmony
        const quartal = chordlock.detectQuartalHarmony(selectedNotes);
        if (quartal.isQuartal) {
            displayQuartal(quartal);
            return;
        }
        
        // Fall back to standard analysis
        result = chordlock.identifyChord(selectedNotes);
    } else {
        result = chordlock.identifyChord(selectedNotes);
    }
    
    displayAnalysis(result);
    
    // Get suggestions
    const suggestions = chordlock.getChordSuggestions(selectedNotes);
    displaySuggestions(suggestions);
}

// Display standard analysis
function displayAnalysis(result) {
    document.getElementById('chord-name').textContent = result.fullName || result.chordName || '-';
    document.getElementById('chord-symbol').textContent = result.symbol || '';
    document.getElementById('root-note').textContent = result.rootNote || '-';
    document.getElementById('quality').textContent = result.quality || '-';
    document.getElementById('intervals').textContent = result.intervals ? 
        '[' + result.intervals.join(', ') + ']' : '-';
    document.getElementById('confidence').textContent = result.confidence ? 
        Math.round(result.confidence * 100) + '%' : '-';
    document.getElementById('processing-time').textContent = result.processingTime ? 
        result.processingTime + 'μs' : '-';
    document.getElementById('category').textContent = result.category || '-';
    
    // Show badges
    document.getElementById('inversion-badge').style.display = 
        result.isInversion ? 'inline-block' : 'none';
    document.getElementById('slash-badge').style.display = 
        result.isSlashChord ? 'inline-block' : 'none';
    
    // Show alternatives
    if (result.alternatives && result.alternatives.length > 0) {
        const altList = document.getElementById('alternatives-list');
        altList.innerHTML = '';
        result.alternatives.forEach(alt => {
            const div = document.createElement('div');
            div.className = 'alternative-item';
            div.textContent = alt;
            altList.appendChild(div);
        });
        document.getElementById('alternatives-section').style.display = 'block';
    } else {
        document.getElementById('alternatives-section').style.display = 'none';
    }
}

// Display jazz analysis
function displayJazzAnalysis(result) {
    // First display basic info
    document.getElementById('chord-name').textContent = result.primaryChord || '-';
    document.getElementById('confidence').textContent = 
        Math.round(result.confidence * 100) + '%';
    
    // Show jazz section
    document.getElementById('jazz-analysis').style.display = 'block';
    document.getElementById('extensions').textContent = 
        result.extensions && result.extensions.length > 0 ? 
        result.extensions.join(', ') : 'None';
    document.getElementById('alterations').textContent = 
        result.alterations && result.alterations.length > 0 ? 
        result.alterations.join(', ') : 'None';
    document.getElementById('tonal-ambiguity').textContent = 
        result.tonalAmbiguity ? (result.tonalAmbiguity * 100).toFixed(1) + '%' : '0%';
    
    // Show badges
    document.getElementById('rootless-badge').style.display = 
        result.isRootless ? 'inline-block' : 'none';
    document.getElementById('quartal-badge').style.display = 
        result.isQuartal ? 'inline-block' : 'none';
}

// Display polychord
function displayPolychord(polychord) {
    document.getElementById('chord-name').textContent = 
        `${polychord.lower.chord} / ${polychord.upper.chord}`;
    document.getElementById('chord-symbol').textContent = 'Polychord';
    document.getElementById('polychord-badge').style.display = 'inline-block';
}

// Display quartal harmony
function displayQuartal(quartal) {
    document.getElementById('chord-name').textContent = quartal.chordName;
    document.getElementById('confidence').textContent = 
        Math.round(quartal.confidence * 100) + '%';
    document.getElementById('quartal-badge').style.display = 'inline-block';
}

// Clear analysis display
function clearAnalysis() {
    document.getElementById('chord-name').textContent = '-';
    document.getElementById('chord-symbol').textContent = '';
    document.getElementById('root-note').textContent = '-';
    document.getElementById('quality').textContent = '-';
    document.getElementById('intervals').textContent = '-';
    document.getElementById('confidence').textContent = '-';
    document.getElementById('processing-time').textContent = '-';
    document.getElementById('category').textContent = '-';
    
    // Hide all badges
    document.querySelectorAll('.feature-badge').forEach(badge => {
        badge.style.display = 'none';
    });
    
    document.getElementById('jazz-analysis').style.display = 'none';
    document.getElementById('alternatives-section').style.display = 'none';
}

// Display suggestions
function displaySuggestions(suggestions) {
    const list = document.getElementById('suggestions-list');
    list.innerHTML = '';
    
    if (suggestions && suggestions.length > 0) {
        for (let i = 0; i < suggestions.length; i++) {
            const div = document.createElement('div');
            div.className = 'suggestion-item';
            div.textContent = suggestions[i];
            list.appendChild(div);
        }
    }
}

// Analyze MIDI input
function analyzeMidiInput() {
    const input = document.getElementById('midi-input').value;
    const notes = input.split(',').map(n => parseInt(n.trim())).filter(n => !isNaN(n));
    
    if (notes.length === 0) {
        alert('Please enter valid MIDI note numbers (e.g., 60,64,67)');
        return;
    }
    
    loadExample(notes);
}

// Transpose chord
function transposeChord() {
    if (!chordlock || selectedNotes.length === 0) return;
    
    const semitones = parseInt(document.getElementById('transpose-slider').value);
    const transposed = chordlock.transposeChord(selectedNotes, semitones);
    
    // Clear current selection
    document.querySelectorAll('.piano-key').forEach(key => {
        key.classList.remove('active');
    });
    
    // Load transposed notes
    selectedNotes = [];
    for (let i = 0; i < transposed.length; i++) {
        selectedNotes.push(transposed[i]);
    }
    
    // Update display
    selectedNotes.forEach(note => {
        const key = document.querySelector(`.piano-key[data-midi="${note}"]`);
        if (key) key.classList.add('active');
    });
    
    updateSelectedNotesDisplay();
    analyzeChord();
}

// Update transpose value display
document.getElementById('transpose-slider').addEventListener('input', (e) => {
    document.getElementById('transpose-value').textContent = e.target.value;
});

// Build chord from selection
function buildChord() {
    const root = parseInt(document.getElementById('root-select').value);
    const type = document.getElementById('chord-type').value;
    
    let notes = [root];
    
    switch(type) {
        case 'major': notes = [root, root+4, root+7]; break;
        case 'minor': notes = [root, root+3, root+7]; break;
        case '7': notes = [root, root+4, root+7, root+10]; break;
        case 'maj7': notes = [root, root+4, root+7, root+11]; break;
        case 'm7': notes = [root, root+3, root+7, root+10]; break;
        case '9': notes = [root, root+4, root+7, root+10, root+14]; break;
        case '11': notes = [root, root+4, root+7, root+10, root+14, root+17]; break;
        case '13': notes = [root, root+4, root+7, root+10, root+14, root+17, root+21]; break;
        case 'dim': notes = [root, root+3, root+6]; break;
        case 'aug': notes = [root, root+4, root+8]; break;
        case 'sus4': notes = [root, root+5, root+7]; break;
        case 'sus2': notes = [root, root+2, root+7]; break;
    }
    
    loadExample(notes);
}

// Load example chord
function loadExample(notes) {
    // Clear current selection
    document.querySelectorAll('.piano-key').forEach(key => {
        key.classList.remove('active');
    });
    
    selectedNotes = notes.slice();
    
    // Highlight keys
    selectedNotes.forEach(note => {
        const key = document.querySelector(`.piano-key[data-midi="${note}"]`);
        if (key) key.classList.add('active');
    });
    
    updateSelectedNotesDisplay();
    analyzeChord();
}

// Run performance benchmark
function runBenchmark() {
    if (!chordlock) {
        alert('ChordLock is still loading. Please wait.');
        return;
    }
    
    const results = chordlock.runBenchmark();
    const resultsDiv = document.getElementById('benchmark-results');
    resultsDiv.style.display = 'block';
    resultsDiv.innerHTML = '<h3>Benchmark Results</h3>';
    
    const chordNames = ['C Major', 'C Minor', 'C7', 'Cmaj7', 'C9'];
    
    for (let i = 0; i < results.length; i++) {
        const item = document.createElement('div');
        item.className = 'benchmark-item';
        item.innerHTML = `
            <span>${chordNames[results[i].chordIndex]}</span>
            <span><strong>${results[i].avgTime.toFixed(3)}μs</strong></span>
        `;
        resultsDiv.appendChild(item);
    }
    
    // Calculate average
    const totalTime = results.reduce((sum, r) => sum + r.avgTime, 0);
    const avgTime = totalTime / results.length;
    
    const summary = document.createElement('div');
    summary.className = 'benchmark-item';
    summary.style.fontWeight = 'bold';
    summary.style.borderTop = '2px solid var(--border-color)';
    summary.style.marginTop = '10px';
    summary.innerHTML = `
        <span>Average</span>
        <span style="color: var(--success-color)">${avgTime.toFixed(3)}μs</span>
    `;
    resultsDiv.appendChild(summary);
    
    // Update performance badge
    document.getElementById('performance').textContent = `${avgTime.toFixed(3)}μs/chord`;
}

// Keyboard shortcuts
document.addEventListener('keydown', (e) => {
    if (e.key === 'c' && e.metaKey) {
        e.preventDefault();
        selectedNotes = [];
        document.querySelectorAll('.piano-key').forEach(key => {
            key.classList.remove('active');
        });
        updateSelectedNotesDisplay();
        clearAnalysis();
    }
});