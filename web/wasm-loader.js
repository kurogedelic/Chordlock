// WebAssembly loader with better error handling for GitHub Pages
async function loadChordlockModule() {
    try {
        // Check if WebAssembly is supported
        if (typeof WebAssembly === 'undefined') {
            throw new Error('WebAssembly is not supported in this browser');
        }

        // Load the module
        const module = await createChordlockModule({
            locateFile: (path) => {
                // Ensure correct path for GitHub Pages
                if (path.endsWith('.wasm')) {
                    return './chordlock.wasm';
                }
                return path;
            },
            onRuntimeInitialized: () => {
                console.log('Chordlock WASM module loaded successfully');
            }
        });

        return module;
    } catch (error) {
        console.error('Failed to load Chordlock module:', error);
        document.getElementById('chord').textContent = 'Error: ' + error.message;
        throw error;
    }
}

// Export for use in main.js
window.loadChordlockModule = loadChordlockModule;