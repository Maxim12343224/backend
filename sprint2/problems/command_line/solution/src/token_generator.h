#pragma once
#include "player.h"
#include <random>
#include <string>

namespace model {

    class TokenGenerator {
    public:
        TokenGenerator() = default;
        ~TokenGenerator() = default;

        // «апрещаем копирование и перемещение
        TokenGenerator(const TokenGenerator&) = delete;
        TokenGenerator& operator=(const TokenGenerator&) = delete;
        TokenGenerator(TokenGenerator&&) = delete;
        TokenGenerator& operator=(TokenGenerator&&) = delete;

        Token GenerateToken();

    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{ random_device_() };
        std::mt19937_64 generator2_{ random_device_() };
    };

} // namespace model