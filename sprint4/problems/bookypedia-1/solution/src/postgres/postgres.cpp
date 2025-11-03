#include "postgres.h"

#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    try {
        work.exec_params(
            R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
)"_zv,
            author.GetId().ToString(), author.GetName());
        work.commit();
    } catch (const pqxx::unique_violation&) {
        work.abort();
        throw std::runtime_error("Author already exists");
    }
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() {
    pqxx::read_transaction r{connection_};
    auto query = "SELECT id, name FROM authors ORDER BY name"_zv;
    std::vector<domain::Author> authors;
    
    for (auto [id, name] : r.query<std::string, std::string>(query)) {
        authors.emplace_back(domain::AuthorId::FromString(id), std::move(name));
    }
    
    return authors;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) 
VALUES ($1, $2, $3, $4)
)"_zv,
        book.GetId().ToString(), 
        book.GetAuthorId().ToString(), 
        book.GetTitle(), 
        book.GetPublicationYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() {
    pqxx::read_transaction r{connection_};
    auto query = "SELECT id, author_id, title, publication_year FROM books ORDER BY title"_zv;
    std::vector<domain::Book> books;
    
    for (auto [id, author_id, title, year] : r.query<std::string, std::string, std::string, int>(query)) {
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
    pqxx::read_transaction r{connection_};
    auto query = "SELECT id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title"_zv;
    std::vector<domain::Book> books;
    
    for (auto [id, title, year] : r.query<std::string, std::string, int>(query, author_id.ToString())) {
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
    
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title varchar(100) NOT NULL,
    publication_year INTEGER NOT NULL
);
)"_zv);

    work.commit();
}

}  // namespace postgres