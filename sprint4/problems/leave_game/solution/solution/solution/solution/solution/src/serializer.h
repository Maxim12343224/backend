#pragma once
#include "model.h"
#include <boost/json.hpp>
#include <filesystem>

namespace serializer {

    namespace json = boost::json;

    // DTO классы - промежуточный слой для сериализации
    struct SerDog {
        std::string name;
        model::Point position;
        model::Point speed;
        model::Direction direction;

        // Конвертация из model::Dog
        static SerDog FromModel(const model::Dog& dog) {
            return {
                dog.GetName(),
                dog.GetPosition(),
                dog.GetSpeed(),
                dog.GetDirection()
            };
        }

        // Конвертация в model::Dog
        model::Dog ToModel() const {
            model::Dog dog(name, position);
            dog.SetSpeed(speed);
            dog.SetDirection(direction);
            return dog;
        }
    };

    struct SerLostObject {
        size_t id;
        size_t type;
        model::Point position;
        int value;

        static SerLostObject FromModel(const model::LostObject& obj) {
            return { obj.id, obj.type, obj.position, obj.value };
        }

        model::LostObject ToModel() const {
            return { id, type, position, value };
        }
    };

    struct SerPlayer {
        uint32_t id;
        std::string token;
        SerDog dog;
        int score;
        size_t bag_capacity;
        std::vector<SerLostObject> bag;
        double join_time;
        double last_move_time;

        static SerPlayer FromModel(const std::shared_ptr<model::Player>& player) {
            std::vector<SerLostObject> ser_bag;
            for (const auto& item : player->GetBag()) {
                ser_bag.push_back(SerLostObject::FromModel(item));
            }

            return {
                *player->GetId(),
                *player->GetToken(),
                SerDog::FromModel(player->GetDog()),
                player->GetScore(),
                player->GetBagCapacity(),
                ser_bag,
                player->GetJoinTime(),
                player->GetLastMoveTime()
            };
        }
    };

    struct SerGameSession {
        uint32_t id;
        std::string map_id;
        double dog_speed;
        size_t next_lost_object_id;
        std::vector<SerPlayer> players;
        std::vector<SerLostObject> lost_objects;

        static SerGameSession FromModel(const std::shared_ptr<model::GameSession>& session) {
            std::vector<SerPlayer> ser_players;
            for (const auto& player : session->GetPlayers()) {
                ser_players.push_back(SerPlayer::FromModel(player));
            }

            std::vector<SerLostObject> ser_lost_objects;
            for (const auto& obj : session->GetLostObjects()) {
                ser_lost_objects.push_back(SerLostObject::FromModel(obj));
            }

            return {
                *session->GetId(),
                *session->GetMap().GetId(),
                session->GetDogSpeed(),
                session->GetNextLostObjectId(),
                ser_players,
                ser_lost_objects
            };
        }
    };

    class GameSerializer {
    public:
        static boost::json::value SerializeGame(const model::Game& game);
        static void DeserializeGame(model::Game& game, const boost::json::value& data);
        static bool SaveToFile(const model::Game& game, const std::filesystem::path& path);
        static bool LoadFromFile(model::Game& game, const std::filesystem::path& path);
    };

} // namespace serializer