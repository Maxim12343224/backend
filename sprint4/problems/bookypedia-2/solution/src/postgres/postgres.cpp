// solution/src/postgres/postgres.cpp
#include "postgres.h"

#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec("INSERT INTO authors (id, name) VALUES (" +
              work.quote(author.GetId().ToString()) + ", " +
              work.quote(author.GetName()) + ")");
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT id, name FROM authors ORDER BY name");
    std::vector<domain::Author> authors;
    for (const auto& row : result) {
        authors.emplace_back(
            domain::AuthorId::FromString(row[0].as<std::string>()),
            row[1].as<std::string>()
        );
    }
    return authors;
}

std::optional<domain::Author> AuthorRepositoryImpl::GetById(const domain::AuthorId& id) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT name FROM authors WHERE id = " + work.quote(id.ToString()));
    if (result.empty()) {
        return std::nullopt;
    }
    return domain::Author{id, result[0][0].as<std::string>()};
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    pqxx::work work{connection_};
    work.exec("DELETE FROM authors WHERE id = " + work.quote(id.ToString()));
    work.commit();
}

void AuthorRepositoryImpl::Update(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec("UPDATE authors SET name = " + work.quote(author.GetName()) +
              " WHERE id = " + work.quote(author.GetId().ToString()));
    work.commit();
}

std::optional<domain::Author> AuthorRepositoryImpl::GetByName(const std::string& name) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT id FROM authors WHERE name = " + work.quote(name));
    if (result.empty()) {
        return std::nullopt;
    }
    return domain::Author{
        domain::AuthorId::FromString(result[0][0].as<std::string>()),
        name
    };
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec("INSERT INTO books (id, author_id, title, publication_year) VALUES (" +
              work.quote(book.GetId().ToString()) + ", " +
              work.quote(book.GetAuthorId().ToString()) + ", " +
              work.quote(book.GetTitle()) + ", " +
              work.quote(book.GetPublicationYear()) + ")");
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT id, author_id, title, publication_year FROM books");
    std::vector<domain::Book> books;
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row[0].as<std::string>()),
            domain::AuthorId::FromString(row[1].as<std::string>()),
            row[2].as<std::string>(),
            row[3].as<int>()
        );
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthorId(const domain::AuthorId& author_id) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT id, title, publication_year FROM books WHERE author_id = " +
                           work.quote(author_id.ToString()) + " ORDER BY publication_year, title");
    std::vector<domain::Book> books;
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row[0].as<std::string>()),
            author_id,
            row[1].as<std::string>(),
            row[2].as<int>()
        );
    }
    return books;
}

std::optional<domain::Book> BookRepositoryImpl::GetById(const domain::BookId& id) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT author_id, title, publication_year FROM books WHERE id = " +
                           work.quote(id.ToString()));
    if (result.empty()) {
        return std::nullopt;
    }
    return domain::Book{
        id,
        domain::AuthorId::FromString(result[0][0].as<std::string>()),
        result[0][1].as<std::string>(),
        result[0][2].as<int>()
    };
}

void BookRepositoryImpl::Delete(const domain::BookId& id) {
    pqxx::work work{connection_};
    
    
    work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(id.ToString()));
    work.exec("DELETE FROM books WHERE id = " + work.quote(id.ToString()));
    
    work.commit();
}

void BookRepositoryImpl::Update(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec("UPDATE books SET title = " + work.quote(book.GetTitle()) +
              ", publication_year = " + work.quote(book.GetPublicationYear()) +
              " WHERE id = " + work.quote(book.GetId().ToString()));
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetByTitle(const std::string& title) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT id, author_id, publication_year FROM books WHERE title = " +
                           work.quote(title) + " ORDER BY publication_year");
    std::vector<domain::Book> books;
    for (const auto& row : result) {
        books.emplace_back(
            domain::BookId::FromString(row[0].as<std::string>()),
            domain::AuthorId::FromString(row[1].as<std::string>()),
            title,
            row[2].as<int>()
        );
    }
    return books;
}

std::vector<std::string> BookRepositoryImpl::GetBookTags(const domain::BookId& id) {
    pqxx::read_transaction work{connection_};
    auto result = work.exec("SELECT tag FROM book_tags WHERE book_id = " +
                           work.quote(id.ToString()) + " ORDER BY tag");
    std::vector<std::string> tags;
    for (const auto& row : result) {
        tags.push_back(row[0].as<std::string>());
    }
    return tags;
}

void BookRepositoryImpl::SetBookTags(const domain::BookId& id, const std::vector<std::string>& tags) {
    pqxx::work work{connection_};
    
    // Delete existing tags
    work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(id.ToString()));
    
    // Insert new tags
    for (const auto& tag : tags) {
        work.exec("INSERT INTO book_tags (book_id, tag) VALUES (" +
                  work.quote(id.ToString()) + ", " + work.quote(tag) + ")");
    }
    
    work.commit();
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
        "author_id UUID NOT NULL REFERENCES authors(id) ON DELETE CASCADE,"
        "title varchar(100) NOT NULL,"
        "publication_year INTEGER NOT NULL"
        ")"
    );

    work.exec(
        "CREATE TABLE IF NOT EXISTS book_tags ("
        "book_id UUID NOT NULL REFERENCES books(id) ON DELETE CASCADE,"
        "tag varchar(30) NOT NULL,"
        "PRIMARY KEY (book_id, tag)"
        ")"
    );

    work.commit();
}

}  // namespace postgres