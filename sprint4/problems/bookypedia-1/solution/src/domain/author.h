#pragma once
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"

namespace domain {

    namespace detail {
        struct AuthorTag {};
        struct BookTag {};
    }  // namespace detail

    using AuthorId = util::TaggedUUID<detail::AuthorTag>;
    using BookId = util::TaggedUUID<detail::BookTag>;

    class Author {
    public:
        Author(AuthorId id, std::string name)
            : id_(std::move(id))
            , name_(std::move(name)) {
        }

        const AuthorId& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

    private:
        AuthorId id_;
        std::string name_;
    };

    class Book {
    public:
        Book(BookId id, AuthorId author_id, std::string title, int publication_year)
            : id_(std::move(id))
            , author_id_(std::move(author_id))
            , title_(std::move(title))
            , publication_year_(publication_year) {
        }

        const BookId& GetId() const noexcept {
            return id_;
        }

        const AuthorId& GetAuthorId() const noexcept {
            return author_id_;
        }

        const std::string& GetTitle() const noexcept {
            return title_;
        }

        int GetPublicationYear() const noexcept {
            return publication_year_;
        }

    private:
        BookId id_;
        AuthorId author_id_;
        std::string title_;
        int publication_year_;
    };

    class AuthorRepository {
    public:
        virtual void Save(const Author& author) = 0;
        // ”Ѕ»–ј≈ћ GetAll отсюда - он не нужен дл€ тестов

    protected:
        ~AuthorRepository() = default;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& book) = 0;
        virtual std::vector<Book> GetAll() = 0;
        virtual std::vector<Book> GetByAuthorId(const AuthorId& author_id) = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain