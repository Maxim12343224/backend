#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace menu {
    class Menu;
}

namespace app {
    class UseCases;
    struct AuthorInfo;
    struct BookInfo;
    struct BookInfoExtended;
}

namespace ui {
    namespace detail {

        struct AddBookParams {
            std::string title;
            std::string author_name;
            int publication_year = 0;
            std::vector<std::string> tags;
        };

        using AuthorInfo = app::AuthorInfo;
        using BookInfo = app::BookInfo;

    }

    class View {
    public:
        View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

    private:
        bool AddAuthor(std::istream& cmd_input) const;
        bool AddBook(std::istream& cmd_input) const;
        bool ShowAuthors() const;
        bool ShowBooks() const;
        bool ShowAuthorBooks() const;
        bool DeleteAuthor(std::istream& cmd_input) const;
        bool EditAuthor(std::istream& cmd_input) const;
        bool DeleteBook(std::istream& cmd_input) const;
        bool EditBook(std::istream& cmd_input) const;
        bool ShowBook(std::istream& cmd_input) const;

        std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
        std::optional<std::string> SelectAuthor() const;
        std::vector<detail::AuthorInfo> GetAuthors() const;
        std::vector<detail::BookInfo> GetBooks() const;
        std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
        std::vector<std::string> ParseAndNormalizeTags(const std::string& tags_input) const;
        void PrintBookDetails(const app::BookInfoExtended& book) const;
        void PrintAuthors(const std::vector<detail::AuthorInfo>& authors) const;
        void PrintBooks(const std::vector<detail::BookInfo>& books) const;

        menu::Menu& menu_;
        app::UseCases& use_cases_;
        std::istream& input_;
        std::ostream& output_;
    };

}