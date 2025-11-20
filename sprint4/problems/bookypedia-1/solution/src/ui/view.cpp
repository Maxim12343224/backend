#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

using AuthorInfo = app::AuthorInfo;
using BookInfo = app::BookInfo;

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            output_ << "Failed to add author"sv << std::endl;
        } else {
            use_cases_.AddAuthor(std::move(name));
        }
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        std::cout << "DEBUG View::AddBook: started" << std::endl;
        if (auto params = GetBookParams(cmd_input)) {
            std::cout << "DEBUG View::AddBook: params - title='" << params->title 
                      << "', author='" << params->author_name 
                      << "', year=" << params->publication_year 
                      << ", tags_count=" << params->tags.size() << std::endl;
            use_cases_.AddBookWithAuthorAndTags(params->author_name, params->title, 
                                               params->publication_year, params->tags);
        } else {
            std::cout << "DEBUG View::AddBook: GetBookParams returned nullopt" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "DEBUG View::AddBook: exception - " << e.what() << std::endl;
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    std::cout << "DEBUG GetBookParams: reading year and title" << std::endl;
    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);
    
    std::cout << "DEBUG GetBookParams: year=" << params.publication_year 
              << ", title='" << params.title << "'" << std::endl;

    if (params.title.empty()) {
        std::cout << "DEBUG GetBookParams: empty title" << std::endl;
        return std::nullopt;
    }

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);
    std::cout << "DEBUG GetBookParams: author input='" << author_name << "'" << std::endl;

    // ... остальной код с отладочными выводами
    std::cout << "DEBUG GetBookParams: final author='" << params.author_name << "'" << std::endl;

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    std::getline(input_, tags_input);
    params.tags = ParseAndNormalizeTags(tags_input);
    std::cout << "DEBUG GetBookParams: tags_input='" << tags_input 
              << "', normalized_tags_count=" << params.tags.size() << std::endl;

    return params;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    return use_cases_.GetAuthors();
}

std::vector<detail::BookInfo> View::GetBooks() const {
    return use_cases_.GetBooks();
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    return use_cases_.GetAuthorBooks(author_id);
}

}  // namespace ui