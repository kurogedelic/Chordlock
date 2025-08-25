#pragma once

#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <stdexcept>

namespace ChordLock {

// Error categories for different subsystems
enum class ErrorCategory {
    NONE = 0,
    INPUT_VALIDATION,
    DATABASE_ERROR,
    INTERVAL_CALCULATION,
    CHORD_IDENTIFICATION,
    MEMORY_ERROR,
    CONFIGURATION_ERROR,
    INTERNAL_ERROR
};

// Specific error codes
enum class ErrorCode {
    SUCCESS = 0,
    
    // Input validation errors
    EMPTY_INPUT,
    TOO_MANY_NOTES,
    INVALID_MIDI_NOTE,
    DUPLICATE_NOTES,
    INVALID_BASS_NOTE,
    
    // Database errors
    DATABASE_NOT_INITIALIZED,
    DATABASE_VALIDATION_FAILED,
    CHORD_NOT_FOUND,
    INVALID_CHORD_PATTERN,
    DATABASE_CORRUPTION,
    
    // Interval calculation errors
    INTERVAL_CALCULATION_FAILED,
    INVALID_INTERVAL_RANGE,
    OCTAVE_OVERFLOW,
    
    // Chord identification errors
    NO_MATCH_FOUND,
    LOW_CONFIDENCE,
    AMBIGUOUS_RESULT,
    NAMING_FAILED,
    
    // Memory errors
    ALLOCATION_FAILED,
    BUFFER_OVERFLOW,
    
    // Configuration errors
    INVALID_MODE,
    INVALID_THRESHOLD,
    UNSUPPORTED_FEATURE,
    
    // Internal errors
    ASSERTION_FAILED,
    UNHANDLED_CASE,
    SYSTEM_ERROR
};

// Error severity levels
enum class ErrorSeverity {
    INFO,       // Informational, operation can continue
    WARNING,    // Warning, might affect quality but operation continues
    ERROR,      // Error, operation failed but system can recover
    CRITICAL    // Critical error, system state compromised
};

// Detailed error information
struct ErrorInfo {
    ErrorCode code;
    ErrorCategory category;
    ErrorSeverity severity;
    std::string message;
    std::string function_name;
    int line_number;
    std::vector<std::string> context_info;
    
    ErrorInfo() : code(ErrorCode::SUCCESS), category(ErrorCategory::NONE), 
                  severity(ErrorSeverity::INFO), line_number(0) {}
    
    ErrorInfo(ErrorCode c, const std::string& msg) 
        : code(c), message(msg), line_number(0) {
        category = getErrorCategory(c);
        severity = getErrorSeverity(c);
    }
    
    ErrorInfo(ErrorCode c, ErrorCategory cat, ErrorSeverity sev, 
              const std::string& msg, const std::string& func = "", int line = 0)
        : code(c), category(cat), severity(sev), message(msg), 
          function_name(func), line_number(line) {}
    
    bool isSuccess() const { return code == ErrorCode::SUCCESS; }
    bool isError() const { return severity >= ErrorSeverity::ERROR; }
    bool isCritical() const { return severity == ErrorSeverity::CRITICAL; }
    
    std::string toString() const;
    
private:
    static ErrorCategory getErrorCategory(ErrorCode code);
    static ErrorSeverity getErrorSeverity(ErrorCode code);
};

// Result wrapper that can contain either a value or an error
template<typename T>
class Result {
public:
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    Result(const ErrorInfo& error) : data_(error) {}
    Result(ErrorCode code, const std::string& message) : data_(ErrorInfo(code, message)) {}
    
    bool isSuccess() const { return std::holds_alternative<T>(data_); }
    bool isError() const { return std::holds_alternative<ErrorInfo>(data_); }
    
    const T& value() const {
        if (isError()) {
            throw std::runtime_error("Attempting to access value of failed Result");
        }
        return std::get<T>(data_);
    }
    
    T& value() {
        if (isError()) {
            throw std::runtime_error("Attempting to access value of failed Result");
        }
        return std::get<T>(data_);
    }
    
    const ErrorInfo& error() const {
        if (isSuccess()) {
            throw std::runtime_error("Attempting to access error of successful Result");
        }
        return std::get<ErrorInfo>(data_);
    }
    
    // Optional-like interface
    T valueOr(const T& default_value) const {
        return isSuccess() ? value() : default_value;
    }
    
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(value()))> {
        if (isError()) {
            return Result<decltype(func(value()))>(error());
        }
        return Result<decltype(func(value()))>(func(value()));
    }
    
private:
    std::variant<T, ErrorInfo> data_;
};

// Specialization for void type
template<>
class Result<void> {
public:
    Result() : has_error_(false) {}
    Result(const ErrorInfo& error) : error_(error), has_error_(true) {}
    Result(ErrorCode code, const std::string& message) : error_(ErrorInfo(code, message)), has_error_(true) {}
    
    bool isSuccess() const { return !has_error_; }
    bool isError() const { return has_error_; }
    
    void value() const {
        if (isError()) {
            throw std::runtime_error("Attempting to access value of failed Result");
        }
        // void return
    }
    
    const ErrorInfo& error() const {
        if (isSuccess()) {
            throw std::runtime_error("Attempting to access error of successful Result");
        }
        return error_;
    }
    
private:
    ErrorInfo error_;
    bool has_error_;
};

// Macro for creating error info with location
#define CHORD_ERROR(code, message) \
    ErrorInfo(code, ErrorCategory::getErrorCategory(code), \
              ErrorCategory::getErrorSeverity(code), message, __func__, __LINE__)

// Input validation utilities
class InputValidator {
public:
    static Result<std::vector<int>> validateMidiNotes(const std::vector<int>& notes);
    static Result<int> validateBassNote(int bass_note);
    static Result<void> validateNoteRange(int note);
    static Result<std::vector<int>> validateAndCleanNotes(const std::vector<int>& notes);
    
private:
    static constexpr int MIN_MIDI_NOTE = 0;
    static constexpr int MAX_MIDI_NOTE = 127;
    static constexpr size_t MAX_CHORD_SIZE = 16;
    static constexpr size_t MIN_CHORD_SIZE = 1;
};

// Error logging and reporting
class ErrorLogger {
public:
    static void logError(const ErrorInfo& error);
    static void logWarning(const std::string& message);
    static void logInfo(const std::string& message);
    
    static void setLogLevel(ErrorSeverity min_level);
    static void enableFileLogging(const std::string& filepath);
    static void disableFileLogging();
    
    static std::vector<ErrorInfo> getRecentErrors(size_t count = 10);
    static void clearErrorHistory();
    
private:
    static ErrorSeverity min_log_level_;
    static std::string log_file_path_;
    static bool file_logging_enabled_;
    static std::vector<ErrorInfo> error_history_;
    static constexpr size_t MAX_ERROR_HISTORY = 100;
};

// Exception types for critical errors
class ChordLockException : public std::runtime_error {
public:
    ChordLockException(const ErrorInfo& error) 
        : std::runtime_error(error.toString()), error_info_(error) {}
    
    const ErrorInfo& getErrorInfo() const { return error_info_; }
    
private:
    ErrorInfo error_info_;
};

class DatabaseException : public ChordLockException {
public:
    DatabaseException(const std::string& message) 
        : ChordLockException(ErrorInfo(ErrorCode::DATABASE_CORRUPTION, 
                                     ErrorCategory::DATABASE_ERROR, 
                                     ErrorSeverity::CRITICAL, message)) {}
};

class ValidationException : public ChordLockException {
public:
    ValidationException(ErrorCode code, const std::string& message) 
        : ChordLockException(ErrorInfo(code, ErrorCategory::INPUT_VALIDATION, 
                                     ErrorSeverity::ERROR, message)) {}
};

// Safe operation wrappers
template<typename F>
auto safeExecute(F&& func, const std::string& operation_name) -> Result<decltype(func())> {
    try {
        return Result<decltype(func())>(func());
    } catch (const ChordLockException& e) {
        ErrorInfo error = e.getErrorInfo();
        error.context_info.push_back("Operation: " + operation_name);
        return Result<decltype(func())>(error);
    } catch (const std::exception& e) {
        ErrorInfo error(ErrorCode::SYSTEM_ERROR, ErrorCategory::INTERNAL_ERROR, 
                       ErrorSeverity::CRITICAL, e.what());
        error.context_info.push_back("Operation: " + operation_name);
        return Result<decltype(func())>(error);
    } catch (...) {
        ErrorInfo error(ErrorCode::UNHANDLED_CASE, ErrorCategory::INTERNAL_ERROR, 
                       ErrorSeverity::CRITICAL, "Unknown exception caught");
        error.context_info.push_back("Operation: " + operation_name);
        return Result<decltype(func())>(error);
    }
}

} // namespace ChordLock