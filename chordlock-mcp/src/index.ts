#!/usr/bin/env node

/**
 * Chordlock MCP Server Entry Point
 * Command-line interface for the Chordlock Model Context Protocol server
 */

import { ChordlockMCPServer } from './server';

async function main() {
  const server = new ChordlockMCPServer();
  
  try {
    await server.start();
  } catch (error) {
    process.stderr.write(`Failed to start Chordlock MCP Server: ${error}\n`);
    process.exit(1);
  }
}

// Start the server
if (require.main === module) {
  main().catch(error => {
    process.stderr.write(`Unhandled error: ${error}\n`);
    process.exit(1);
  });
}

export { ChordlockMCPServer };
export * from './types';
export * from './tools';