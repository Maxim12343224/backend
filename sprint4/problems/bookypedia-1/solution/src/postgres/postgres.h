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

    private:
        pqxx::connection& connection_;
    };

    class AuthorQueriesImpl : public domain::AuthorQueries {
    public:
        explicit AuthorQueriesImpl(pqxx::connection& connection)
            : connection_{ connection } {
        }

        std::vector<domain::Author> GetAllAuthors() override;

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

        AuthorQueriesImpl& GetAuthorQueries()& {
            return author_queries_;
        }

    private:
        pqxx::connection connection_;
        AuthorRepositoryImpl authors_{ connection_ };
        BookRepositoryImpl books_{ connection_ };
        AuthorQueriesImpl author_queries_{ connection_ };
    };

}  // namespace postgres