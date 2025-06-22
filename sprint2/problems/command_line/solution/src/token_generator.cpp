#include "token_generator.h"
#include <iomanip>
#include <sstream>

namespace model {

    Token TokenGenerator::GenerateToken() {
        std::stringstream ss;
        ss << std::hex << generator1_() << generator2_();
        return Token{ ss.str() };
    }

}  // namespace model