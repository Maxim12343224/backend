#include "serializer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace serializer {

// Реализация SerDog
SerDog SerDog::FromModel(const model::Dog& dog) {
    return {
        dog.GetName(),
        dog.GetPosition(),
        dog.GetSpeed(),
        dog.GetDirection()
    };
}

model::Dog SerDog::ToModel() const {
    model::Dog dog(name, position);
    dog.SetSpeed(speed);
    dog.SetDirection(direction);
    return dog;
}

// Реализация SerLostObject
SerLostObject SerLostObject::FromModel(const model::LostObject& obj) {
    return { obj.id, obj.type, obj.position, obj.value };
}

model::LostObject SerLostObject::ToModel() const {
    return { id, type, position, value };
}

// Реализация SerPlayer
SerPlayer SerPlayer::FromModel(const std::shared_ptr<model::Player>& player) {
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
        player->GetJoinTime()
    };
}

// Реализация SerGameSession
SerGameSession SerGameSession::FromModel(const std::shared_ptr<model::GameSession>& session) {
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
        session->GetRetirementTime(),
        session->GetNextLostObjectId(),
        ser_players,
        ser_lost_objects
    };
}

} // namespace serializer