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
        }
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        std::cerr << "DEBUG: Starting AddBook command" << std::endl;
        
        if (auto params = GetBookParams(cmd_input)) {
            std::cerr << "DEBUG: Calling AddBookWithAuthorAndTags with author: " << params->author_name 
                      << ", title: " << params->title << std::endl;
            use_cases_.AddBookWithAuthorAndTags(params->author_name, params->title, 
                                               params->publication_year, params->tags);
            std::cerr << "DEBUG: Book added successfully" << std::endl;
        } else {
            std::cerr << "DEBUG: Failed to get book parameters - book not added" << std::endl;
        }
    } catch (const std::exception& e) {
        output_ << "Failed to add book"sv << std::endl;
        std::cerr << "DEBUG: Exception in AddBook: " << e.what() << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintAuthors(GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    auto books = use_cases_.GetBooksExtended();
    if (books.empty()) {
        return true;
    }
    
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
            PrintBooks(GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        output_ << "Failed to Show Books"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
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
                output_ << "Failed to delete author" << std::endl;
            }
        }
    } catch (const std::exception&) {
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
            auto selected = SelectAuthor();
            if (!selected) return true;
            author_id = *selected;
        } else {
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
        
    } catch (const std::exception&) {
        output_ << "Failed to edit author" << std::endl;
    }
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::cerr << "DEBUG: Starting ShowBook command" << std::endl;
        
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::cerr << "DEBUG: ShowBook title: '" << title << "'" << std::endl;
        
        if (title.empty()) {
            std::cerr << "DEBUG: Empty title, showing book list" << std::endl;
            
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                std::cerr << "DEBUG: No books found" << std::endl;
                return true;
            }
            
            output_ << "Select book:" << std::endl;
            int i = 1;
            for (const auto& book : books) {
                output_ << i++ << " " << book.title << " by " << book.author_name 
                       << ", " << book.publication_year << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return true;
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                std::cerr << "DEBUG: User cancelled book selection" << std::endl;
                return true;
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    PrintBookDetails(books[idx]);
                }
            } catch (...) {
            }
            
        } else {
            std::cerr << "DEBUG: Looking for books with title: '" << title << "'" << std::endl;
            
            auto books = use_cases_.GetBooksByTitle(title);
            std::cerr << "DEBUG: Found " << books.size() << " books with this title" << std::endl;
            
            if (books.empty()) {
                std::cerr << "DEBUG: No books found with title '" << title << "'" << std::endl;
                return true;
            } else if (books.size() == 1) {
                std::cerr << "DEBUG: Single book found, showing details" << std::endl;
                PrintBookDetails(books[0]);
            } else {
                std::cerr << "DEBUG: Multiple books found, showing selection" << std::endl;
                
                output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                int i = 1;
                for (const auto& book : books) {
                    output_ << i++ << " " << book.title << " by " << book.author_name 
                           << ", " << book.publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel: ";
                
                std::string choice;
                if (!std::getline(input_, choice)) {
                    return true;
                }
                boost::algorithm::trim(choice);
                
                if (choice.empty()) {
                    std::cerr << "DEBUG: User cancelled book selection" << std::endl;
                    return true;
                }
                
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(books.size())) {
                        PrintBookDetails(books[idx]);
                    }
                } catch (...) {
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "DEBUG: Exception in ShowBook: " << e.what() << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::cerr << "DEBUG: Starting DeleteBook command" << std::endl;
        
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::cerr << "DEBUG: DeleteBook title: '" << title << "'" << std::endl;
        
        std::string book_id;
        
        if (title.empty()) {
            std::cerr << "DEBUG: Empty title, showing book list for deletion" << std::endl;
            
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                return true;
            }
            
            output_ << "Select book:" << std::endl;
            int i = 1;
            for (const auto& book : books) {
                output_ << i++ << " " << book.title << " by " << book.author_name 
                       << ", " << book.publication_year << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return true;
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return true;
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    book_id = books[idx].id;
                    std::cerr << "DEBUG: Deleting book with id: " << book_id << std::endl;
                }
            } catch (...) {
                return true;
            }
            
        } else {
            std::cerr << "DEBUG: Looking for books to delete with title: '" << title << "'" << std::endl;
            
            auto books = use_cases_.GetBooksByTitle(title);
            std::cerr << "DEBUG: Found " << books.size() << " books with this title" << std::endl;
            
            if (books.empty()) {
                return true;
            } else if (books.size() == 1) {
                book_id = books[0].id;
                std::cerr << "DEBUG: Single book found, deleting id: " << book_id << std::endl;
            } else {
                std::cerr << "DEBUG: Multiple books found, showing selection for deletion" << std::endl;
                
                output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                int i = 1;
                for (const auto& book : books) {
                    output_ << i++ << " " << book.title << " by " << book.author_name 
                           << ", " << book.publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel: ";
                
                std::string choice;
                if (!std::getline(input_, choice)) {
                    return true;
                }
                boost::algorithm::trim(choice);
                
                if (choice.empty()) {
                    return true;
                }
                
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(books.size())) {
                        book_id = books[idx].id;
                        std::cerr << "DEBUG: Deleting book with id: " << book_id << std::endl;
                    }
                } catch (...) {
                    return true;
                }
            }
        }
        
        if (!book_id.empty()) {
            use_cases_.DeleteBook(book_id);
            std::cerr << "DEBUG: Book deletion completed" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "DEBUG: Exception in DeleteBook: " << e.what() << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::cerr << "DEBUG: Starting EditBook command" << std::endl;
        
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::cerr << "DEBUG: EditBook title: '" << title << "'" << std::endl;
        
        std::string book_id;
        
        if (title.empty()) {
            std::cerr << "DEBUG: Empty title, showing book list for editing" << std::endl;
            
            auto books = use_cases_.GetBooksExtended();
            if (books.empty()) {
                output_ << "Book not found" << std::endl;
                return true;
            }
            
            output_ << "Select book:" << std::endl;
            int i = 1;
            for (const auto& book : books) {
                output_ << i++ << " " << book.title << " by " << book.author_name 
                       << ", " << book.publication_year << std::endl;
            }
            output_ << "Enter the book # or empty line to cancel: ";
            
            std::string choice;
            if (!std::getline(input_, choice)) {
                return true;
            }
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                std::cerr << "DEBUG: User cancelled book selection" << std::endl;
                return true;
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    book_id = books[idx].id;
                    std::cerr << "DEBUG: Editing book with id: " << book_id << std::endl;
                } else {
                    output_ << "Book not found" << std::endl;
                    return true;
                }
            } catch (...) {
                output_ << "Book not found" << std::endl;
                return true;
            }
            
        } else {
            std::cerr << "DEBUG: Looking for books to edit with title: '" << title << "'" << std::endl;
            
            auto books = use_cases_.GetBooksByTitle(title);
            std::cerr << "DEBUG: Found " << books.size() << " books with this title" << std::endl;
            
            if (books.empty()) {
                output_ << "Book not found" << std::endl;
                return true;
            } else if (books.size() == 1) {
                book_id = books[0].id;
                std::cerr << "DEBUG: Single book found, editing id: " << book_id << std::endl;
            } else {
                std::cerr << "DEBUG: Multiple books found, showing selection for editing" << std::endl;
                
                output_ << "Multiple books found with title \"" << title << "\":" << std::endl;
                int i = 1;
                for (const auto& book : books) {
                    output_ << i++ << " " << book.title << " by " << book.author_name 
                           << ", " << book.publication_year << std::endl;
                }
                output_ << "Enter the book # or empty line to cancel: ";
                
                std::string choice;
                if (!std::getline(input_, choice)) {
                    return true;
                }
                boost::algorithm::trim(choice);
                
                if (choice.empty()) {
                    std::cerr << "DEBUG: User cancelled book selection" << std::endl;
                    return true;
                }
                
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(books.size())) {
                        book_id = books[idx].id;
                        std::cerr << "DEBUG: Editing book with id: " << book_id << std::endl;
                    } else {
                        output_ << "Book not found" << std::endl;
                        return true;
                    }
                } catch (...) {
                    output_ << "Book not found" << std::endl;
                    return true;
                }
            }
        }
        
        if (book_id.empty()) {
            output_ << "Book not found" << std::endl;
            return true;
        }
        
        auto book = use_cases_.GetBookById(book_id);
        if (!book) {
            output_ << "Book not found" << std::endl;
            return true;
        }
        
        std::cerr << "DEBUG: Current book - title: " << book->title 
                  << ", author: " << book->author_name 
                  << ", year: " << book->publication_year 
                  << ", tags count: " << book->tags.size() << std::endl;
        for (const auto& tag : book->tags) {
            std::cerr << "DEBUG:   tag: '" << tag << "'" << std::endl;
        }
        
        // Ввод нового названия
        output_ << "Enter new title or empty line to use the current one (" << book->title << "): ";
        std::string new_title;
        if (!std::getline(input_, new_title)) {
            return true;
        }
        boost::algorithm::trim(new_title);
        if (new_title.empty()) {
            new_title = book->title;
        }
        std::cerr << "DEBUG: New title: '" << new_title << "'" << std::endl;
        
        // Ввод года - с явной очисткой буфера
        input_.clear();
        output_ << "Enter publication year or empty line to use the current one (" << book->publication_year << "): ";
        std::string year_str;
        if (!std::getline(input_, year_str)) {
            return true;
        }
        boost::algorithm::trim(year_str);
        std::cerr << "DEBUG: Year input: '" << year_str << "'" << std::endl;
        
        int new_year = book->publication_year;
        if (!year_str.empty()) {
            try {
                new_year = std::stoi(year_str);
                std::cerr << "DEBUG: New year parsed: " << new_year << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "DEBUG: Failed to parse year, using current: " << e.what() << std::endl;
                // Оставляем текущий год
            }
        } else {
            std::cerr << "DEBUG: Using current year: " << new_year << std::endl;
        }
        
        // Ввод тегов - с явной очисткой буфера
        input_.clear();
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
        if (!std::getline(input_, tags_input)) {
            return true;
        }
        boost::algorithm::trim(tags_input);
        std::cerr << "DEBUG: Tags input raw: '" << tags_input << "'" << std::endl;
        
        auto tags = ParseAndNormalizeTags(tags_input);
        
        std::cerr << "DEBUG: Final book data - new_title: " << new_title 
                  << ", new_year: " << new_year 
                  << ", new_tags_count: " << tags.size() << std::endl;
        for (const auto& tag : tags) {
            std::cerr << "DEBUG:   new_tag: '" << tag << "'" << std::endl;
        }
        
        use_cases_.EditBook(book_id, new_title, new_year, tags);
        std::cerr << "DEBUG: Book editing completed" << std::endl;
        
    } catch (const std::exception& e) {
        output_ << "Failed to edit book" << std::endl;
        std::cerr << "DEBUG: Exception in EditBook: " << e.what() << std::endl;
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    if (!(cmd_input >> params.publication_year)) {
        std::cerr << "DEBUG: Failed to read publication year" << std::endl;
        return std::nullopt;
    }

    cmd_input.get();

    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    std::cerr << "DEBUG: GetBookParams - year: " << params.publication_year << ", title: '" << title << "'" << std::endl;

    if (title.empty()) {
        std::cerr << "DEBUG: Empty title in GetBookParams" << std::endl;
        return std::nullopt;
    }

    params.title = title;

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    if (!std::getline(input_, author_name)) {
        std::cerr << "DEBUG: Failed to read author name" << std::endl;
        return std::nullopt;
    }
    boost::algorithm::trim(author_name);

    std::cerr << "DEBUG: Author input: '" << author_name << "'" << std::endl;

    if (author_name.empty()) {
        std::cerr << "DEBUG: Empty author name, selecting from list" << std::endl;
        auto author_id = SelectAuthor();
        if (!author_id) {
            std::cerr << "DEBUG: No author selected - cancellation" << std::endl;
            output_ << "Failed to add book" << std::endl;
            
            std::string next_line;
            if (std::getline(input_, next_line)) {
                std::cerr << "DEBUG: Discarding tags input after cancellation: '" << next_line << "'" << std::endl;
            }
            return std::nullopt;
        }
        
        auto authors = GetAuthors();
        bool author_found = false;
        for (const auto& author : authors) {
            if (author.id == *author_id) {
                params.author_name = author.name;
                author_found = true;
                std::cerr << "DEBUG: Selected author from list: " << author.name << std::endl;
                break;
            }
        }
        
        if (!author_found) {
            output_ << "Author not found" << std::endl;
            std::cerr << "DEBUG: Author not found in list after selection" << std::endl;
            return std::nullopt;
        }
    } else {
        auto author = use_cases_.GetAuthorByName(author_name);
        if (!author) {
            output_ << "No author found. Do you want to add " << author_name << " (y/n)?" << std::endl;
            std::string answer;
            if (!std::getline(input_, answer)) {
                std::cerr << "DEBUG: Failed to read answer for adding author" << std::endl;
                return std::nullopt;
            }
            boost::algorithm::trim(answer);
            std::cerr << "DEBUG: User answer for adding author: '" << answer << "'" << std::endl;
            
            if (answer == "y" || answer == "Y") {
                try {
                    use_cases_.AddAuthor(author_name);
                    params.author_name = author_name;
                    std::cerr << "DEBUG: Author added: " << author_name << std::endl;
                } catch (const std::exception& e) {
                    output_ << "Failed to add author" << std::endl;
                    std::cerr << "DEBUG: Exception when adding author: " << e.what() << std::endl;
                    return std::nullopt;
                }
            } else {
                output_ << "Failed to add book" << std::endl;
                std::cerr << "DEBUG: User declined to add author" << std::endl;
                
                std::string next_line;
                if (std::getline(input_, next_line)) {
                    std::cerr << "DEBUG: Discarding tags input after decline: '" << next_line << "'" << std::endl;
                }
                return std::nullopt;
            }
        } else {
            params.author_name = author->name;
            std::cerr << "DEBUG: Existing author found: " << author->name << std::endl;
        }
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    if (!std::getline(input_, tags_input)) {
        std::cerr << "DEBUG: Failed to read tags input" << std::endl;
        return std::nullopt;
    }
    
    params.tags = ParseAndNormalizeTags(tags_input);
    std::cerr << "DEBUG: Tags input: '" << tags_input << "', normalized count: " << params.tags.size() << std::endl;

    return params;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintAuthors(authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str)) {
        std::cerr << "DEBUG: SelectAuthor - failed to read input" << std::endl;
        return std::nullopt;
    }
    boost::algorithm::trim(str);

    std::cerr << "DEBUG: SelectAuthor - user input: '" << str << "'" << std::endl;

    if (str.empty()) {
        std::cerr << "DEBUG: SelectAuthor - user cancelled" << std::endl;
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        std::cerr << "DEBUG: SelectAuthor - invalid number: '" << str << "'" << std::endl;
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= static_cast<int>(authors.size())) {
        std::cerr << "DEBUG: SelectAuthor - index out of range: " << author_idx << std::endl;
        throw std::runtime_error("Invalid author num");
    }

    std::cerr << "DEBUG: SelectAuthor - selected author: " << authors[author_idx].name << std::endl;
    return authors[author_idx].id;
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

void View::PrintAuthors(const std::vector<detail::AuthorInfo>& authors) const {
    int i = 1;
    for (const auto& author : authors) {
        output_ << i++ << " " << author.name << std::endl;
    }
}

void View::PrintBooks(const std::vector<detail::BookInfo>& books) const {
    int i = 1;
    for (const auto& book : books) {
        output_ << i++ << " " << book.title << ", " << book.publication_year << std::endl;
    }
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
                if (!last_was_space && !normalized_tag.empty()) {
                    normalized_tag += ' ';
                    last_was_space = true;
                }
            } else {
                normalized_tag += c;
                last_was_space = false;
            }
        }
        
        boost::algorithm::trim(normalized_tag);
        
        // Проверяем длину тега (максимум 30 символов)
        if (!normalized_tag.empty()) {
            if (normalized_tag.length() > 30) {
                std::cerr << "DEBUG: Tag too long, truncating: '" << normalized_tag << "'" << std::endl;
                normalized_tag = normalized_tag.substr(0, 30);
                // Убедимся, что не обрезали посередине слова
                size_t last_space = normalized_tag.find_last_of(' ');
                if (last_space != std::string::npos && last_space > 25) {
                    normalized_tag = normalized_tag.substr(0, last_space);
                }
                boost::algorithm::trim(normalized_tag);
            }
            tags.push_back(std::move(normalized_tag));
        }
    }
    
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    
    return tags;
}

}