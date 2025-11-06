// solution/src/postgres/postgres.h
#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"

namespace postgres {

    class AuthorRepositoryImpl : public domain::AuthorRepository {
    public:
        explicit AuthorRepositoryImpl(pqxx::connection& connection)
            : connection_{ connection } {
        }

        void Save(const domain::Author& author) override;
        std::vector<domain::Author> GetAll() override;
        std::optional<domain::Author> GetById(const domain::AuthorId& id) override;
        void Delete(const domain::AuthorId& id) override;
        void Update(const domain::Author& author) override;
        std::optional<domain::Author> GetByName(const std::string& name) override;

    private:
        pqxx::connection& connection_;
    };

    class BookRepositoryImpl : public domain::BookRepository {
    public:
        explicit BookRepositoryImpl(pqxx::connection& connection)
            : connection_{ connection } {
        }

        void Save(const domain::Book& book) override;
        std::vector<domain::Book> GetAll() override;
        std::vector<domain::Book> GetByAuthorId(const domain::AuthorId& author_id) override;
        std::optional<domain::Book> GetById(const domain::BookId& id) override;
        void Delete(const domain::BookId& id) override;
        void Update(const domain::Book& book) override;
        std::vector<domain::Book> GetByTitle(const std::string& title) override;
        std::vector<std::string> GetBookTags(const domain::BookId& id) override;
        void SetBookTags(const domain::BookId& id, const std::vector<std::string>& tags) override;

    private:
        pqxx::connection& connection_;
    };

    class Database {
    public:
        explicit Database(pqxx::connection connection);

        AuthorRepositoryImpl& GetAuthors()& {
            return authors_;
        }

        BookRepositoryImpl& GetBooks()& {
            return books_;
        }

        pqxx::connection& GetConnection()& {
            return connection_;
        }

    private:
        pqxx::connection connection_;
        AuthorRepositoryImpl authors_{ connection_ };
        BookRepositoryImpl books_{ connection_ };
    };

}  // namespace postgres