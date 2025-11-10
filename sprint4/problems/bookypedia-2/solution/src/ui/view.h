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

    }  // namespace detail

    class View {
    public:
        View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

    private:
        std::vector<std::string> AddAuthor(std::istream& cmd_input) const;
        std::vector<std::string> AddBook(std::istream& cmd_input) const;
        std::vector<std::string> ShowAuthors() const;
        std::vector<std::string> ShowBooks() const;
        std::vector<std::string> ShowAuthorBooks() const;
        std::vector<std::string> DeleteAuthor(std::istream& cmd_input) const;
        std::vector<std::string> EditAuthor(std::istream& cmd_input) const;
        std::vector<std::string> DeleteBook(std::istream& cmd_input) const;
        std::vector<std::string> EditBook(std::istream& cmd_input) const;
        std::vector<std::string> ShowBook(std::istream& cmd_input) const;

        std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
        std::optional<std::string> SelectAuthor() const;
        std::optional<std::string> SelectBook(const std::string& title = "") const;
        std::vector<detail::AuthorInfo> GetAuthors() const;
        std::vector<detail::BookInfo> GetBooks() const;
        std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;
        std::vector<std::string> ParseAndNormalizeTags(const std::string& tags_input) const;
        std::vector<std::string> BookInfoExtendedToResult(const app::BookInfoExtended& book) const;

        menu::Menu& menu_;
        app::UseCases& use_cases_;
        std::istream& input_;
        std::ostream& output_;
    };

}  // namespace ui