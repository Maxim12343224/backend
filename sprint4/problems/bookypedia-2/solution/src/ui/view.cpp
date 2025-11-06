// solution/src/ui/view.cpp
#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <cassert>
#include <iostream>
#include <sstream>

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
    menu_.AddAction("DeleteAuthor"s, "[name]"s, "Delete author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "[name]"s, "Edit author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "[title]"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "[title]"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
    menu_.AddAction("ShowBook"s, "[title]"s, "Show book details"s, std::bind(&View::ShowBook, this, ph::_1));
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
            output_ << "Author added"sv << std::endl;
        }
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            // Используем новый метод для добавления книги с автором и тегами
            use_cases_.AddBookWithAuthorAndTags(params->author_name, params->title, 
                                               params->publication_year, params->tags);
            output_ << "Book added"sv << std::endl;
        }
    } catch (const std::exception& e) {
        output_ << "Failed to add book: " << e.what() << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    // Используем расширенный метод для показа книг с авторами
    auto books = use_cases_.GetBooksExtended();
    int i = 1;
    for (const auto& book : books) {
        output_ << i++ << " " << book.title << " by " << book.author_name 
               << ", " << book.publication_year << std::endl;
    }
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

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        if (name.empty()) {
            // Select from list
            auto author_id = SelectAuthor();
            if (author_id) {
                use_cases_.DeleteAuthor(*author_id);
                output_ << "Author deleted" << std::endl;
            }
        } else {
            // Delete by name
            auto author = use_cases_.GetAuthorByName(name);
            if (author) {
                use_cases_.DeleteAuthor(author->id);
                output_ << "Author deleted" << std::endl;
            } else {
                output_ << "Failed to delete author" << std::endl;
            }
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete author" << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        std::string author_id;
        if (name.empty()) {
            // Select from list
            auto selected = SelectAuthor();
            if (!selected) return true;
            author_id = *selected;
        } else {
            // Edit by name
            auto author = use_cases_.GetAuthorByName(name);
            if (!author) {
                output_ << "Failed to edit author" << std::endl;
                return true;
            }
            author_id = author->id;
        }
        
        output_ << "Enter new name: ";
        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);
        
        if (new_name.empty()) {
            output_ << "Name cannot be empty" << std::endl;
            return true;
        }
        
        use_cases_.EditAuthor(author_id, new_name);
        output_ << "Author updated" << std::endl;
        
    } catch (const std::exception&) {
        output_ << "Failed to edit author" << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        auto book_id = SelectBook(title);
        if (book_id) {
            use_cases_.DeleteBook(*book_id);
            output_ << "Book deleted" << std::endl;
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete book" << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        auto book_id = SelectBook(title);
        if (!book_id) return true;
        
        auto book = use_cases_.GetBookById(*book_id);
        if (!book) {
            output_ << "Book not found" << std::endl;
            return true;
        }
        
        output_ << "Enter new title or empty line to use the current one (" << book->title << "): ";
        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);
        if (new_title.empty()) {
            new_title = book->title;
        }
        
        output_ << "Enter publication year or empty line to use the current one (" << book->publication_year << "): ";
        std::string year_str;
        std::getline(input_, year_str);
        boost::algorithm::trim(year_str);
        int new_year = book->publication_year;
        if (!year_str.empty()) {
            try {
                new_year = std::stoi(year_str);
            } catch (...) {
                output_ << "Invalid year" << std::endl;
                return true;
            }
        }
        
        output_ << "Enter tags (current tags: ";
        if (book->tags.empty()) {
            output_ << "none";
        } else {
            for (size_t i = 0; i < book->tags.size(); ++i) {
                if (i > 0) output_ << ", ";
                output_ << book->tags[i];
            }
        }
        output_ << "): ";
        
        std::string tags_input;
        std::getline(input_, tags_input);
        auto tags = ParseAndNormalizeTags(tags_input);
        
        use_cases_.EditBook(*book_id, new_title, new_year, tags);
        output_ << "Book updated" << std::endl;
        
    } catch (const std::exception& e) {
        output_ << "Failed to edit book: " << e.what() << std::endl;
    }
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        if (title.empty()) {
            auto book_id = SelectBook();
            if (!book_id) return true;
            auto book = use_cases_.GetBookById(*book_id);
            if (book) {
                PrintBookDetails(*book);
            }
        } else {
            auto books = use_cases_.GetBooksByTitle(title);
            if (books.empty()) {
                // Do nothing as per requirements
                return true;
            } else if (books.size() == 1) {
                PrintBookDetails(books[0]);
            } else {
                // Multiple books with same title
                output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                for (size_t i = 0; i < books.size(); ++i) {
                    output_ << (i + 1) << " " << books[i].title << " by " << books[i].author_name 
                           << ", " << books[i].publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel: ";
                std::string choice;
                std::getline(input_, choice);
                boost::algorithm::trim(choice);
                if (!choice.empty()) {
                    try {
                        int idx = std::stoi(choice) - 1;
                        if (idx >= 0 && idx < static_cast<int>(books.size())) {
                            PrintBookDetails(books[idx]);
                        }
                    } catch (...) {
                        // Invalid input, do nothing
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        output_ << "Error: " << e.what() << std::endl;
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    if (params.title.empty()) {
        return std::nullopt;
    }

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);

    if (author_name.empty()) {
        auto author_id = SelectAuthor();
        if (!author_id) {
            return std::nullopt;
        }
        // Получаем автора по ID чтобы узнать его имя
        auto authors = GetAuthors();
        for (const auto& author : authors) {
            if (author.id == *author_id) {
                params.author_name = author.name;
                break;
            }
        }
        if (params.author_name.empty()) {
            output_ << "Author not found" << std::endl;
            return std::nullopt;
        }
    } else {
        auto author = use_cases_.GetAuthorByName(author_name);
        if (!author) {
            output_ << "No author found. Do you want to add " << author_name << " (y/n)?" << std::endl;
            std::string answer;
            std::getline(input_, answer);
            boost::algorithm::trim(answer);
            if (answer == "y" || answer == "Y") {
                use_cases_.AddAuthor(author_name);
                params.author_name = author_name;
            } else {
                output_ << "Failed to add book" << std::endl;
                return std::nullopt;
            }
        } else {
            params.author_name = author->name;
        }
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    std::getline(input_, tags_input);
    params.tags = ParseAndNormalizeTags(tags_input);

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
    if (author_idx < 0 or author_idx >= static_cast<int>(authors.size())) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::optional<std::string> View::SelectBook(const std::string& title) const {
    std::vector<app::BookInfoExtended> books;
    
    if (title.empty()) {
        books = use_cases_.GetBooksExtended();
        output_ << "Select book:" << std::endl;
        int i = 1;
        for (const auto& book : books) {
            output_ << i++ << " " << book.title << " by " << book.author_name 
                   << ", " << book.publication_year << std::endl;
        }
    } else {
        books = use_cases_.GetBooksByTitle(title);
        if (books.empty()) {
            output_ << "Book not found" << std::endl;
            return std::nullopt;
        } else if (books.size() == 1) {
            return books[0].id;
        } else {
            output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
            int i = 1;
            for (const auto& book : books) {
                output_ << i++ << " " << book.title << " by " << book.author_name 
                       << ", " << book.publication_year << std::endl;
            }
        }
    }
    
    if (books.empty()) {
        output_ << "No books found" << std::endl;
        return std::nullopt;
    }
    
    output_ << "Enter the book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= static_cast<int>(books.size())) {
        throw std::runtime_error("Invalid book num");
    }

    return books[book_idx].id;
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

std::vector<std::string> View::ParseAndNormalizeTags(const std::string& tags_input) const {
    std::vector<std::string> raw_tags;
    boost::split(raw_tags, tags_input, boost::is_any_of(","), boost::token_compress_on);
    
    std::vector<std::string> tags;
    for (auto& tag : raw_tags) {
        boost::algorithm::trim(tag);
        
        // Remove extra spaces between words
        std::string normalized_tag;
        bool last_was_space = false;
        for (char c : tag) {
            if (std::isspace(c)) {
                if (!last_was_space) {
                    normalized_tag += ' ';
                    last_was_space = true;
                }
            } else {
                normalized_tag += c;
                last_was_space = false;
            }
        }
        
        boost::algorithm::trim(normalized_tag);
        
        if (!normalized_tag.empty()) {
            tags.push_back(std::move(normalized_tag));
        }
    }
    
    // Remove duplicates
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    
    return tags;
}

void View::PrintBookDetails(const app::BookInfoExtended& book) const {
    output_ << "Title: " << book.title << std::endl;
    output_ << "Author: " << book.author_name << std::endl;
    output_ << "Publication year: " << book.publication_year << std::endl;
    if (!book.tags.empty()) {
        output_ << "Tags: ";
        for (size_t i = 0; i < book.tags.size(); ++i) {
            if (i > 0) output_ << ", ";
            output_ << book.tags[i];
        }
        output_ << std::endl;
    }
}

}  // namespace ui