#include "state_manager.h"
#include <iostream>

StateManager::StateManager(model::Game& game, 
                         std::filesystem::path state_file,
                         std::chrono::milliseconds save_period)
    : game_(game)
    , state_file_(std::move(state_file))
    , save_period_(save_period) {
    
    enabled_ = !state_file_.empty();
    
    if (enabled_) {
        // Создаем директорию для файла состояния, если нужно
        auto parent_path = state_file_.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::error_code ec;
            if (std::filesystem::create_directories(parent_path, ec)) {
                std::cout << "Created state directory: " << parent_path << std::endl;
            } else {
                std::cerr << "Warning: Could not create state directory " 
                          << parent_path << ": " << ec.message() << std::endl;
            }
        }
        
        std::cout << "State management enabled. File: " << state_file_ << std::endl;
        if (save_period_.count() > 0) {
            std::cout << "Auto-save period: " << save_period_.count() << " ms" << std::endl;
        }
    } else {
        std::cout << "State management disabled" << std::endl;
    }
}

void StateManager::SetSavePeriod(std::chrono::milliseconds period) {
    save_period_ = period;
    if (enabled_ && save_period_.count() > 0) {
        std::cout << "Auto-save period set to: " << save_period_.count() << " ms" << std::endl;
    }
}

void StateManager::SaveState() {
    if (!enabled_ || is_saving_.exchange(true)) {
        return;
    }
    
    try {
        std::cout << "Saving game state to: " << state_file_ << std::endl;
        
        if (serializer::GameSerializer::SaveToFile(game_, state_file_)) {
            std::cout << "Game state saved successfully" << std::endl;
        } else {
            std::cerr << "Failed to save game state" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving game state: " << e.what() << std::endl;
    }
    
    is_saving_.store(false);
}

bool StateManager::LoadState() {
    if (!enabled_) {
        std::cout << "State management disabled, starting with clean state" << std::endl;
        return false;
    }
    
    try {
        std::cout << "Loading game state from: " << state_file_ << std::endl;
        
        if (serializer::GameSerializer::LoadFromFile(game_, state_file_)) {
            std::cout << "Game state loaded successfully" << std::endl;
            return true;
        } else {
            std::cout << "Starting with clean state" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading game state: " << e.what() << std::endl;
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
        std::cout << "Shutdown: saving game state..." << std::endl;
        SaveState();
    }
}