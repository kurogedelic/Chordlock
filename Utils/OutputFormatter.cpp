#include "OutputFormatter.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>

namespace ChordLock {

OutputFormatter::OutputFormatter(OutputFormat format) 
    : current_format(format), indent_string("  ") {
}

FormattedOutput OutputFormatter::formatChord(const ChordIdentificationResult& result) const {
    FormattedOutput output;
    
    switch (current_format) {
        case OutputFormat::JSON:
            output.content = formatJsonChord(result);
            output.mime_type = "application/json";
            output.file_extension = ".json";
            break;
            
        case OutputFormat::XML:
            output.content = formatXmlChord(result);
            output.mime_type = "application/xml";
            output.file_extension = ".xml";
            break;
            
        case OutputFormat::MINIMAL:
            output.content = !result.full_display_name.empty() ? result.full_display_name : result.chord_name;
            output.mime_type = "text/plain";
            output.file_extension = ".txt";
            break;
            
        case OutputFormat::JAZZ:
            output.content = "Chord: " + result.chord_name;
            if (result.is_slash_chord && !result.bass_note_name.empty()) {
                output.content += "/" + result.bass_note_name;
            }
            output.mime_type = "text/plain";
            output.file_extension = ".txt";
            break;
            
        default: // STANDARD
            output.content = "Chord: " + (!result.full_display_name.empty() ? result.full_display_name : result.chord_name);
            output.mime_type = "text/plain";
            output.file_extension = ".txt";
            break;
    }
    
    return output;
}

std::string OutputFormatter::formatJsonChord(const ChordIdentificationResult& result) const {
    std::ostringstream json;
    
    json << "{\n";
    json << indent_string << "\"chord_name\": \"" << escapeJsonString(result.chord_name) << "\",\n";
    json << indent_string << "\"display_name\": \"" << escapeJsonString(result.full_display_name) << "\",\n";
    json << indent_string << "\"root_note\": \"" << escapeJsonString(result.root_note) << "\",\n";
    json << indent_string << "\"chord_symbol\": \"" << escapeJsonString(result.chord_symbol) << "\",\n";
    json << indent_string << "\"bass_note\": \"" << escapeJsonString(result.bass_note_name) << "\",\n";
    json << indent_string << "\"confidence\": " << formatFloat(result.confidence, 3) << ",\n";
    json << indent_string << "\"is_slash_chord\": " << (result.is_slash_chord ? "true" : "false") << ",\n";
    json << indent_string << "\"is_inversion\": " << (result.is_inversion ? "true" : "false") << ",\n";
    json << indent_string << "\"inversion_type\": " << result.inversion_type << ",\n";
    json << indent_string << "\"category\": \"" << escapeJsonString(result.chord_category) << "\",\n";
    json << indent_string << "\"quality\": \"" << escapeJsonString(result.chord_quality) << "\",\n";
    json << indent_string << "\"processing_time_us\": " << result.processing_time.count() << ",\n";
    json << indent_string << "\"intervals\": [";
    
    for (size_t i = 0; i < result.identified_intervals.size(); ++i) {
        if (i > 0) json << ", ";
        json << result.identified_intervals[i];
    }
    json << "],\n";
    
    json << indent_string << "\"notes\": [";
    for (size_t i = 0; i < result.note_names.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << escapeJsonString(result.note_names[i]) << "\"";
    }
    json << "],\n";
    
    json << indent_string << "\"alternatives\": [";
    for (size_t i = 0; i < result.alternative_names.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << escapeJsonString(result.alternative_names[i]) << "\"";
    }
    json << "],\n";
    
    json << indent_string << "\"timestamp\": \"" << formatTimestamp() << "\"\n";
    json << "}";
    
    return json.str();
}

std::string OutputFormatter::formatXmlChord(const ChordIdentificationResult& result) const {
    std::ostringstream xml;
    
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<chord>\n";
    xml << indent_string << "<name>" << escapeXmlString(result.chord_name) << "</name>\n";
    xml << indent_string << "<display_name>" << escapeXmlString(result.full_display_name) << "</display_name>\n";
    xml << indent_string << "<root_note>" << escapeXmlString(result.root_note) << "</root_note>\n";
    xml << indent_string << "<chord_symbol>" << escapeXmlString(result.chord_symbol) << "</chord_symbol>\n";
    xml << indent_string << "<bass_note>" << escapeXmlString(result.bass_note_name) << "</bass_note>\n";
    xml << indent_string << "<confidence>" << formatFloat(result.confidence, 3) << "</confidence>\n";
    xml << indent_string << "<is_slash_chord>" << (result.is_slash_chord ? "true" : "false") << "</is_slash_chord>\n";
    xml << indent_string << "<is_inversion>" << (result.is_inversion ? "true" : "false") << "</is_inversion>\n";
    xml << indent_string << "<inversion_type>" << result.inversion_type << "</inversion_type>\n";
    xml << indent_string << "<category>" << escapeXmlString(result.chord_category) << "</category>\n";
    xml << indent_string << "<quality>" << escapeXmlString(result.chord_quality) << "</quality>\n";
    xml << indent_string << "<processing_time_us>" << result.processing_time.count() << "</processing_time_us>\n";
    
    xml << indent_string << "<intervals>\n";
    for (int interval : result.identified_intervals) {
        xml << indent_string << indent_string << "<interval>" << interval << "</interval>\n";
    }
    xml << indent_string << "</intervals>\n";
    
    xml << indent_string << "<notes>\n";
    for (const std::string& note : result.note_names) {
        xml << indent_string << indent_string << "<note>" << escapeXmlString(note) << "</note>\n";
    }
    xml << indent_string << "</notes>\n";
    
    xml << indent_string << "<alternatives>\n";
    for (const std::string& alt : result.alternative_names) {
        xml << indent_string << indent_string << "<alternative>" << escapeXmlString(alt) << "</alternative>\n";
    }
    xml << indent_string << "</alternatives>\n";
    
    xml << indent_string << "<timestamp>" << formatTimestamp() << "</timestamp>\n";
    xml << "</chord>\n";
    
    return xml.str();
}

FormattedOutput OutputFormatter::formatCompleteAnalysis(const ChordIdentificationResult& chord,
                                                       const std::vector<int>& midi_notes,
                                                       const std::string& roman_numeral,
                                                       const std::vector<std::string>& suggestions) const {
    FormattedOutput output;
    
    switch (current_format) {
        case OutputFormat::JSON: {
            std::ostringstream json;
            json << "{\n";
            json << indent_string << "\"chord_analysis\": " << formatJsonChord(chord) << ",\n";
            json << indent_string << "\"midi_notes\": [" << formatContainer(midi_notes) << "],\n";
            
            if (!roman_numeral.empty()) {
                json << indent_string << "\"roman_numeral\": \"" << escapeJsonString(roman_numeral) << "\",\n";
            }
            
            if (!suggestions.empty()) {
                json << indent_string << "\"chord_suggestions\": [";
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    if (i > 0) json << ", ";
                    json << "\"" << escapeJsonString(suggestions[i]) << "\"";
                }
                json << "],\n";
            }
            
            json << indent_string << "\"metadata\": {\n";
            json << indent_string << indent_string << "\"generated_by\": \"ChordLock\",\n";
            json << indent_string << indent_string << "\"timestamp\": \"" << formatTimestamp() << "\"\n";
            json << indent_string << "}\n";
            json << "}";
            
            output.content = json.str();
            output.mime_type = "application/json";
            output.file_extension = ".json";
            break;
        }
        
        default: {
            std::ostringstream standard;
            standard << "=== ChordLock Complete Analysis ===\n\n";
            standard << "Chord: " << chord.full_display_name << "\n";
            standard << "Root: " << chord.root_note << "\n";
            standard << "Symbol: " << chord.chord_symbol << "\n";
            standard << "Confidence: " << (chord.confidence * 100.0f) << "%\n";
            
            if (!roman_numeral.empty()) {
                standard << "Roman Numeral: " << roman_numeral << "\n";
            }
            
            standard << "MIDI Notes: [" << formatContainer(midi_notes) << "]\n";
            standard << "Intervals: [" << formatContainer(chord.identified_intervals) << "]\n";
            
            if (chord.is_slash_chord) {
                standard << "Bass Note: " << chord.bass_note_name << "\n";
            }
            
            if (chord.is_inversion) {
                standard << "Inversion: " << chord.inversion_type;
                if (chord.inversion_type == 1) standard << "st";
                else if (chord.inversion_type == 2) standard << "nd";
                else if (chord.inversion_type == 3) standard << "rd";
                else standard << "th";
                standard << "\n";
            }
            
            if (!suggestions.empty()) {
                standard << "Suggested Next Chords: ";
                for (size_t i = 0; i < suggestions.size(); ++i) {
                    if (i > 0) standard << ", ";
                    standard << suggestions[i];
                }
                standard << "\n";
            }
            
            standard << "Processing Time: " << chord.processing_time.count() << " μs\n";
            
            output.content = standard.str();
            output.mime_type = "text/plain";
            output.file_extension = ".txt";
            break;
        }
    }
    
    return output;
}

FormattedOutput OutputFormatter::exportToMusicXML(const std::vector<ChordIdentificationResult>& chords,
                                                  const std::string& title) const {
    FormattedOutput output;
    
    std::ostringstream xml;
    xml << generateMusicXMLHeader();
    xml << "  <work>\n";
    xml << "    <work-title>" << escapeXmlString(title) << "</work-title>\n";
    xml << "  </work>\n";
    xml << "  <part-list>\n";
    xml << "    <score-part id=\"P1\">\n";
    xml << "      <part-name>Chord Analysis</part-name>\n";
    xml << "    </score-part>\n";
    xml << "  </part-list>\n";
    xml << "  <part id=\"P1\">\n";
    
    for (size_t i = 0; i < chords.size(); ++i) {
        xml << generateMusicXMLChord(chords[i], static_cast<int>(i + 1));
    }
    
    xml << "  </part>\n";
    xml << generateMusicXMLFooter();
    
    output.content = xml.str();
    output.mime_type = "application/vnd.recordare.musicxml+xml";
    output.file_extension = ".musicxml";
    
    return output;
}

std::string OutputFormatter::generateMusicXMLHeader() const {
    std::ostringstream header;
    header << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    header << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 Partwise//EN\" ";
    header << "\"http://www.musicxml.org/dtds/partwise.dtd\">\n";
    header << "<score-partwise version=\"3.1\">\n";
    return header.str();
}

std::string OutputFormatter::generateMusicXMLChord(const ChordIdentificationResult& result, int measure) const {
    std::ostringstream xml;
    
    xml << "    <measure number=\"" << measure << "\">\n";
    xml << "      <note>\n";
    xml << "        <chord/>\n";
    xml << "        <pitch>\n";
    xml << "          <step>C</step>\n";
    xml << "          <octave>4</octave>\n";
    xml << "        </pitch>\n";
    xml << "        <duration>4</duration>\n";
    xml << "      </note>\n";
    xml << "      <harmony>\n";
    xml << "        <root>\n";
    xml << "          <root-step>" << (result.root_note.empty() ? "C" : result.root_note.substr(0,1)) << "</root-step>\n";
    if (result.root_note.length() > 1 && (result.root_note[1] == '#' || result.root_note[1] == 'b')) {
        xml << "          <root-alter>" << (result.root_note[1] == '#' ? "1" : "-1") << "</root-alter>\n";
    }
    xml << "        </root>\n";
    xml << "        <kind>" << escapeXmlString(result.chord_symbol) << "</kind>\n";
    if (result.is_slash_chord && !result.bass_note_name.empty()) {
        xml << "        <bass>\n";
        xml << "          <bass-step>" << result.bass_note_name.substr(0,1) << "</bass-step>\n";
        if (result.bass_note_name.length() > 1 && (result.bass_note_name[1] == '#' || result.bass_note_name[1] == 'b')) {
            xml << "          <bass-alter>" << (result.bass_note_name[1] == '#' ? "1" : "-1") << "</bass-alter>\n";
        }
        xml << "        </bass>\n";
    }
    xml << "      </harmony>\n";
    xml << "    </measure>\n";
    
    return xml.str();
}

std::string OutputFormatter::generateMusicXMLFooter() const {
    return "</score-partwise>\n";
}

bool OutputFormatter::saveToFile(const FormattedOutput& output, const std::string& filepath) const {
    try {
        if (output.is_binary) {
            std::ofstream file(filepath, std::ios::binary);
            if (!file.is_open()) return false;
            file.write(output.content.data(), output.content.size());
        } else {
            std::ofstream file(filepath);
            if (!file.is_open()) return false;
            file << output.content;
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string OutputFormatter::formatTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string OutputFormatter::formatFloat(float value, int precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

} // namespace ChordLock