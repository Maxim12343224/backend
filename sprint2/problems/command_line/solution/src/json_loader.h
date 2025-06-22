#pragma once

#include <filesystem>
#include <memory>
#include <boost/json.hpp>
#include "game.h"
#include "model.h"

namespace json_loader {

	std::unique_ptr<model::Game> LoadGame(const std::filesystem::path& json_path);

	namespace detail {
		model::Road ParseRoad(const boost::json::object& road_obj);
		model::Building ParseBuilding(const boost::json::object& building_obj);
		model::Office ParseOffice(const boost::json::object& office_obj);
		model::Map ParseMap(const boost::json::object& map_obj, double default_dog_speed);
	} // namespace detail

}  // namespace json_loader