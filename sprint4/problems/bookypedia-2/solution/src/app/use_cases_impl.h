// solution/src/app/use_cases_impl.h
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

        // Существующие методы
        void AddAuthor(const std::string& name) override;
        std::vector<AuthorInfo> GetAuthors() override;
        void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
        std::vector<BookInfo> GetBooks() override;
        std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) override;

        // Новые методы
        void AddBookWithAuthorAndTags(const std::string& author_name, const std::string& title,
            int publication_year, const std::vector<std::string>& tags) override;
        void DeleteAuthor(const std::string& author_id) override;
        void EditAuthor(const std::string& author_id, const std::string& new_name) override;
        void DeleteBook(const std::string& book_id) override;
        void EditBook(const std::string& book_id, const std::string& new_title,
            int new_publication_year, const std::vector<std::string>& tags) override;
        std::vector<BookInfoExtended> GetBooksExtended() override;
        std::vector<BookInfoExtended> GetBooksByTitle(const std::string& title) override;
        std::optional<BookInfoExtended> GetBookById(const std::string& book_id) override;
        std::optional<AuthorInfo> GetAuthorByName(const std::string& name) override;

        static void SetBookRepository(domain::BookRepository* book_repo);
        static void SetConnection(pqxx::connection* connection);

    private:
        domain::AuthorRepository& authors_;
        static domain::BookRepository* books_;
        static pqxx::connection* connection_;
    };

}  // namespace app