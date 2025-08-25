#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "../Core/ChordIdentifier.h"
#include "../Core/ProgressionAnalyzer.h"

namespace ChordLock {

enum class OutputFormat {
    STANDARD,
    JSON,
    XML,
    MINIMAL,
    JAZZ,
    MIDI,
    MUSICXML,
    LILYPOND
};

struct FormattedOutput {
    std::string content;
    std::string mime_type;
    std::string file_extension;
    bool is_binary;
    
    FormattedOutput() : is_binary(false) {}
};

class OutputFormatter {
private:
    OutputFormat current_format;
    std::string indent_string;
    
    // JSON formatting helpers
    std::string escapeJsonString(const std::string& str) const;
    std::string formatJsonChord(const ChordIdentificationResult& result) const;
    std::string formatJsonProgression(const ProgressionAnalysis& analysis) const;
    
    // XML formatting helpers
    std::string escapeXmlString(const std::string& str) const;
    std::string formatXmlChord(const ChordIdentificationResult& result) const;
    std::string formatXmlProgression(const ProgressionAnalysis& analysis) const;
    
    // MusicXML specific formatting
    std::string generateMusicXMLHeader() const;
    std::string generateMusicXMLChord(const ChordIdentificationResult& result, int measure = 1) const;
    std::string generateMusicXMLFooter() const;
    
    // LilyPond formatting
    std::string formatLilyPondChord(const ChordIdentificationResult& result) const;
    std::string generateLilyPondHeader() const;
    
    // MIDI export helpers
    std::vector<uint8_t> generateMIDIBytes(const std::vector<ChordIdentificationResult>& chords) const;
    void writeMIDIVariableLength(std::vector<uint8_t>& data, uint32_t value) const;
    
public:
    OutputFormatter(OutputFormat format = OutputFormat::STANDARD);
    ~OutputFormatter() = default;
    
    // Single chord formatting
    FormattedOutput formatChord(const ChordIdentificationResult& result) const;
    FormattedOutput formatChordWithVoicing(const ChordIdentificationResult& result,
                                          const std::vector<int>& midi_notes) const;
    
    // Progression formatting
    FormattedOutput formatProgression(const std::vector<ChordIdentificationResult>& chords) const;
    FormattedOutput formatProgressionAnalysis(const ProgressionAnalysis& analysis) const;
    FormattedOutput formatProgressionAnalysis(const ProgressionAnalysis& analysis,
                                             const std::vector<ChordIdentificationResult>& chords) const;
    
    // Advanced formatting with multiple analysis types
    FormattedOutput formatCompleteAnalysis(const ChordIdentificationResult& chord,
                                          const std::vector<int>& midi_notes,
                                          const std::string& roman_numeral = "",
                                          const std::vector<std::string>& suggestions = {}) const;
    
    // Export formats
    FormattedOutput exportToMIDI(const std::vector<ChordIdentificationResult>& chords,
                                 int tempo = 120, int time_signature_num = 4) const;
    FormattedOutput exportToMusicXML(const std::vector<ChordIdentificationResult>& chords,
                                    const std::string& title = "ChordLock Analysis") const;
    FormattedOutput exportToLilyPond(const std::vector<ChordIdentificationResult>& chords,
                                    const std::string& title = "ChordLock Analysis") const;
    
    // File operations
    bool saveToFile(const FormattedOutput& output, const std::string& filepath) const;
    FormattedOutput loadFromFile(const std::string& filepath) const;
    
    // Configuration
    void setFormat(OutputFormat format) { current_format = format; }
    OutputFormat getFormat() const { return current_format; }
    void setIndentation(const std::string& indent) { indent_string = indent; }
    
    // Format detection
    static OutputFormat detectFormat(const std::string& format_string);
    static std::string getFormatName(OutputFormat format);
    static std::vector<std::string> getSupportedFormats();
    
    // Validation
    bool isValidFormat(const std::string& format_string) const;
    bool canExportBinary() const;
    
private:
    // Internal formatting utilities
    std::string formatTimestamp() const;
    std::string generateMetadata() const;
    std::string formatFloat(float value, int precision = 2) const;
    
    // Note conversion utilities
    std::string midiNoteToLilyPond(int midi_note) const;
    std::string midiNoteToMusicXML(int midi_note) const;
    int chordToMIDIPitch(const std::string& chord_name) const;
    
    // Template specializations for different formats
    template<typename T>
    std::string formatContainer(const std::vector<T>& container, const std::string& separator = ", ") const;
};

// Inline utility implementations
inline OutputFormat OutputFormatter::detectFormat(const std::string& format_string) {
    std::string lower_format = format_string;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);
    
    if (lower_format == "json") return OutputFormat::JSON;
    if (lower_format == "xml") return OutputFormat::XML;
    if (lower_format == "minimal") return OutputFormat::MINIMAL;
    if (lower_format == "jazz") return OutputFormat::JAZZ;
    if (lower_format == "midi") return OutputFormat::MIDI;
    if (lower_format == "musicxml") return OutputFormat::MUSICXML;
    if (lower_format == "lilypond" || lower_format == "ly") return OutputFormat::LILYPOND;
    
    return OutputFormat::STANDARD;
}

inline std::string OutputFormatter::getFormatName(OutputFormat format) {
    switch (format) {
        case OutputFormat::JSON: return "JSON";
        case OutputFormat::XML: return "XML";
        case OutputFormat::MINIMAL: return "Minimal";
        case OutputFormat::JAZZ: return "Jazz";
        case OutputFormat::MIDI: return "MIDI";
        case OutputFormat::MUSICXML: return "MusicXML";
        case OutputFormat::LILYPOND: return "LilyPond";
        default: return "Standard";
    }
}

inline std::vector<std::string> OutputFormatter::getSupportedFormats() {
    return {"standard", "json", "xml", "minimal", "jazz", "midi", "musicxml", "lilypond"};
}

inline bool OutputFormatter::canExportBinary() const {
    return current_format == OutputFormat::MIDI;
}

inline std::string OutputFormatter::escapeJsonString(const std::string& str) const {
    std::string escaped;
    escaped.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    
    return escaped;
}

inline std::string OutputFormatter::escapeXmlString(const std::string& str) const {
    std::string escaped;
    escaped.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&apos;"; break;
            default: escaped += c; break;
        }
    }
    
    return escaped;
}

template<typename T>
std::string OutputFormatter::formatContainer(const std::vector<T>& container, const std::string& separator) const {
    if (container.empty()) return "";
    
    std::string result;
    for (size_t i = 0; i < container.size(); ++i) {
        if (i > 0) result += separator;
        result += std::to_string(container[i]);
    }
    
    return result;
}

} // namespace ChordLock