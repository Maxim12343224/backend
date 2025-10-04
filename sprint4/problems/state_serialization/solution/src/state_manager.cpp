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
    std::cout << "DEBUG: SaveState called" << std::endl;
    
    if (!enabled_ || is_saving_.exchange(true)) {
        std::cout << "DEBUG: SaveState skipped - disabled or already saving" << std::endl;
        return;
    }
    
    std::cout << "DEBUG: Attempting to save state to: " << state_file_ << std::endl;
    
    try {
        bool result = serializer::GameSerializer::SaveToFile(game_, state_file_);
        std::cout << "DEBUG: SaveState result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "DEBUG: SaveState error: " << e.what() << std::endl;
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
    std::cout << "DEBUG: OnTick called, enabled: " << enabled_ 
              << ", save_period: " << save_period_.count() 
              << ", time_since_last_save: " << time_since_last_save_.count() << std::endl;
    
    if (!enabled_ || save_period_.count() == 0) {
        std::cout << "DEBUG: State saving disabled or period is 0" << std::endl;
        return;
    }
    
    time_since_last_save_ += delta;
    std::cout << "DEBUG: time_since_last_save after add: " << time_since_last_save_.count() << std::endl;
    
    if (time_since_last_save_ >= save_period_) {
        std::cout << "DEBUG: Time to save! Calling SaveState()" << std::endl;
        SaveState();
        time_since_last_save_ = std::chrono::milliseconds(0);
    }
}

void StateManager::OnShutdown() {
    if (enabled_) {
        SaveState();
    }
}

