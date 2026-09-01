#pragma once
// fluxerpp/util/Logger.h
//
// Minimal leveled logger. Replaces the scattered std::cout/std::cerr calls
// that used to live directly in GatewayClient.cpp / RestClient.cpp / env.h.
//
// Two things this buys you over raw iostream calls:
//   1. A single place to turn debug spam on/off (set_level).
//   2. redact() so secrets (bot tokens, Authorization headers) never hit
//      stdout/stderr even when debug logging is on.

#include <string>
#include <iostream>
#include <mutex>
#include <atomic>

namespace fluxerpp::util {

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3, Silent = 4 };

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_level(LogLevel lvl) { level_.store(static_cast<int>(lvl)); }
    LogLevel level() const { return static_cast<LogLevel>(level_.load()); }

    void debug(const std::string& msg) { log(LogLevel::Debug, msg, std::cout); }
    void info(const std::string& msg)  { log(LogLevel::Info,  msg, std::cout); }
    void warn(const std::string& msg)  { log(LogLevel::Warn,  msg, std::cerr); }
    void error(const std::string& msg) { log(LogLevel::Error, msg, std::cerr); }

    // Replace a secret value with a short fingerprint so logs stay useful
    // for debugging ("token looked non-empty, length N") without leaking it.
    static std::string redact(const std::string& secret) {
        if (secret.empty()) return "<empty>";
        std::string tail = secret.size() > 4 ? secret.substr(secret.size() - 4) : "";
        return "<redacted len=" + std::to_string(secret.size()) +
               (tail.empty() ? "" : " ...suffix=" + tail) + ">";
    }

    // Best-effort: redact anything that looks like `Authorization: Bot X`
    // or `token=X` / `"token": "X"` inside an arbitrary log line.
    static std::string redact_line(std::string line) {
        redact_after(line, "Authorization: Bot ");
        redact_after(line, "\"token\":\"");
        redact_after(line, "\"token\": \"");
        return line;
    }

private:
    Logger() = default;
    std::atomic<int> level_{static_cast<int>(LogLevel::Info)};
    std::mutex mutex_;

    void log(LogLevel lvl, const std::string& msg, std::ostream& os) {
        if (static_cast<int>(lvl) < level_.load()) return;
        std::lock_guard<std::mutex> lk(mutex_);
        os << prefix(lvl) << msg << "\n";
    }

    static const char* prefix(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::Debug: return "[fluxerpp][debug] ";
            case LogLevel::Info:  return "[fluxerpp][info]  ";
            case LogLevel::Warn:  return "[fluxerpp][warn]  ";
            case LogLevel::Error: return "[fluxerpp][error] ";
            default: return "[fluxerpp] ";
        }
    }

    static void redact_after(std::string& line, const std::string& marker) {
        auto pos = line.find(marker);
        if (pos == std::string::npos) return;
        auto start = pos + marker.size();
        auto end = line.find_first_of("\"\r\n", start);
        if (end == std::string::npos) end = line.size();
        line.replace(start, end - start, "<redacted>");
    }
};

} // namespace fluxerpp::util