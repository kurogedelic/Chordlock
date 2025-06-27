/**
 * Chordlock WebAssembly Integration for Node.js
 * Provides a Node.js-compatible interface to the Chordlock engine
 */

import * as fs from 'fs';
import * as path from 'path';
import { ChordlockWASM, ChordDetectionResult, ChordNotesResult } from './types';

class ChordlockEngine {
  private wasmModule: ChordlockWASM | null = null;
  private initialized = false;

  /**
   * Initialize the Chordlock WebAssembly module
   */
  async initialize(): Promise<void> {
    if (this.initialized) return;

    try {
      // Load the WebAssembly module for Node.js
      const ChordlockModule = require('../chordlock.js');
      
      // Create WebAssembly instance
      this.wasmModule = await ChordlockModule({
        locateFile: (path: string) => {
          if (path.endsWith('.wasm')) {
            return require.resolve('../chordlock.wasm');
          }
          return path;
        }
      });

      // Initialize the Chordlock engine
      this.wasmModule!.ccall('chordlock_init', null, [], []);
      this.initialized = true;
      
      console.log('✅ Chordlock engine initialized successfully');
    } catch (error) {
      console.error('❌ Failed to initialize Chordlock engine:', error);
      throw new Error(`Chordlock initialization failed: ${error}`);
    }
  }

  /**
   * Detect chord from MIDI notes
   */
  detectChord(midiNotes: number[]): ChordDetectionResult {
    this.ensureInitialized();

    try {
      // Clear previous notes
      this.wasmModule!.ccall('chordlock_clear_notes', null, [], []);

      // Add MIDI notes
      for (const note of midiNotes) {
        this.wasmModule!.ccall('chordlock_note_on', null, ['number', 'number'], [note, 80]);
      }

      // Detect chord with detailed analysis
      const resultJson = this.wasmModule!.ccall('chordlock_detect_chord_detailed', 'string', ['number'], [5]);
      const result = JSON.parse(resultJson);

      return {
        chord: result.chord || 'Unknown',
        confidence: result.confidence || 0,
        detectionTime: result.detectionTime,
        alternatives: result.alternatives || []
      };
    } catch (error) {
      console.error('Chord detection error:', error);
      return {
        chord: 'Error',
        confidence: 0,
        alternatives: []
      };
    }
  }

  /**
   * Convert chord name to MIDI notes
   */
  chordToNotes(chordName: string, rootOctave: number = 4): ChordNotesResult {
    this.ensureInitialized();

    try {
      const resultJson = this.wasmModule!.ccall(
        'chordlock_chord_name_to_notes_json', 
        'string', 
        ['string', 'number'], 
        [chordName, rootOctave]
      );
      
      const result = JSON.parse(resultJson);

      if (!result.found || result.error) {
        // Try to find similar chords
        const similarJson = this.wasmModule!.ccall(
          'chordlock_find_similar_chord_names', 
          'string', 
          ['string'], 
          [chordName]
        );
        const similarData = JSON.parse(similarJson);

        return {
          found: false,
          chord: chordName,
          notes: [],
          noteNames: [],
          error: result.error || `Chord "${chordName}" not found`,
          suggestions: similarData.similar || []
        };
      }

      // Convert MIDI numbers to note names
      const noteNames = result.notes.map((note: number) => {
        const noteClass = note % 12;
        const octave = Math.floor(note / 12) - 1;
        const names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        return `${names[noteClass]}${octave}`;
      });

      return {
        found: true,
        chord: result.chord,
        notes: result.notes,
        noteNames,
        suggestions: []
      };
    } catch (error) {
      console.error('Chord to notes conversion error:', error);
      return {
        found: false,
        chord: chordName,
        notes: [],
        noteNames: [],
        error: `Conversion failed: ${error}`,
        suggestions: []
      };
    }
  }

  /**
   * Find similar chord names
   */
  findSimilarChords(chordName: string): string[] {
    this.ensureInitialized();

    try {
      const resultJson = this.wasmModule!.ccall(
        'chordlock_find_similar_chord_names', 
        'string', 
        ['string'], 
        [chordName]
      );
      
      const result = JSON.parse(resultJson);
      return result.similar || [];
    } catch (error) {
      console.error('Similar chords search error:', error);
      return [];
    }
  }

  /**
   * Get engine version
   */
  getVersion(): string {
    this.ensureInitialized();

    try {
      return this.wasmModule!.ccall('chordlock_get_version', 'string', [], []);
    } catch (error) {
      console.error('Version retrieval error:', error);
      return 'unknown';
    }
  }

  /**
   * Cleanup resources
   */
  cleanup(): void {
    if (this.wasmModule && this.initialized) {
      try {
        this.wasmModule.ccall('chordlock_cleanup', null, [], []);
      } catch (error) {
        console.warn('Cleanup error (non-critical):', error);
      }
    }
    this.initialized = false;
    this.wasmModule = null;
  }

  private ensureInitialized(): void {
    if (!this.initialized || !this.wasmModule) {
      throw new Error('Chordlock engine not initialized. Call initialize() first.');
    }
  }
}

export default ChordlockEngine;