#include "postgres.h"

#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    try {
        std::string query = "INSERT INTO authors (id, name) VALUES ('" + 
                           author.GetId().ToString() + "', '" + 
                           work.esc(author.GetName()) + "')";
        work.exec(query);
        work.commit();
    } catch (const pqxx::unique_violation&) {
        work.abort();
        throw std::runtime_error("Author already exists");
    }
}

// УБИРАЕМ GetAll для AuthorRepositoryImpl

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    std::string query = "INSERT INTO books (id, author_id, title, publication_year) VALUES ('" +
                       book.GetId().ToString() + "', '" +
                       book.GetAuthorId().ToString() + "', '" +
                       work.esc(book.GetTitle()) + "', " +
                       std::to_string(book.GetPublicationYear()) + ")";
    work.exec(query);
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() {
    pqxx::nontransaction work{connection_};
    auto result = work.exec("SELECT id, author_id, title, publication_year FROM books ORDER BY title");
    std::vector<domain::Book> books;
    
    for (int i = 0; i < result.size(); ++i) {
        std::string id = result[i][0].as<std::string>();
        std::string author_id = result[i][1].as<std::string>();
        std::string title = result[i][2].as<std::string>();
        int year = result[i][3].as<int>();
        books.emplace_back(
            domain::BookId::FromString(id),
            domain::AuthorId::FromString(author_id),
            std::move(title),
            year
        );
    }
    
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthorId(const domain::AuthorId& author_id) {
    pqxx::nontransaction work{connection_};
    std::string query = "SELECT id, title, publication_year FROM books WHERE author_id = '" +
                       author_id.ToString() + "' ORDER BY publication_year, title";
    auto result = work.exec(query);
    std::vector<domain::Book> books;
    
    for (int i = 0; i < result.size(); ++i) {
        std::string id = result[i][0].as<std::string>();
        std::string title = result[i][1].as<std::string>();
        int year = result[i][2].as<int>();
        books.emplace_back(
            domain::BookId::FromString(id),
            author_id,
            std::move(title),
            year
        );
    }
    
    return books;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    
    work.exec(
        "CREATE TABLE IF NOT EXISTS authors ("
        "id UUID CONSTRAINT author_id_constraint PRIMARY KEY,"
        "name varchar(100) UNIQUE NOT NULL"
        ")"
    );
    
    work.exec(
        "CREATE TABLE IF NOT EXISTS books ("
        "id UUID CONSTRAINT book_id_constraint PRIMARY KEY,"
        "author_id UUID NOT NULL REFERENCES authors(id),"
        "title varchar(100) NOT NULL,"
        "publication_year INTEGER NOT NULL"
        ")"
    );

    work.commit();
}

}  // namespace postgres