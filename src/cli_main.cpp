#include "Chordlock.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

// Global Chordlock instance
Chordlock chordlock;
bool running = true;

// Simple MIDI input for macOS using Core MIDI
#ifdef __APPLE__
void ChordlockMIDIReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon) {
    const MIDIPacket *packet = &pktlist->packet[0];
    for (int i = 0; i < pktlist->numPackets; ++i) {
        for (int j = 0; j < packet->length; j += 3) {
            unsigned char status = packet->data[j];
            unsigned char data1 = packet->data[j + 1];
            unsigned char data2 = packet->data[j + 2];
            
            // Get current time in milliseconds
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            // Note On
            if ((status & 0xF0) == 0x90 && data2 > 0) {
                chordlock.noteOn(data1, data2);
            }
            // Note Off (or Note On with velocity 0)
            else if ((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0)) {
                chordlock.noteOff(data1);
            }
        }
        packet = MIDIPacketNext(packet);
    }
}

bool setupCoreMIDI() {
    MIDIClientRef client;
    MIDIPortRef inputPort;
    
    OSStatus status = MIDIClientCreate(CFSTR("Chordlock CLI"), NULL, NULL, &client);
    if (status != noErr) {
        std::cerr << "Failed to create MIDI client" << std::endl;
        return false;
    }
    
    status = MIDIInputPortCreate(client, CFSTR("Chordlock Input"), ChordlockMIDIReadProc, NULL, &inputPort);
    if (status != noErr) {
        std::cerr << "Failed to create MIDI input port" << std::endl;
        return false;
    }
    
    // Connect to all available MIDI sources
    ItemCount sourceCount = MIDIGetNumberOfSources();
    if (sourceCount == 0) {
        std::cerr << "No MIDI sources found" << std::endl;
        return false;
    }
    
    std::cout << "Found " << sourceCount << " MIDI source(s):" << std::endl;
    
    for (ItemCount i = 0; i < sourceCount; ++i) {
        MIDIEndpointRef source = MIDIGetSource(i);
        
        CFStringRef name;
        MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);
        
        char cname[256];
        CFStringGetCString(name, cname, sizeof(cname), kCFStringEncodingUTF8);
        CFRelease(name);
        
        std::cout << "  [" << i << "] " << cname << std::endl;
        
        // Connect to this source
        MIDIPortConnectSource(inputPort, source, NULL);
    }
    
    return true;
}
#endif

void displayChord() {
    // Clear screen (works on most terminals)
    std::cout << "\033[2J\033[H";
    
    std::cout << "==================================" << std::endl;
    std::cout << "      Chordlock CLI Monitor       " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    // Get current time
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    // Detect chord
    auto chord = chordlock.detectChord();
    
    std::cout << "Current Chord: ";
    if (!chord.hasValidChord || chord.chordName == "No Chord") {
        std::cout << "(no chord detected)" << std::endl;
    } else {
        std::cout << "\033[1;32m" << chord.chordName << "\033[0m";
        std::cout << " (confidence: " << std::fixed << std::setprecision(2) << chord.confidence << ")" << std::endl;
    }
    
    std::cout << std::endl;
    
    // Show alternatives
    auto result = chordlock.detectChordWithAlternatives(5);
    if (result.alternativeChords.size() > 0) {
        std::cout << "Alternative interpretations:" << std::endl;
        for (size_t i = 0; i < result.alternativeChords.size(); ++i) {
            std::cout << "  " << result.alternativeChords[i] 
                      << " (" << std::fixed << std::setprecision(2) 
                      << result.alternativeConfidences[i] << ")" << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
}

void signalHandler(int signum) {
    running = false;
}

// Parse note numbers from string (e.g., "[60,64,67]" or "60,64,67")
std::vector<int> parseNoteNumbers(const std::string& input) {
    std::vector<int> notes;
    std::string cleanInput = input;
    
    // Remove brackets if present
    cleanInput.erase(std::remove(cleanInput.begin(), cleanInput.end(), '['), cleanInput.end());
    cleanInput.erase(std::remove(cleanInput.begin(), cleanInput.end(), ']'), cleanInput.end());
    cleanInput.erase(std::remove(cleanInput.begin(), cleanInput.end(), ' '), cleanInput.end());
    
    // Parse comma-separated values
    std::stringstream ss(cleanInput);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        try {
            int note = std::stoi(item);
            if (note >= 0 && note <= 127) {
                notes.push_back(note);
            }
        } catch (...) {
            // Skip invalid numbers
        }
    }
    
    return notes;
}

// Process note numbers and detect chord
void processNotes(const std::vector<int>& notes) {
    // Clear any existing notes
    for (int i = 0; i < 128; ++i) {
        chordlock.noteOff(i);
    }
    
    // Add all notes
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    for (int note : notes) {
        chordlock.noteOn(note, 100);  // Default velocity
    }
    
    // Detect chord
    auto result = chordlock.detectChordWithAlternatives(5);
    
    std::cout << "Notes: ";
    for (size_t i = 0; i < notes.size(); ++i) {
        std::cout << notes[i];
        if (i < notes.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    std::cout << "Detected Chord: ";
    if (!result.hasValidChord || result.chordName == "No Chord") {
        std::cout << "(no chord detected)" << std::endl;
    } else {
        std::cout << result.chordName;
        std::cout << " (confidence: " << std::fixed << std::setprecision(2) << result.confidence << ")" << std::endl;
    }
    
    // Show alternatives
    if (result.alternativeChords.size() > 0) {
        std::cout << "\nAlternative interpretations:" << std::endl;
        for (size_t i = 0; i < result.alternativeChords.size(); ++i) {
            std::cout << "  " << result.alternativeChords[i]
                      << " (" << std::fixed << std::setprecision(2) 
                      << result.alternativeConfidences[i] << ")" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Chordlock Command Line Interface" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Parse arguments
    bool slashChords = true;
    bool velocitySensitive = false;
    std::string noteInput = "";
    std::string filename = "";
    std::string chordName = "";
    bool midiMode = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-slash" || arg == "-n") {
            slashChords = false;
        } else if (arg == "--velocity" || arg == "-v") {
            velocitySensitive = true;
        } else if ((arg == "--notes" || arg == "-N") && i + 1 < argc) {
            noteInput = argv[++i];
            midiMode = false;
        } else if ((arg == "--file" || arg == "-f") && i + 1 < argc) {
            filename = argv[++i];
            midiMode = false;
        } else if ((arg == "--chord" || arg == "-c") && i + 1 < argc) {
            chordName = argv[++i];
            midiMode = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -n, --no-slash          Disable slash chord detection" << std::endl;
            std::cout << "  -v, --velocity          Enable velocity sensitivity" << std::endl;
            std::cout << "  -N, --notes <notes>     Analyze specific notes (e.g., \"60,64,67\" or \"[60,64,67]\")" << std::endl;
            std::cout << "  -f, --file <filename>   Read notes from file (one set per line)" << std::endl;
            std::cout << "  -c, --chord <name>      Convert chord name to MIDI notes (e.g., \"Cmaj7\", \"F#m\")" << std::endl;
            std::cout << "  -h, --help              Show this help message" << std::endl;
            std::cout << "\nExamples:" << std::endl;
            std::cout << "  " << argv[0] << " -N 60,64,67           # Analyze C major chord" << std::endl;
            std::cout << "  " << argv[0] << " -N \"[60, 63, 67]\"    # Analyze C minor chord" << std::endl;
            std::cout << "  " << argv[0] << " -c \"Gmaj7\"           # Convert Gmaj7 to MIDI notes" << std::endl;
            std::cout << "  " << argv[0] << " -c \"CM7\"             # Convert CM7 to MIDI notes (with fallback)" << std::endl;
            std::cout << "  " << argv[0] << " -f chords.txt         # Read from file" << std::endl;
            return 0;
        }
    }
    
    // Configure Chordlock
    chordlock.setSlashChordDetection(slashChords);
    chordlock.setVelocitySensitivity(velocitySensitive);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Slash chords: " << (slashChords ? "enabled" : "disabled") << std::endl;
    std::cout << "  Velocity sensitivity: " << (velocitySensitive ? "enabled" : "disabled") << std::endl;
    std::cout << std::endl;
    
    // Process based on mode
    if (!chordName.empty()) {
        // Reverse chord lookup mode
        std::cout << "🎵 Chord Name to MIDI Notes Conversion" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Input chord: " << chordName << std::endl << std::endl;
        
        // Try primary conversion
        auto notes = chordlock.chordNameToNotes(chordName, 4);
        if (!notes.empty()) {
            std::cout << "✅ Primary result:" << std::endl;
            std::cout << "  MIDI Notes: [";
            for (size_t i = 0; i < notes.size(); i++) {
                std::cout << notes[i];
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
            
            // Show note names
            std::cout << "  Note Names: [";
            const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            for (size_t i = 0; i < notes.size(); i++) {
                int noteClass = notes[i] % 12;
                int octave = notes[i] / 12 - 1;
                std::cout << noteNames[noteClass] << octave;
                if (i < notes.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl << std::endl;
        }
        
        // Try alternatives
        auto alternatives = chordlock.chordNameToNotesWithAlternatives(chordName, 4);
        if (alternatives.size() > 1) {
            std::cout << "🔄 Alternative interpretations:" << std::endl;
            for (size_t alt = 1; alt < alternatives.size(); alt++) {
                std::cout << "  Alternative " << alt << ": [";
                for (size_t i = 0; i < alternatives[alt].size(); i++) {
                    std::cout << alternatives[alt][i];
                    if (i < alternatives[alt].size() - 1) std::cout << ", ";
                }
                std::cout << "]" << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Show similar chords
        auto similar = chordlock.findSimilarChordNames(chordName);
        if (!similar.empty()) {
            std::cout << "🎼 Similar chords available:" << std::endl;
            for (size_t i = 0; i < std::min(similar.size(), size_t(10)); i++) {
                std::cout << "  " << similar[i] << std::endl;
            }
            if (similar.size() > 10) {
                std::cout << "  ... and " << (similar.size() - 10) << " more" << std::endl;
            }
        }
        
        // Show JSON format
        std::cout << std::endl << "📄 JSON format:" << std::endl;
        std::cout << chordlock.chordNameToNotesJSON(chordName, 4) << std::endl;
        
    } else if (!noteInput.empty()) {
        // Direct note input mode
        std::vector<int> notes = parseNoteNumbers(noteInput);
        if (notes.empty()) {
            std::cerr << "Error: No valid notes found in input" << std::endl;
            return 1;
        }
        processNotes(notes);
    } else if (!filename.empty()) {
        // File input mode
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }
        
        std::string line;
        int lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            if (line.empty() || line[0] == '#') continue;  // Skip empty lines and comments
            
            std::cout << "\nLine " << lineNum << ": " << line << std::endl;
            std::vector<int> notes = parseNoteNumbers(line);
            if (!notes.empty()) {
                processNotes(notes);
            }
        }
        file.close();
    } else if (midiMode) {
        // MIDI input mode
#ifdef __APPLE__
        if (!setupCoreMIDI()) {
            std::cerr << "Failed to setup MIDI" << std::endl;
            return 1;
        }
        
        std::cout << "\nListening for MIDI input..." << std::endl;
        std::cout << "Play some chords on your MIDI keyboard!" << std::endl;
        
        // Main loop
        signal(SIGINT, signalHandler);
        
        while (running) {
            displayChord();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "\nShutting down..." << std::endl;
#else
        std::cout << "MIDI input not implemented for this platform yet." << std::endl;
        std::cout << "Currently only macOS is supported." << std::endl;
#endif
    }
    
    return 0;
}