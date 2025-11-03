#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app {

    class UseCasesImpl : public UseCases {
    public:
        explicit UseCasesImpl(domain::AuthorRepository& authors)
            : authors_{ authors } {
        }

        UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books, domain::AuthorQueries& author_queries)
            : authors_{ authors }, books_{ &books }, author_queries_{ &author_queries } {
        }

        void AddAuthor(const std::string& name) override;
        std::vector<AuthorInfo> GetAuthors() override;
        void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
        std::vector<BookInfo> GetBooks() override;
        std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) override;

        static void SetBookRepository(domain::BookRepository* book_repo);
        static void SetAuthorQueries(domain::AuthorQueries* author_queries);

    private:
        domain::AuthorRepository& authors_;
        static domain::BookRepository* books_;
        static domain::AuthorQueries* author_queries_;
    };

}  // namespace app