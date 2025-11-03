#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"
#include <pqxx/connection>

namespace app {

    class UseCasesImpl : public UseCases {
    public:
        explicit UseCasesImpl(domain::AuthorRepository& authors)
            : authors_{ authors } {
        }

        void AddAuthor(const std::string& name) override;
        std::vector<AuthorInfo> GetAuthors() override;
        void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
        std::vector<BookInfo> GetBooks() override;
        std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) override;

        static void SetBookRepository(domain::BookRepository* book_repo);
        static void SetConnection(pqxx::connection* connection);

    private:
        domain::AuthorRepository& authors_;
        static domain::BookRepository* books_;
        static pqxx::connection* connection_;
    };

}  // namespace app