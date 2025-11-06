// solution/src/app/use_cases.h
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <optional>

namespace app {

    struct AuthorInfo {
        std::string id;
        std::string name;
    };

    struct BookInfo {
        std::string id;
        std::string title;
        std::string author_name;
        int publication_year;
        std::vector<std::string> tags;
    };

    struct BookInfoWithAuthorId {
        std::string id;
        std::string title;
        std::string author_id;
        int publication_year;
    };

    inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
        out << author.name;
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
        out << book.title << " by " << book.author_name << ", " << book.publication_year;
        return out;
    }

    class UseCases {
    public:
        virtual void AddAuthor(const std::string& name) = 0;
        virtual std::vector<AuthorInfo> GetAuthors() = 0;
        virtual void AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::vector<std::string>& tags = {}) = 0;
        virtual std::vector<BookInfo> GetBooks() = 0;
        virtual std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) = 0;

        // New methods
        virtual void DeleteAuthor(const std::string& author_id) = 0;
        virtual void EditAuthor(const std::string& author_id, const std::string& new_name) = 0;
        virtual void DeleteBook(const std::string& book_id) = 0;
        virtual void EditBook(const std::string& book_id, const std::string& new_title, int new_publication_year, const std::vector<std::string>& tags) = 0;
        virtual std::optional<AuthorInfo> GetAuthorById(const std::string& author_id) = 0;
        virtual std::optional<AuthorInfo> GetAuthorByName(const std::string& name) = 0;
        virtual std::vector<BookInfo> GetBooksByTitle(const std::string& title) = 0;
        virtual std::optional<BookInfo> GetBookById(const std::string& book_id) = 0;
        virtual std::vector<std::string> GetBookTags(const std::string& book_id) = 0;

    protected:
        ~UseCases() = default;
    };

}  // namespace app