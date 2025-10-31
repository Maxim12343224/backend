#include "book_manager.h"
#include <iostream>

using namespace std::literals;
using pqxx::operator"" _zv;

BookManager::BookManager(const std::string& connection_string) 
    : connection_string_(connection_string) {
}

bool BookManager::initialize_database() {
    try {
        pqxx::connection conn(connection_string_);
        pqxx::work txn(conn);
        
        create_table_if_not_exists(txn);
        txn.commit();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Database initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void BookManager::create_table_if_not_exists(pqxx::work& txn) {
    txn.exec(
        "CREATE TABLE IF NOT EXISTS books ("
        "id SERIAL PRIMARY KEY, "
        "title VARCHAR(100) NOT NULL, "
        "author VARCHAR(100) NOT NULL, "
        "year INTEGER NOT NULL, "
        "ISBN CHAR(13) UNIQUE"
        ")"_zv
    );
}

bool BookManager::add_book(const std::string& title, const std::string& author, 
                          int year, const std::string& isbn) {
    try {
        pqxx::connection conn(connection_string_);
        pqxx::work txn(conn);
        
        if (isbn.empty() || isbn == "null") {
            // Используем exec с экранированием вместо подготовленных запросов
            txn.exec("INSERT INTO books (title, author, year, isbn) VALUES (" + 
                     txn.quote(title) + ", " + txn.quote(author) + ", " + 
                     std::to_string(year) + ", NULL)");
        } else {
            txn.exec("INSERT INTO books (title, author, year, isbn) VALUES (" + 
                     txn.quote(title) + ", " + txn.quote(author) + ", " + 
                     std::to_string(year) + ", " + txn.quote(isbn) + ")");
        }
        
        txn.commit();
        return true;
        
    } catch (const pqxx::unique_violation& e) {
        // ISBN duplicate violation
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error adding book: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Book> BookManager::get_all_books() {
    std::vector<Book> books;
    
    try {
        pqxx::connection conn(connection_string_);
        pqxx::read_transaction txn(conn);
        
        auto result = txn.exec(
            "SELECT id, title, author, year, isbn FROM books "
            "ORDER BY year DESC, title ASC, author ASC, isbn ASC");
        
        for (const auto& row : result) {
            Book book;
            book.id = row[0].as<int>();
            book.title = row[1].as<std::string>();
            book.author = row[2].as<std::string>();
            book.year = row[3].as<int>();
            
            // Обрабатываем возможный NULL в ISBN
            if (!row[4].is_null()) {
                book.isbn = row[4].as<std::string>();
            }
            
            books.push_back(book);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error retrieving books: " << e.what() << std::endl;
    }
    
    return books;
}