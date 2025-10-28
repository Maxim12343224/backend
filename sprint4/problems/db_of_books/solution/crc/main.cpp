#include "book_manager.h"
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <connection_string>" << std::endl;
        return 1;
    }
    
    std::string connection_string = argv[1];
    
    try {
        BookManager manager(connection_string);
        
        // Initialize database
        if (!manager.initialize_database()) {
            return 1;
        }
        
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            try {
                auto request = json::parse(line);
                std::string action = request["action"];
                
                if (action == "add_book") {
                    auto payload = request["payload"];
                    std::string title = payload["title"];
                    std::string author = payload["author"];
                    int year = payload["year"];
                    
                    std::string isbn;
                    if (payload["ISBN"].is_null()) {
                        isbn = "";
                    } else {
                        isbn = payload["ISBN"];
                    }
                    
                    bool success = manager.add_book(title, author, year, isbn);
                    
                    json response = {{"result", success}};
                    std::cout << response.dump() << std::endl;
                    
                } else if (action == "all_books") {
                    auto books = manager.get_all_books();
                    
                    json books_array = json::array();
                    for (const auto& book : books) {
                        json book_json = {
                            {"id", book.id},
                            {"title", book.title},
                            {"author", book.author},
                            {"year", book.year}
                        };
                        
                        if (book.isbn.empty()) {
                            book_json["ISBN"] = nullptr;
                        } else {
                            book_json["ISBN"] = book.isbn;
                        }
                        
                        books_array.push_back(book_json);
                    }
                    
                    std::cout << books_array.dump() << std::endl;
                    
                } else if (action == "exit") {
                    break;
                }
                
            } catch (const json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error processing request: " << e.what() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}