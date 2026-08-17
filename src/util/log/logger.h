#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

class Logger {
private:
    static std::string getLogFilePath() {
#ifdef _WIN32
        // Try to use %LOCALAPPDATA%\app\logs
        wchar_t* localAppData = nullptr;
        if (GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0) > 0) {
            DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
            localAppData = new wchar_t[size];
            GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, size);

            std::filesystem::path logDir = std::filesystem::path(localAppData) / "app" / "logs";
            delete[] localAppData;

            try {
                if (!std::filesystem::exists(logDir)) {
                    std::filesystem::create_directories(logDir);
                }
                std::filesystem::path logFile = logDir / "app.log";
                return logFile.string();
            } catch (...) {
                // Continue to next fallback
            }
        }

        // Fallback: try current directory (works if started from writable location)
        try {
            std::filesystem::path logDir = "logs";
            if (!std::filesystem::exists(logDir)) {
                std::filesystem::create_directories(logDir);
            }
            return "logs/app.log";
        } catch (...) {
            // Disable file logging if all attempts fail
            return "";
        }
#else
        // For Linux, use home directory
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            std::filesystem::path logDir = std::filesystem::path(homeDir) / ".app" / "logs";
            try {
                if (!std::filesystem::exists(logDir)) {
                    std::filesystem::create_directories(logDir);
                }
                return (logDir / "app.log").string();
            } catch (...) {
                // Continue to fallback
            }
        }
        return "";
#endif
    }

public:
    static void init() {
        try {
            // console
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("[%H:%M:%S] [%^%l%$] [%s:%#] %v");

            std::vector<spdlog::sink_ptr> sinks{console_sink};

            // file rotating (optional - only if we have a writable path)
            std::string logPath = getLogFilePath();
            if (!logPath.empty()) {
                try {
                    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                        logPath,
                        1024 * 1024 * 5,
                        3
                    );
                    file_sink->set_level(spdlog::level::trace);
                    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] [%s:%#] %v");
                    sinks.push_back(file_sink);
                } catch (...) {
                    // File logger failed, continue with console only
                }
            }

            // Create logger with available sinks
            auto logger = std::make_shared<spdlog::logger>("app", sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::trace);
            logger->flush_on(spdlog::level::err);

            spdlog::set_default_logger(logger);
        } catch (const std::exception& e) {
            // If logger initialization fails completely, create a console-only logger
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("[%H:%M:%S] [%^%l%$] [%s:%#] %v");

            auto logger = std::make_shared<spdlog::logger>("app", console_sink);
            logger->set_level(spdlog::level::trace);
            spdlog::set_default_logger(logger);

            SPDLOG_ERROR("Failed to initialize logger: {}", e.what());
        }
    }

    static void shutdown() {
        spdlog::shutdown();
    }
};

#define LOG_TRACE(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::err, __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::critical, __VA_ARGS__)
