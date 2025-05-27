#include "Chordlock.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>

#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

// Global Chordlock instance
Chordlock chordlock;
bool running = true;

// Simple MIDI input for macOS using Core MIDI
#ifdef __APPLE__
void MIDIReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon) {
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
                chordlock.noteOn(data1, ms);
                chordlock.setVelocity(data1, data2);
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
    
    status = MIDIInputPortCreate(client, CFSTR("Chordlock Input"), MIDIReadProc, NULL, &inputPort);
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
    auto chord = chordlock.detect(ms);
    
    std::cout << "Current Chord: ";
    if (chord.name == "—" || chord.name == "???") {
        std::cout << "(no chord detected)" << std::endl;
    } else {
        std::cout << "\033[1;32m" << chord.name << "\033[0m";
        std::cout << " (confidence: " << std::fixed << std::setprecision(2) << chord.confidence << ")" << std::endl;
    }
    
    std::cout << std::endl;
    
    // Show alternatives
    auto alternatives = chordlock.detectAlternatives(ms, 5);
    if (alternatives.size() > 1) {
        std::cout << "Alternative interpretations:" << std::endl;
        for (size_t i = 1; i < alternatives.size(); ++i) {
            std::cout << "  " << alternatives[i].name 
                      << " (" << std::fixed << std::setprecision(2) 
                      << alternatives[i].confidence << ")" << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
}

void signalHandler(int signum) {
    running = false;
}

int main(int argc, char* argv[]) {
    std::cout << "Chordlock Command Line Interface" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Parse arguments
    bool slashChords = true;
    bool velocitySensitive = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-slash" || arg == "-n") {
            slashChords = false;
        } else if (arg == "--velocity" || arg == "-v") {
            velocitySensitive = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -n, --no-slash    Disable slash chord detection" << std::endl;
            std::cout << "  -v, --velocity    Enable velocity sensitivity" << std::endl;
            std::cout << "  -h, --help        Show this help message" << std::endl;
            return 0;
        }
    }
    
    // Configure Chordlock
    chordlock.setOnChordDetection(slashChords);
    chordlock.setVelocitySensitivity(velocitySensitive);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Slash chords: " << (slashChords ? "enabled" : "disabled") << std::endl;
    std::cout << "  Velocity sensitivity: " << (velocitySensitive ? "enabled" : "disabled") << std::endl;
    std::cout << std::endl;
    
    // Setup MIDI
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
    
    return 0;
}