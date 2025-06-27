/**
 * Chordlock MCP Types
 * TypeScript type definitions for the Chordlock Model Context Protocol server
 */

export interface ChordDetectionResult {
  chord: string;
  confidence: number;
  detectionTime?: number;
  alternatives?: string[];
}

export interface ChordNotesResult {
  found: boolean;
  chord: string;
  notes: number[];
  noteNames: string[];
  error?: string;
  suggestions?: string[];
}

export interface ChordProgression {
  chords: string[];
  key?: string;
  mode?: 'major' | 'minor';
  tempo?: number;
}

export interface VoicingOptions {
  type?: 'standard' | 'jazz' | 'close' | 'open';
  rootOctave?: number;
  inversion?: number;
}

export interface HarmonyAnalysis {
  key: string;
  scale: string[];
  romanNumerals: string[];
  functions: string[];
  suggestions: string[];
}

export interface MCPToolResult {
  success: boolean;
  data?: any;
  error?: string;
  metadata?: {
    processingTime?: number;
    version?: string;
  };
}

// WebAssembly module interface
export interface ChordlockWASM {
  ccall: (name: string, returnType: string | null, argTypes: string[], args: any[]) => any;
  Module: any;
}

// MCP Tool definitions
export type MCPTool = 
  | 'detect_chord'
  | 'chord_to_notes' 
  | 'chord_progression'
  | 'find_similar_chords'
  | 'analyze_harmony'
  | 'get_voicings';