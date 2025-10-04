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
    
    if (enabled_) {
        auto parent_path = state_file_.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
        }
    }
}

void StateManager::SetSavePeriod(std::chrono::milliseconds period) {
    save_period_ = period;
}

void StateManager::SaveState() {
    if (!enabled_) {
        return;
    }
    
    bool expected = false;
    if (!is_saving_.compare_exchange_strong(expected, true)) {
        return;
    }
    
    try {
        bool result = serializer::GameSerializer::SaveToFile(game_, state_file_);
        
        json::value save_data{
            {"state_file", state_file_.string()},
            {"success", result}
        };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, save_data)
            << "state saved";
            
    } catch (const std::exception& e) {
        json::value error_data{
            {"state_file", state_file_.string()},
            {"error", e.what()}
        };
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(logger::additional_data, error_data)
            << "state save error";
    }
    
    is_saving_.store(false);
}

bool StateManager::LoadState() {
    if (!enabled_) {
        return false;
    }
    
    try {
        if (!std::filesystem::exists(state_file_)) {
            json::value info_data{
                {"state_file", state_file_.string()},
                {"info", "state file does not exist, starting fresh"}
            };
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, info_data)
                << "state load";
            return false;
        }
        
        bool result = serializer::GameSerializer::LoadFromFile(game_, state_file_);
        
        json::value load_data{
            {"state_file", state_file_.string()},
            {"success", result}
        };
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(logger::additional_data, load_data)
            << "state loaded";
            
        return result;
        
    } catch (const std::exception& e) {
        json::value error_data{
            {"state_file", state_file_.string()},
            {"error", e.what()}
        };
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(logger::additional_data, error_data)
            << "state load error";
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