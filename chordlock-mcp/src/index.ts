#!/usr/bin/env node

/**
 * Chordlock MCP Server Entry Point
 * Command-line interface for the Chordlock Model Context Protocol server
 */

import { ChordlockMCPServer } from './server';

async function main() {
  console.log('🎵 Starting Chordlock MCP Server...');
  
  const server = new ChordlockMCPServer();
  
  try {
    await server.start();
  } catch (error) {
    console.error('❌ Failed to start Chordlock MCP Server:', error);
    process.exit(1);
  }
}

// Start the server
if (require.main === module) {
  main().catch(error => {
    console.error('❌ Unhandled error:', error);
    process.exit(1);
  });
}

export { ChordlockMCPServer };
export * from './types';
export * from './tools';