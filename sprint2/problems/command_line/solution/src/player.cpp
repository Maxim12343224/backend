#include "player.h"
#include <random>

namespace model {

    std::shared_ptr<Player> Players::Add(std::string name, std::shared_ptr<Dog> dog) {
        auto player = std::make_shared<Player>(static_cast<uint32_t>(players_.size()),
            std::move(name), std::move(dog));
        players_.push_back(player);
        token_to_player_[player->GetToken()] = player;
        dog_to_player_[dog->GetId()] = player;
        return player;
    }

    std::shared_ptr<Player> Players::FindByToken(const Token& token) const {
        if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<Player> Players::FindByDogId(const Dog::Id& id) const {
        if (auto it = dog_to_player_.find(id); it != dog_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const std::vector<std::shared_ptr<Player>>& Players::GetPlayers() const {
        return players_;
    }

}  // namespace model