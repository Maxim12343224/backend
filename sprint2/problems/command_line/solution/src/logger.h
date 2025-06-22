#pragma once
#include <chrono>
#include <string_view>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <filesystem>
#include <optional>
#include <sstream>

namespace logger {

    namespace fs = std::filesystem;

    class Logger {
    public:
        static Logger& GetInstance() {
            static Logger instance;
            return instance;
        }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        template <typename... Args>
        void Log(Args&&... args) {
            std::lock_guard lock(mutex_);

            auto now = timestamp_override_.has_value() ?
                timestamp_override_.value() : std::chrono::system_clock::now();

            UpdateLogFile(now);
            WriteTimestamp(now);
            (log_file_ << ... << std::forward<Args>(args)) << std::endl;
        }

        void SetTimestamp(std::chrono::system_clock::time_point timestamp) {
            std::lock_guard lock(mutex_);
            timestamp_override_ = timestamp;
        }

    private:
        Logger() = default;
        ~Logger() {
            if (log_file_.is_open()) {
                log_file_.close();
            }
        }

        void UpdateLogFile(const std::chrono::system_clock::time_point& now) {
            auto new_date = GetDate(now);
            if (current_date_ != new_date) {
                if (log_file_.is_open()) {
                    log_file_.close();
                }

                fs::path log_dir = "/var/log";
                if (!fs::exists(log_dir)) {
                    fs::create_directory(log_dir);
                }

                std::ostringstream filename;
                auto t = std::chrono::system_clock::to_time_t(now);
                auto tm = *std::localtime(&t);
                filename << "sample_log_"
                    << std::put_time(&tm, "%Y_%m_%d") << ".log";

                log_file_.open(log_dir / filename.str(), std::ios::app);
                current_date_ = new_date;
            }
        }

        void WriteTimestamp(const std::chrono::system_clock::time_point& now) {
            auto t = std::chrono::system_clock::to_time_t(now);
            log_file_ << std::put_time(std::localtime(&t), "%F %T") << ": ";
        }

        static std::string GetDate(const std::chrono::system_clock::time_point& tp) {
            auto t = std::chrono::system_clock::to_time_t(tp);
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&t), "%F");
            return oss.str();
        }

        std::ofstream log_file_;
        std::string current_date_;
        std::optional<std::chrono::system_clock::time_point> timestamp_override_;
        std::mutex mutex_;
    };

} // namespace logger