#include "my_logger.h"
#include <string_view>
#include <thread>
#include <iostream>
#include <vector>

using namespace std::literals;

void ThreadTest(int thread_id) {
    for (int i = 0; i < 5; ++i) {
        LOG("Thread "sv, thread_id, " iteration "sv, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RunTests() {
    // Тест 1: Базовое логирование
    std::cout << "=== Basic logging test ===\n";
    Logger::GetInstance().SetTimestamp(std::chrono::system_clock::time_point{ 1000000s });
    LOG("Basic types: "sv, 42, " ", 3.14, " ", "string"s, " ", true);

    // Тест 2: Смена даты
    std::cout << "=== Date change test ===\n";
    Logger::GetInstance().SetTimestamp(std::chrono::system_clock::time_point{ 0s }); // 1970 год
    LOG("This should go to 1970 log file");

    // Тест 3: Многопоточность
    std::cout << "=== Thread safety test ===\n";
    Logger::GetInstance().SetTimestamp(std::chrono::system_clock::time_point{ 2000000s });
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(ThreadTest, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    // Тест 4: Большое количество аргументов
    std::cout << "=== Many arguments test ===\n";
    LOG(1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j");

    // Тест 5: Массовое логирование
    std::cout << "=== Mass logging test ===\n";
    const int attempts = 1000;
    for (int i = 0; i < attempts; ++i) {
        auto ts = std::chrono::system_clock::time_point{
            std::chrono::seconds(2000000 + i * 100) };
        Logger::GetInstance().SetTimestamp(ts);
        LOG("Message ", i, " of ", attempts);
    }
}

int main() {
    try {
        RunTests();
        std::cout << "All tests completed successfully!\n";
        std::cout << "Check log files in /var/log/sample_log_*.log\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}