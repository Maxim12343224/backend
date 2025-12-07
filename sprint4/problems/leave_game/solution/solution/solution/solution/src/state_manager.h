#pragma once
#include "model.h"
#include "serializer.h"
#include <chrono>
#include <filesystem>
#include <atomic>

class StateManager {
public:
    StateManager(model::Game& game,
        std::filesystem::path state_file = "",
        std::chrono::milliseconds save_period = std::chrono::milliseconds(0));

    void SetSavePeriod(std::chrono::milliseconds period);
    void SaveState();
    bool LoadState();
    void OnTick(std::chrono::milliseconds delta);
    void OnShutdown();
    bool IsEnabled() const { return enabled_; }
    const std::filesystem::path& GetStateFilePath() const { return state_file_; }

private:
    model::Game& game_;
    std::filesystem::path state_file_;
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds time_since_last_save_{ 0 };
    bool enabled_ = false;
    std::atomic<bool> is_saving_{ false };
};