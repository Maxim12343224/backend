#pragma once
#include "model.h"
#include "dog.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace model {

    namespace detail {
        struct TokenTag {};
    }  // namespace detail

    using Token = util::Tagged<std::string, detail::TokenTag>;

    class Player {
    public:
        Player(uint32_t id, std::string name, std::shared_ptr<Dog> dog)
            : id_{ id }, name_{ std::move(name) }, dog_{ std::move(dog) }, token_{ Token{""} } {}

        uint32_t GetId() const { return id_; }
        const std::string& GetName() const { return name_; }
        const Dog& GetDog() const { return *dog_; }
        Dog& GetDog() { return *dog_; }
        const Token& GetToken() const { return token_; }
        void SetToken(Token token) { token_ = std::move(token); }
        const std::string& GetUnderlying() const { return *token_; }

    private:
        uint32_t id_;
        std::string name_;
        std::shared_ptr<Dog> dog_;
        Token token_;
    };

    class Players {
    public:
        std::shared_ptr<Player> Add(std::string name, std::shared_ptr<Dog> dog);
        std::shared_ptr<Player> FindByToken(const Token& token) const;
        std::shared_ptr<Player> FindByDogId(const Dog::Id& id) const;
        const std::vector<std::shared_ptr<Player>>& GetPlayers() const;

    private:
        std::vector<std::shared_ptr<Player>> players_;
        std::unordered_map<Token, std::shared_ptr<Player>> token_to_player_;
        std::unordered_map<Dog::Id, std::shared_ptr<Player>> dog_to_player_;
    };

}  // namespace model