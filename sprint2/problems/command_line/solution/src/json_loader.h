#pragma once

#include <filesystem>
#include <string>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {

    void LoadGame(const std::filesystem::path& json_path, model::Game& game);

}