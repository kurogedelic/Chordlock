/**
 * Chordlock MCP Server Implementation
 * Model Context Protocol server for AI-powered music analysis
 */

import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  Tool,
} from '@modelcontextprotocol/sdk/types.js';
import { ChordlockMCPTools } from './tools';

/**
 * MCP Server for Chordlock
 */
export class ChordlockMCPServer {
  private server: Server;
  private tools: ChordlockMCPTools;

  constructor() {
    this.server = new Server(
      {
        name: 'chordlock-mcp',
        version: '1.0.0',
      },
      {
        capabilities: {
          tools: {},
        },
      }
    );

    this.tools = new ChordlockMCPTools();
    this.setupToolHandlers();
  }

  private setupToolHandlers(): void {
    // List available tools
    this.server.setRequestHandler(ListToolsRequestSchema, async () => {
      return {
        tools: [
          {
            name: 'detect_chord',
            description: 'Detect chord name from MIDI note numbers',
            inputSchema: {
              type: 'object',
              properties: {
                notes: {
                  type: 'array',
                  items: { type: 'number' },
                  description: 'Array of MIDI note numbers (0-127)',
                  minItems: 1
                }
              },
              required: ['notes']
            }
          },
          {
            name: 'chord_to_notes',
            description: 'Convert chord name to MIDI note numbers',
            inputSchema: {
              type: 'object',
              properties: {
                chordName: {
                  type: 'string',
                  description: 'Chord name (e.g., "Cmaj7", "F#m", "Bb13")'
                },
                rootOctave: {
                  type: 'number',
                  description: 'Root note octave (default: 4)',
                  default: 4,
                  minimum: 0,
                  maximum: 8
                },
                voicing: {
                  type: 'object',
                  description: 'Voicing options',
                  properties: {
                    type: {
                      type: 'string',
                      enum: ['standard', 'jazz', 'close', 'open'],
                      default: 'standard'
                    }
                  }
                }
              },
              required: ['chordName']
            }
          },
          {
            name: 'chord_progression',
            description: 'Generate MIDI sequences from chord progressions',
            inputSchema: {
              type: 'object',
              properties: {
                chords: {
                  type: 'array',
                  items: { type: 'string' },
                  description: 'Array of chord names',
                  minItems: 1
                },
                key: {
                  type: 'string',
                  description: 'Key signature (e.g., "C major", "A minor")'
                },
                rootOctave: {
                  type: 'number',
                  description: 'Root octave for all chords (default: 4)',
                  default: 4
                },
                beatsPerChord: {
                  type: 'number',
                  description: 'Duration of each chord in beats (default: 4)',
                  default: 4
                }
              },
              required: ['chords']
            }
          },
          {
            name: 'find_similar_chords',
            description: 'Find similar or alternative chord names',
            inputSchema: {
              type: 'object',
              properties: {
                chordName: {
                  type: 'string',
                  description: 'Chord name to find alternatives for'
                },
                maxResults: {
                  type: 'number',
                  description: 'Maximum number of results (default: 10)',
                  default: 10,
                  minimum: 1,
                  maximum: 50
                }
              },
              required: ['chordName']
            }
          },
          {
            name: 'analyze_harmony',
            description: 'Perform harmonic analysis on chord progressions',
            inputSchema: {
              type: 'object',
              properties: {
                chords: {
                  type: 'array',
                  items: { type: 'string' },
                  description: 'Array of chord names to analyze',
                  minItems: 1
                },
                key: {
                  type: 'string',
                  description: 'Key signature for analysis (e.g., "C major")',
                  default: 'C major'
                }
              },
              required: ['chords']
            }
          }
        ] as Tool[]
      };
    });

    // Handle tool calls
    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      const { name, arguments: args } = request.params;

      try {
        let result;
        
        switch (name) {
          case 'detect_chord':
            result = await this.tools.detectChord(args as { notes: number[] });
            break;
          
          case 'chord_to_notes':
            result = await this.tools.chordToNotes(args as any);
            break;
          
          case 'chord_progression':
            result = await this.tools.chordProgression(args as any);
            break;
          
          case 'find_similar_chords':
            result = await this.tools.findSimilarChords(args as any);
            break;
          
          case 'analyze_harmony':
            result = await this.tools.analyzeHarmony(args as any);
            break;
          
          default:
            throw new Error(`Unknown tool: ${name}`);
        }

        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify(result, null, 2)
            }
          ]
        };

      } catch (error) {
        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify({
                success: false,
                error: `Tool execution failed: ${error}`,
                data: null
              }, null, 2)
            }
          ],
          isError: true
        };
      }
    });
  }

  async start(): Promise<void> {
    // Initialize Chordlock engine
    await this.tools.initialize();

    // Set up transport
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
  }

  async stop(): Promise<void> {
    this.tools.cleanup();
  }
}

// Handle process cleanup
process.on('SIGINT', () => {
  process.exit(0);
});

process.on('SIGTERM', () => {
  process.exit(0);
});