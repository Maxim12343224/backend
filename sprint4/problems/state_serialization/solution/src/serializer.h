#pragma once
#include "model.h"
#include <boost/json.hpp>
#include <filesystem>

namespace serializer {

    class GameSerializer {
    public:
        static boost::json::value SerializeGame(const model::Game& game);
        static void DeserializeGame(model::Game& game, const boost::json::value& data);
        static bool SaveToFile(const model::Game& game, const std::filesystem::path& path);
        static bool LoadFromFile(model::Game& game, const std::filesystem::path& path);
    };

} // namespace serializer