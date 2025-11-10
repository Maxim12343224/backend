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
std::vector<std::string> VectorToStrings(const std::vector<T>& vector) {
    std::vector<std::string> result;
    int i = 1;
    for (auto& value : vector) {
        std::ostringstream oss;
        oss << i++ << " " << value;
        result.push_back(oss.str());
    }
    return result;
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

std::vector<std::string> View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            return {"Failed to add author"};
        } else {
            use_cases_.AddAuthor(std::move(name));
            return {};
        }
    } catch (const std::exception&) {
        return {"Failed to add author"};
    }
}

std::vector<std::string> View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            use_cases_.AddBookWithAuthorAndTags(params->author_name, params->title, 
                                               params->publication_year, params->tags);
            return {};
        } else {
            return {"Failed to add book"};
        }
    } catch (const std::exception& e) {
        return {"Failed to add book"};
    }
}

std::vector<std::string> View::ShowAuthors() const {
    return VectorToStrings(GetAuthors());
}

std::vector<std::string> View::ShowBooks() const {
    auto books = use_cases_.GetBooksExtended();
    std::vector<std::string> result;
    int i = 1;
    for (const auto& book : books) {
        std::ostringstream oss;
        oss << i++ << " " << book.title << " by " << book.author_name 
            << ", " << book.publication_year;
        result.push_back(oss.str());
    }
    return result;
}

std::vector<std::string> View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            return VectorToStrings(GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        return {"Failed to Show Books"};
    }
    return {};
}

std::vector<std::string> View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        if (name.empty()) {
            auto author_id = SelectAuthor();
            if (author_id) {
                use_cases_.DeleteAuthor(*author_id);
            }
        } else {
            auto author = use_cases_.GetAuthorByName(name);
            if (author) {
                use_cases_.DeleteAuthor(author->id);
            } else {
                return {"Failed to delete author"};
            }
        }
    } catch (const std::exception&) {
        return {"Failed to delete author"};
    }
    return {};
}

std::vector<std::string> View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        std::string author_id;
        if (name.empty()) {
            auto selected = SelectAuthor();
            if (!selected) return {};
            author_id = *selected;
        } else {
            auto author = use_cases_.GetAuthorByName(name);
            if (!author) {
                return {"Failed to edit author"};
            }
            author_id = author->id;
        }
        
        output_ << "Enter new name: ";
        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);
        
        if (new_name.empty()) {
            return {"Name cannot be empty"};
        }
        
        use_cases_.EditAuthor(author_id, new_name);
        return {};
        
    } catch (const std::exception&) {
        return {"Failed to edit author"};
    }
}

std::vector<std::string> View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        if (title.empty()) {
            // Показываем все книги для выбора
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                return {};
            }
            
            output_ << "Select book:" << std::endl;
            auto book_list = VectorToStrings(books);
            for (const auto& line : book_list) {
                output_ << line << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return {};
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return {};
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    return BookInfoExtendedToResult(books[idx]);
                }
            } catch (...) {
                return {};
            }
            
        } else {
            // Ищем книги по названию
            auto books = use_cases_.GetBooksByTitle(title);
            if (books.empty()) {
                return {};
            } else if (books.size() == 1) {
                return BookInfoExtendedToResult(books[0]);
            } else {
                // Найдено несколько книг - проверяем предварительный выбор
                std::string pre_selected_choice;
                if (input_.peek() != EOF) {
                    std::getline(input_, pre_selected_choice);
                    boost::algorithm::trim(pre_selected_choice);
                }
                
                if (!pre_selected_choice.empty()) {
                    // Пытаемся найти книгу по автору
                    for (const auto& book : books) {
                        if (book.author_name == pre_selected_choice) {
                            return BookInfoExtendedToResult(book);
                        }
                    }
                    // Пытаемся найти по номеру
                    try {
                        int idx = std::stoi(pre_selected_choice) - 1;
                        if (idx >= 0 && idx < static_cast<int>(books.size())) {
                            return BookInfoExtendedToResult(books[idx]);
                        }
                    } catch (...) {
                        // Не число - показываем список
                    }
                }
                
                // Показываем список для выбора
                output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                std::vector<std::string> book_list;
                int i = 1;
                for (const auto& book : books) {
                    std::ostringstream oss;
                    oss << i++ << " " << book.title << " by " << book.author_name 
                        << ", " << book.publication_year;
                    book_list.push_back(oss.str());
                    output_ << book_list.back() << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel: ";
                
                std::string choice;
                if (!std::getline(input_, choice)) {
                    return {};
                }
                boost::algorithm::trim(choice);
                
                if (choice.empty()) {
                    return {};
                }
                
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(books.size())) {
                        return BookInfoExtendedToResult(books[idx]);
                    }
                } catch (...) {
                    return {};
                }
            }
        }
    } catch (const std::exception& e) {
        return {};
    }
    return {};
}

std::vector<std::string> View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::string book_id;
        
        if (title.empty()) {
            // Удаление без указания названия - показываем список
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                return {};
            }
            
            output_ << "Select book:" << std::endl;
            auto book_list = VectorToStrings(books);
            for (const auto& line : book_list) {
                output_ << line << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return {};
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return {};
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    book_id = books[idx].id;
                }
            } catch (...) {
                return {};
            }
            
        } else {
            // Удаление по названию
            auto books = use_cases_.GetBooksByTitle(title);
            if (books.empty()) {
                return {};
            } else if (books.size() == 1) {
                book_id = books[0].id;
            } else {
                // Проверяем предварительный выбор
                std::string pre_selected_choice;
                if (input_.peek() != EOF) {
                    std::getline(input_, pre_selected_choice);
                    boost::algorithm::trim(pre_selected_choice);
                }
                
                if (!pre_selected_choice.empty()) {
                    // Пытаемся найти по автору
                    for (const auto& book : books) {
                        if (book.author_name == pre_selected_choice) {
                            book_id = book.id;
                            break;
                        }
                    }
                    // Пытаемся найти по номеру
                    if (book_id.empty()) {
                        try {
                            int idx = std::stoi(pre_selected_choice) - 1;
                            if (idx >= 0 && idx < static_cast<int>(books.size())) {
                                book_id = books[idx].id;
                            }
                        } catch (...) {
                            // Не число - показываем список
                        }
                    }
                }
                
                // Если не нашли по предварительному выбору, показываем список
                if (book_id.empty()) {
                    output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                    std::vector<std::string> book_list;
                    int i = 1;
                    for (const auto& book : books) {
                        std::ostringstream oss;
                        oss << i++ << " " << book.title << " by " << book.author_name 
                            << ", " << book.publication_year;
                        book_list.push_back(oss.str());
                        output_ << book_list.back() << std::endl;
                    }
                    output_ << "Enter the book # or empty line to cancel: ";
                    
                    std::string choice;
                    if (!std::getline(input_, choice)) {
                        return {};
                    }
                    boost::algorithm::trim(choice);
                    
                    if (choice.empty()) {
                        return {};
                    }
                    
                    try {
                        int idx = std::stoi(choice) - 1;
                        if (idx >= 0 && idx < static_cast<int>(books.size())) {
                            book_id = books[idx].id;
                        }
                    } catch (...) {
                        return {};
                    }
                }
            }
        }
        
        if (!book_id.empty()) {
            use_cases_.DeleteBook(book_id);
        }
        
    } catch (const std::exception&) {
        return {"Failed to delete book"};
    }
    return {};
}

std::vector<std::string> View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::string book_id;
        
        if (title.empty()) {
            // Редактирование без указания названия - показываем список
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                return {"Book not found"};
            }
            
            output_ << "Select book:" << std::endl;
            auto book_list = VectorToStrings(books);
            for (const auto& line : book_list) {
                output_ << line << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return {"Book not found"};
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return {"Book not found"};
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    book_id = books[idx].id;
                } else {
                    return {"Book not found"};
                }
            } catch (...) {
                return {"Book not found"};
            }
            
        } else {
            // Редактирование по названию
            auto books = use_cases_.GetBooksByTitle(title);
            if (books.empty()) {
                return {"Book not found"};
            } else if (books.size() == 1) {
                book_id = books[0].id;
            } else {
                // Проверяем предварительный выбор
                std::string pre_selected_choice;
                if (input_.peek() != EOF) {
                    std::getline(input_, pre_selected_choice);
                    boost::algorithm::trim(pre_selected_choice);
                }
                
                if (!pre_selected_choice.empty()) {
                    // Пытаемся найти по автору
                    for (const auto& book : books) {
                        if (book.author_name == pre_selected_choice) {
                            book_id = book.id;
                            break;
                        }
                    }
                    // Пытаемся найти по номеру
                    if (book_id.empty()) {
                        try {
                            int idx = std::stoi(pre_selected_choice) - 1;
                            if (idx >= 0 && idx < static_cast<int>(books.size())) {
                                book_id = books[idx].id;
                            }
                        } catch (...) {
                            // Не число - показываем список
                        }
                    }
                }
                
                // Если не нашли по предварительному выбору, показываем список
                if (book_id.empty()) {
                    output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                    std::vector<std::string> book_list;
                    int i = 1;
                    for (const auto& book : books) {
                        std::ostringstream oss;
                        oss << i++ << " " << book.title << " by " << book.author_name 
                            << ", " << book.publication_year;
                        book_list.push_back(oss.str());
                        output_ << book_list.back() << std::endl;
                    }
                    output_ << "Enter the book # or empty line to cancel: ";
                    
                    std::string choice;
                    if (!std::getline(input_, choice)) {
                        return {"Book not found"};
                    }
                    boost::algorithm::trim(choice);
                    
                    if (choice.empty()) {
                        return {"Book not found"};
                    }
                    
                    try {
                        int idx = std::stoi(choice) - 1;
                        if (idx >= 0 && idx < static_cast<int>(books.size())) {
                            book_id = books[idx].id;
                        } else {
                            return {"Book not found"};
                        }
                    } catch (...) {
                        return {"Book not found"};
                    }
                }
            }
        }
        
        if (book_id.empty()) {
            return {"Book not found"};
        }
        
        auto book = use_cases_.GetBookById(book_id);
        if (!book) {
            return {"Book not found"};
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
                // Оставляем старый год
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
        
        use_cases_.EditBook(book_id, new_title, new_year, tags);
        return {};
        
    } catch (const std::exception& e) {
        return {"Failed to edit book"};
    }
}

// Вспомогательные методы остаются без изменений
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
    auto author_list = VectorToStrings(authors);
    for (const auto& line : author_list) {
        output_ << line << std::endl;
    }
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

std::vector<std::string> View::BookInfoExtendedToResult(const app::BookInfoExtended& book) const {
    std::vector<std::string> result;
    result.push_back("Title: " + book.title);
    result.push_back("Author: " + book.author_name);
    result.push_back("Publication year: " + std::to_string(book.publication_year));
    
    if (!book.tags.empty()) {
        std::string tags_line = "Tags: ";
        for (size_t i = 0; i < book.tags.size(); ++i) {
            if (i > 0) tags_line += ", ";
            tags_line += book.tags[i];
        }
        result.push_back(tags_line);
    }
    
    return result;
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
    
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    
    return tags;
}

}  // namespace ui