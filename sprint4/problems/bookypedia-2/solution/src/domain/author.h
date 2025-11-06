// solution/src/domain/author.h
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

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

        // Новые методы для расширенного функционала
        virtual std::vector<Author> GetAll() {
            throw std::runtime_error("GetAll not implemented");
            return {};
        }
        virtual std::optional<Author> GetById(const AuthorId&) {
            throw std::runtime_error("GetById not implemented");
            return std::nullopt;
        }
        virtual std::optional<Author> GetByName(const std::string&) {
            throw std::runtime_error("GetByName not implemented");
            return std::nullopt;
        }
        virtual void Delete(const AuthorId&) {
            throw std::runtime_error("Delete not implemented");
        }
        virtual void Update(const Author&) {
            throw std::runtime_error("Update not implemented");
        }

    protected:
        ~AuthorRepository() = default;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& book) = 0;
        virtual std::vector<Book> GetAll() = 0;
        virtual std::vector<Book> GetByAuthorId(const AuthorId& author_id) = 0;

        // Новые методы для расширенного функционала
        virtual std::optional<Book> GetById(const BookId&) {
            throw std::runtime_error("GetById not implemented");
            return std::nullopt;
        }
        virtual std::vector<Book> GetByTitle(const std::string&) {
            throw std::runtime_error("GetByTitle not implemented");
            return {};
        }
        virtual void Delete(const BookId&) {
            throw std::runtime_error("Delete not implemented");
        }
        virtual void Update(const Book&) {
            throw std::runtime_error("Update not implemented");
        }
        virtual std::vector<std::string> GetBookTags(const BookId&) {
            throw std::runtime_error("GetBookTags not implemented");
            return {};
        }
        virtual void SetBookTags(const BookId&, const std::vector<std::string>&) {
            throw std::runtime_error("SetBookTags not implemented");
        }

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain