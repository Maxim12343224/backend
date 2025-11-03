#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app {

    class UseCasesImpl : public UseCases {
    public:
        // Конструктор для тестов (только authors)
        explicit UseCasesImpl(domain::AuthorRepository& authors)
            : authors_{ authors } {
        }

        // Конструктор для реального использования (authors + books)
        UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
            : authors_{ authors }, books_{ &books } {
        }

        void AddAuthor(const std::string& name) override;
        std::vector<AuthorInfo> GetAuthors() override;
        void AddBook(const std::string& author_id, const std::string& title, int publication_year) override;
        std::vector<BookInfo> GetBooks() override;
        std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) override;

        // Статический метод для установки book repository
        static void SetBookRepository(domain::BookRepository* book_repo);

    private:
        domain::AuthorRepository& authors_;
        static domain::BookRepository* books_; // Статический указатель
    };

}  // namespace app