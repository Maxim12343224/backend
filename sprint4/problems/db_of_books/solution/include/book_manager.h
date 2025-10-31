#pragma once

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Book {
    int id;
    std::string title;
    std::string author;
    int year;
    std::string isbn;
};

class BookManager {
public:
    BookManager(const std::string& connection_string);

    bool initialize_database();
    bool add_book(const std::string& title, const std::string& author, int year, const std::string& isbn);
    std::vector<Book> get_all_books();

private:
    std::string connection_string_;

    void create_table_if_not_exists(pqxx::work& txn);
    void prepare_statements(pqxx::connection& conn);
};