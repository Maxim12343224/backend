#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <cctype>

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        
        if (c == '+') {
            // Пробел кодируется как '+'
            result += ' ';
        } else if (c == '%') {
            // Обработка %-последовательностей
            if (i + 2 >= str.size()) {
                throw std::invalid_argument("Incomplete %-sequence");
            }
            
            // Получаем два следующих символа
            char hex1 = str[i + 1];
            char hex2 = str[i + 2];
            
            // Проверяем, что это валидные hex-символы
            if (!std::isxdigit(hex1) || !std::isxdigit(hex2)) {
                throw std::invalid_argument("Invalid %-sequence");
            }
            
            // Конвертируем hex в число
            int char_code;
            std::string hex_str = {hex1, hex2};
            auto [ptr, ec] = std::from_chars(hex_str.data(), hex_str.data() + 2, char_code, 16);
            
            if (ec != std::errc()) {
                throw std::invalid_argument("Invalid %-sequence");
            }
            
            result += static_cast<char>(char_code);
            i += 2; // Пропускаем два обработанных символа
        } else {
            // Обычные символы добавляем как есть
            result += c;
        }
    }
    
    return result;
}