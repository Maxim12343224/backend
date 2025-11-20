#include "urlencode.h"
#include <sstream>
#include <iomanip>
#include <cctype>

std::string UrlEncode(std::string_view str) {
    std::ostringstream encoded;
    
    for (unsigned char c : str) {
        // Пробел кодируется как +
        if (c == ' ') {
            encoded << '+';
        }
        // Безопасные символы: буквы, цифры, -._~
        else if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        }
        // Все остальные символы кодируются как %XX
        else {
            encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
                   << static_cast<int>(c);
        }
    }
    
    return encoded.str();
}