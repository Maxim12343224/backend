#include "state_manager.h"
#include <iostream>

StateManager::StateManager(model::Game& game, 
                         std::filesystem::path state_file,
                         std::chrono::milliseconds save_period)
    : game_(game)
    , state_file_(std::move(state_file))
    , save_period_(save_period) {
    
    enabled_ = !state_file_.empty();
}

void StateManager::SetSavePeriod(std::chrono::milliseconds period) {
    save_period_ = period;
}

void StateManager::SaveState() {
    if (!enabled_ || is_saving_.exchange(true)) {
        return;
    }
    
    try {
        serializer::GameSerializer::SaveToFile(game_, state_file_);
    } catch (const std::exception& e) {
        
    }
    
    is_saving_.store(false);
}

bool StateManager::LoadState() {
    if (!enabled_) {
        return false;
    }
    
    try {
        return serializer::GameSerializer::LoadFromFile(game_, state_file_);
    } catch (const std::exception& e) {
        
        throw;
    }
}

void StateManager::OnTick(std::chrono::milliseconds delta) {
    if (!enabled_ || save_period_.count() == 0) {
        return;
    }
    
    time_since_last_save_ += delta;
    if (time_since_last_save_ >= save_period_) {
        SaveState();
        time_since_last_save_ = std::chrono::milliseconds(0);
    }
}

void StateManager::OnShutdown() {
    if (enabled_) {
        SaveState();
    }
}

