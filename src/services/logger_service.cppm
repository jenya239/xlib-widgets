module;
#include <iostream>
#include <string>
#include <memory>

export module services.logger_service;

export enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

export class LoggerService {
private:
    LogLevel currentLevel = LogLevel::INFO;

public:
    void setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    void debug(const std::string& message) {
        if (currentLevel <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }

    void info(const std::string& message) {
        if (currentLevel <= LogLevel::INFO) {
            std::cout << "[INFO] " << message << std::endl;
        }
    }

    void warning(const std::string& message) {
        if (currentLevel <= LogLevel::WARNING) {
            std::cout << "[WARNING] " << message << std::endl;
        }
    }

    void error(const std::string& message) {
        if (currentLevel <= LogLevel::ERROR) {
            std::cout << "[ERROR] " << message << std::endl;
        }
    }
};