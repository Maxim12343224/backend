#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <ctime>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    mutable std::mutex mutex_;
    std::ofstream file_stream_;
    std::string current_file_date_;

    std::string GetFileTimeStamp(const std::chrono::system_clock::time_point& tp) const {
        const auto t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm;
        localtime_r(&t, &tm);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y_%m_%d");
        return oss.str();
    }

    std::string GetTimeStampImpl(const std::chrono::system_clock::time_point& tp) const {
        const auto t_c = std::chrono::system_clock::to_time_t(tp);
        std::tm tm;
        localtime_r(&t_c, &tm);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%F %T");
        return oss.str();
    }

    void UpdateFileStream(const std::chrono::system_clock::time_point& tp) {
        std::string file_date = GetFileTimeStamp(tp);
        if (!file_stream_.is_open() || current_file_date_ != file_date) {
            if (file_stream_.is_open()) {
                file_stream_.close();
            }
            std::string filename = "/var/log/sample_log_" + file_date + ".log";
            file_stream_.open(filename, std::ios::app);
            if (!file_stream_) {
                throw std::runtime_error("Cannot open log file: " + filename);
            }
            current_file_date_ = file_date;
        }
    }

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    Logger() = default;
    Logger(const Logger&) = delete;

    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto current_time = manual_ts_.value_or(std::chrono::system_clock::now());
        UpdateFileStream(current_time);
        if (file_stream_.is_open()) {
            file_stream_ << GetTimeStampImpl(current_time) << ": ";
            (file_stream_ << ... << args);
            file_stream_ << std::endl;
        }
    }

    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(mutex_);
        manual_ts_ = ts;
    }

    auto GetTimeStamp() const {
        return GetTimeStampImpl(manual_ts_.value_or(std::chrono::system_clock::now()));
    }
};