#include "ErrorHandling.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <ctime>
#include <iomanip>

namespace ChordLock {

// Static member definitions
ErrorSeverity ErrorLogger::min_log_level_ = ErrorSeverity::WARNING;
std::string ErrorLogger::log_file_path_;
bool ErrorLogger::file_logging_enabled_ = false;
std::vector<ErrorInfo> ErrorLogger::error_history_;

// ErrorInfo implementation
std::string ErrorInfo::toString() const {
    std::ostringstream oss;
    
    // Severity prefix
    switch (severity) {
        case ErrorSeverity::INFO:    oss << "[INFO] "; break;
        case ErrorSeverity::WARNING: oss << "[WARN] "; break;
        case ErrorSeverity::ERROR:   oss << "[ERROR] "; break;
        case ErrorSeverity::CRITICAL: oss << "[CRITICAL] "; break;
    }
    
    // Category and code
    oss << "[" << static_cast<int>(category) << ":" << static_cast<int>(code) << "] ";
    
    // Message
    oss << message;
    
    // Location info
    if (!function_name.empty()) {
        oss << " (in " << function_name;
        if (line_number > 0) {
            oss << ":" << line_number;
        }
        oss << ")";
    }
    
    // Context info
    for (const auto& context : context_info) {
        oss << " | " << context;
    }
    
    return oss.str();
}

ErrorCategory ErrorInfo::getErrorCategory(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return ErrorCategory::NONE;
            
        case ErrorCode::EMPTY_INPUT:
        case ErrorCode::TOO_MANY_NOTES:
        case ErrorCode::INVALID_MIDI_NOTE:
        case ErrorCode::DUPLICATE_NOTES:
        case ErrorCode::INVALID_BASS_NOTE:
            return ErrorCategory::INPUT_VALIDATION;
            
        case ErrorCode::DATABASE_NOT_INITIALIZED:
        case ErrorCode::DATABASE_VALIDATION_FAILED:
        case ErrorCode::CHORD_NOT_FOUND:
        case ErrorCode::INVALID_CHORD_PATTERN:
        case ErrorCode::DATABASE_CORRUPTION:
            return ErrorCategory::DATABASE_ERROR;
            
        case ErrorCode::INTERVAL_CALCULATION_FAILED:
        case ErrorCode::INVALID_INTERVAL_RANGE:
        case ErrorCode::OCTAVE_OVERFLOW:
            return ErrorCategory::INTERVAL_CALCULATION;
            
        case ErrorCode::NO_MATCH_FOUND:
        case ErrorCode::LOW_CONFIDENCE:
        case ErrorCode::AMBIGUOUS_RESULT:
        case ErrorCode::NAMING_FAILED:
            return ErrorCategory::CHORD_IDENTIFICATION;
            
        case ErrorCode::ALLOCATION_FAILED:
        case ErrorCode::BUFFER_OVERFLOW:
            return ErrorCategory::MEMORY_ERROR;
            
        case ErrorCode::INVALID_MODE:
        case ErrorCode::INVALID_THRESHOLD:
        case ErrorCode::UNSUPPORTED_FEATURE:
            return ErrorCategory::CONFIGURATION_ERROR;
            
        case ErrorCode::ASSERTION_FAILED:
        case ErrorCode::UNHANDLED_CASE:
        case ErrorCode::SYSTEM_ERROR:
        default:
            return ErrorCategory::INTERNAL_ERROR;
    }
}

ErrorSeverity ErrorInfo::getErrorSeverity(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return ErrorSeverity::INFO;
            
        // Warnings - operation can continue with degraded functionality
        case ErrorCode::DUPLICATE_NOTES:
        case ErrorCode::LOW_CONFIDENCE:
        case ErrorCode::AMBIGUOUS_RESULT:
        case ErrorCode::DATABASE_VALIDATION_FAILED:
            return ErrorSeverity::WARNING;
            
        // Errors - operation failed but system can recover
        case ErrorCode::EMPTY_INPUT:
        case ErrorCode::TOO_MANY_NOTES:
        case ErrorCode::INVALID_MIDI_NOTE:
        case ErrorCode::INVALID_BASS_NOTE:
        case ErrorCode::CHORD_NOT_FOUND:
        case ErrorCode::INVALID_CHORD_PATTERN:
        case ErrorCode::INTERVAL_CALCULATION_FAILED:
        case ErrorCode::INVALID_INTERVAL_RANGE:
        case ErrorCode::OCTAVE_OVERFLOW:
        case ErrorCode::NO_MATCH_FOUND:
        case ErrorCode::NAMING_FAILED:
        case ErrorCode::INVALID_MODE:
        case ErrorCode::INVALID_THRESHOLD:
        case ErrorCode::UNSUPPORTED_FEATURE:
            return ErrorSeverity::ERROR;
            
        // Critical errors - system state compromised
        case ErrorCode::DATABASE_NOT_INITIALIZED:
        case ErrorCode::DATABASE_CORRUPTION:
        case ErrorCode::ALLOCATION_FAILED:
        case ErrorCode::BUFFER_OVERFLOW:
        case ErrorCode::ASSERTION_FAILED:
        case ErrorCode::UNHANDLED_CASE:
        case ErrorCode::SYSTEM_ERROR:
        default:
            return ErrorSeverity::CRITICAL;
    }
}

// InputValidator implementation
Result<std::vector<int>> InputValidator::validateMidiNotes(const std::vector<int>& notes) {
    // Check for empty input
    if (notes.empty()) {
        return Result<std::vector<int>>(ErrorCode::EMPTY_INPUT, 
                                       "Input note vector is empty");
    }
    
    // Check for too many notes
    if (notes.size() > MAX_CHORD_SIZE) {
        return Result<std::vector<int>>(ErrorCode::TOO_MANY_NOTES, 
                                       "Too many notes in chord (max " + 
                                       std::to_string(MAX_CHORD_SIZE) + ")");
    }
    
    // Validate each note
    for (size_t i = 0; i < notes.size(); ++i) {
        auto validation_result = validateNoteRange(notes[i]);
        if (validation_result.isError()) {
            ErrorInfo error = validation_result.error();
            error.context_info.push_back("Note index: " + std::to_string(i));
            error.context_info.push_back("Note value: " + std::to_string(notes[i]));
            return Result<std::vector<int>>(error);
        }
    }
    
    return Result<std::vector<int>>(notes);
}

Result<int> InputValidator::validateBassNote(int bass_note) {
    auto validation_result = validateNoteRange(bass_note);
    if (validation_result.isError()) {
        ErrorInfo error = validation_result.error();
        error.context_info.push_back("Bass note validation");
        return Result<int>(error);
    }
    return Result<int>(bass_note);
}

Result<void> InputValidator::validateNoteRange(int note) {
    if (note < MIN_MIDI_NOTE || note > MAX_MIDI_NOTE) {
        return Result<void>(ErrorCode::INVALID_MIDI_NOTE, 
                           "MIDI note " + std::to_string(note) + 
                           " out of valid range [" + std::to_string(MIN_MIDI_NOTE) + 
                           ", " + std::to_string(MAX_MIDI_NOTE) + "]");
    }
    return Result<void>(ErrorCode::SUCCESS, "");
}

Result<std::vector<int>> InputValidator::validateAndCleanNotes(const std::vector<int>& notes) {
    // First validate the input
    auto validation_result = validateMidiNotes(notes);
    if (validation_result.isError()) {
        return validation_result;
    }
    
    // Remove duplicates while preserving order
    std::vector<int> cleaned_notes;
    std::set<int> seen_notes;
    
    for (int note : notes) {
        if (seen_notes.find(note) == seen_notes.end()) {
            cleaned_notes.push_back(note);
            seen_notes.insert(note);
        }
    }
    
    // Check if duplicates were found (warning, not error)
    if (cleaned_notes.size() != notes.size()) {
        ErrorLogger::logWarning("Duplicate notes removed from input. Original size: " + 
                                std::to_string(notes.size()) + ", cleaned size: " + 
                                std::to_string(cleaned_notes.size()));
    }
    
    return Result<std::vector<int>>(cleaned_notes);
}

// ErrorLogger implementation
void ErrorLogger::logError(const ErrorInfo& error) {
    if (error.severity < min_log_level_) {
        return;
    }
    
    std::string log_message = error.toString();
    
    // Console output
    if (error.severity >= ErrorSeverity::ERROR) {
        std::cerr << log_message << std::endl;
    } else {
        std::cout << log_message << std::endl;
    }
    
    // File output
    if (file_logging_enabled_ && !log_file_path_.empty()) {
        std::ofstream log_file(log_file_path_, std::ios::app);
        if (log_file.is_open()) {
            // Add timestamp
            auto now = std::time(nullptr);
            auto* local_time = std::localtime(&now);
            log_file << "[" << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] " 
                     << log_message << std::endl;
        }
    }
    
    // Add to error history
    error_history_.push_back(error);
    if (error_history_.size() > MAX_ERROR_HISTORY) {
        error_history_.erase(error_history_.begin());
    }
}

void ErrorLogger::logWarning(const std::string& message) {
    ErrorInfo warning(ErrorCode::SUCCESS, ErrorCategory::NONE, 
                     ErrorSeverity::WARNING, message);
    logError(warning);
}

void ErrorLogger::logInfo(const std::string& message) {
    ErrorInfo info(ErrorCode::SUCCESS, ErrorCategory::NONE, 
                  ErrorSeverity::INFO, message);
    logError(info);
}

void ErrorLogger::setLogLevel(ErrorSeverity min_level) {
    min_log_level_ = min_level;
}

void ErrorLogger::enableFileLogging(const std::string& filepath) {
    log_file_path_ = filepath;
    file_logging_enabled_ = true;
}

void ErrorLogger::disableFileLogging() {
    file_logging_enabled_ = false;
    log_file_path_.clear();
}

std::vector<ErrorInfo> ErrorLogger::getRecentErrors(size_t count) {
    if (count >= error_history_.size()) {
        return error_history_;
    }
    
    return std::vector<ErrorInfo>(error_history_.end() - count, error_history_.end());
}

void ErrorLogger::clearErrorHistory() {
    error_history_.clear();
}

} // namespace ChordLock