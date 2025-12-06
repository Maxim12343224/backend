#include "state_manager.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/json.hpp>
#include "logger.h"

namespace json = boost::json;

StateManager::StateManager(model::Game& game, 
                         std::filesystem::path state_file,
                         std::chrono::milliseconds save_period)
    : game_(game)
    , state_file_(std::move(state_file))
    , save_period_(save_period) {
    
    enabled_ = !state_file_.empty();
    
    
    std::cout << "DEBUG: StateManager initialized - enabled: " << enabled_ 
              << ", file: " << state_file_.string()
              << ", period: " << save_period_.count() << "ms" << std::endl;
    
    if (enabled_) {
        auto parent_path = state_file_.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
        }
    }
}

void StateManager::SetSavePeriod(std::chrono::milliseconds period) {
    save_period_ = period;
    std::cout << "DEBUG: Save period set to: " << save_period_.count() << "ms" << std::endl;
}

void StateManager::SaveState() {
    if (!enabled_) {
        std::cout << "DEBUG: SaveState skipped - not enabled" << std::endl;
        return;
    }
    
    bool expected = false;
    if (!is_saving_.compare_exchange_strong(expected, true)) {
        std::cout << "DEBUG: SaveState skipped - already saving" << std::endl;
        return;
    }
    
    std::cout << "DEBUG: Starting SaveState to: " << state_file_.string() << std::endl;
    
    try {
        bool result = serializer::GameSerializer::SaveToFile(game_, state_file_);
        
        if (result) {
            std::cout << "DEBUG: SaveState SUCCESS - file created: " << state_file_.string() << std::endl;
            BOOST_LOG_TRIVIAL(info) << "State saved successfully to: " << state_file_.string();
        } else {
            std::cout << "DEBUG: SaveState FAILED - serializer returned false" << std::endl;
            BOOST_LOG_TRIVIAL(error) << "Failed to save state to: " << state_file_.string();
        }
            
    } catch (const std::exception& e) {
        std::cout << "DEBUG: SaveState EXCEPTION: " << e.what() << std::endl;
        BOOST_LOG_TRIVIAL(error) << "State save error: " << e.what();
    }
    
    is_saving_.store(false);
}

bool StateManager::LoadState() {
    if (!enabled_) {
        std::cout << "DEBUG: LoadState skipped - not enabled" << std::endl;
        return false;
    }
    
    std::cout << "DEBUG: Attempting LoadState from: " << state_file_.string() << std::endl;
    
    try {
        if (!std::filesystem::exists(state_file_)) {
            std::cout << "DEBUG: LoadState - file does not exist, starting fresh" << std::endl;
            BOOST_LOG_TRIVIAL(info) << "State file does not exist, starting fresh: " << state_file_.string();
            return false;
        }
        
        bool result = serializer::GameSerializer::LoadFromFile(game_, state_file_);
        
        if (result) {
            std::cout << "DEBUG: LoadState SUCCESS" << std::endl;
            BOOST_LOG_TRIVIAL(info) << "State loaded successfully from: " << state_file_.string();
        } else {
            std::cout << "DEBUG: LoadState FAILED - serializer returned false" << std::endl;
            BOOST_LOG_TRIVIAL(warning) << "Failed to load state from: " << state_file_.string();
        }
            
        return result;
        
    } catch (const std::exception& e) {
        std::cout << "DEBUG: LoadState EXCEPTION: " << e.what() << std::endl;
        BOOST_LOG_TRIVIAL(error) << "State load error: " << e.what();
        throw;
    }
}

void StateManager::OnTick(std::chrono::milliseconds delta) {
    if (!enabled_ || save_period_.count() == 0) {
        return;
    }
    
    time_since_last_save_ += delta;
    
    std::cout << "DEBUG: OnTick - time_since_last_save: " << time_since_last_save_.count() 
              << "ms, save_period: " << save_period_.count() << "ms" << std::endl;
    
    if (time_since_last_save_ >= save_period_) {
        std::cout << "DEBUG: OnTick - triggering auto-save" << std::endl;
        SaveState();
        time_since_last_save_ = std::chrono::milliseconds(0);
    }
}

void StateManager::OnShutdown() {
    std::cout << "DEBUG: OnShutdown called - saving state" << std::endl;
    if (enabled_) {
        SaveState();
    }
}