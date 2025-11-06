// solution/src/app/use_cases_impl.h
#pragma once
#include "../domain/author.h"
#include "use_cases.h"
#include <pqxx/connection>

namespace app {

    class UseCasesImpl : public UseCases {
    public:
        explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books, pqxx::connection& connection)
            : authors_(authors), books_(books), connection_(connection) {
        }

        void AddAuthor(const std::string& name) override;
        std::vector<AuthorInfo> GetAuthors() override;
        void AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::vector<std::string>& tags = {}) override;
        std::vector<BookInfo> GetBooks() override;
        std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) override;

        void DeleteAuthor(const std::string& author_id) override;
        void EditAuthor(const std::string& author_id, const std::string& new_name) override;
        void DeleteBook(const std::string& book_id) override;
        void EditBook(const std::string& book_id, const std::string& new_title, int new_publication_year, const std::vector<std::string>& tags) override;
        std::optional<AuthorInfo> GetAuthorById(const std::string& author_id) override;
        std::optional<AuthorInfo> GetAuthorByName(const std::string& name) override;
        std::vector<BookInfo> GetBooksByTitle(const std::string& title) override;
        std::optional<BookInfo> GetBookById(const std::string& book_id) override;
        std::vector<std::string> GetBookTags(const std::string& book_id) override;

    private:
        domain::AuthorRepository& authors_;
        domain::BookRepository& books_;
        pqxx::connection& connection_;
    };

}  // namespace app