#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace app {

    struct AuthorInfo {
        std::string id;
        std::string name;
    };

    struct BookInfo {
        std::string title;
        int publication_year;
    };

    
    inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
        out << author.name;
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
        out << book.title << ", " << book.publication_year;
        return out;
    }

    class UseCases {
    public:
        virtual void AddAuthor(const std::string& name) = 0;
        virtual std::vector<AuthorInfo> GetAuthors() = 0;
        virtual void AddBook(const std::string& author_id, const std::string& title, int publication_year) = 0;
        virtual std::vector<BookInfo> GetBooks() = 0;
        virtual std::vector<BookInfo> GetAuthorBooks(const std::string& author_id) = 0;

    protected:
        ~UseCases() = default;
    };

}