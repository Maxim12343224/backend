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
            // GetBookParams уже вывел сообщение об ошибке
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
        // Silent fail as per requirements
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
                return true;
            }
            
            try {
                int idx = std::stoi(choice) - 1;
                if (idx >= 0 && idx < static_cast<int>(books.size())) {
                    PrintBookDetails(books[idx]);
                }
            } catch (...) {
                // Invalid input
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
                    return true;
                }
                
                try {
                    int idx = std::stoi(choice) - 1;
                    if (idx >= 0 && idx < static_cast<int>(books.size())) {
                        PrintBookDetails(books[idx]);
                    }
                } catch (...) {
                    // Invalid input
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
                // Keep current year
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
        
        std::cerr << "DEBUG: Calling EditBook with id: " << book_id 
                  << ", title: " << new_title 
                  << ", year: " << new_year 
                  << ", tags count: " << tags.size() << std::endl;
        
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

    // Сначала читаем год публикации
    if (!(cmd_input >> params.publication_year)) {
        std::cerr << "DEBUG: Failed to read publication year" << std::endl;
        return std::nullopt;
    }

    // Пропускаем пробел после года
    cmd_input.get();

    // Читаем оставшуюся часть строки как название книги
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

    // Функция для определения, является ли строка тегами
    auto isLikelyTags = [](const std::string& str) -> bool {
        if (str.empty()) return true;  // Пустая строка - это теги (нет тегов)
        
        // Если содержит запятые - скорее всего теги
        if (str.find(',') != std::string::npos) return true;
        
        // Если это одно слово и не похоже на команду - может быть тегом
        static const std::vector<std::string> commands = {
            "AddAuthor", "AddBook", "ShowAuthors", "ShowBooks", 
            "ShowAuthorBooks", "DeleteAuthor", "EditAuthor", 
            "DeleteBook", "EditBook", "ShowBook", "Help", "Exit"
        };
        
        std::string first_word = str.substr(0, str.find(' '));
        for (const auto& cmd : commands) {
            if (first_word == cmd) {
                return false;  // Это команда
            }
        }
        
        // Если не команда и без запятых, считаем что это один тег
        return true;
    };

    if (author_name.empty()) {
        std::cerr << "DEBUG: Empty author name, selecting from list" << std::endl;
        auto author_id = SelectAuthor();
        if (!author_id) {
            std::cerr << "DEBUG: No author selected - cancellation" << std::endl;
            output_ << "Failed to add book" << std::endl;
            
            // УМНОЕ ПОГЛОЩЕНИЕ: смотрим следующую строку, но не читаем её
            // Просто проверяем, является ли она тегами
            std::streampos original_pos = input_.tellg();
            std::string next_line;
            if (std::getline(input_, next_line)) {
                boost::algorithm::trim(next_line);
                std::cerr << "DEBUG: Next line after cancellation: '" << next_line << "'" << std::endl;
                
                if (isLikelyTags(next_line)) {
                    std::cerr << "DEBUG: Detected tags, discarding: '" << next_line << "'" << std::endl;
                    // Теги поглощаем - ничего не делаем, строка уже прочитана
                } else {
                    std::cerr << "DEBUG: Detected command, seeking back: '" << next_line << "'" << std::endl;
                    // Это команда - пытаемся вернуться назад
                    input_.seekg(original_pos);
                    if (input_.fail()) {
                        std::cerr << "DEBUG: WARNING - cannot seek back, command will be lost" << std::endl;
                    }
                }
            }
            return std::nullopt;
        }
        
        // Находим имя выбранного автора
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
        // Пользователь ввел имя автора напрямую
        auto author = use_cases_.GetAuthorByName(author_name);
        if (!author) {
            // Автор не найден - предлагаем добавить
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
                
                // УМНОЕ ПОГЛОЩЕНИЕ для случая отказа от добавления автора
                std::streampos original_pos = input_.tellg();
                std::string next_line;
                if (std::getline(input_, next_line)) {
                    boost::algorithm::trim(next_line);
                    std::cerr << "DEBUG: Next line after decline: '" << next_line << "'" << std::endl;
                    
                    if (isLikelyTags(next_line)) {
                        std::cerr << "DEBUG: Detected tags, discarding: '" << next_line << "'" << std::endl;
                        // Теги поглощаем
                    } else {
                        std::cerr << "DEBUG: Detected command, seeking back: '" << next_line << "'" << std::endl;
                        // Это команда - пытаемся вернуться назад
                        input_.seekg(original_pos);
                        if (input_.fail()) {
                            std::cerr << "DEBUG: WARNING - cannot seek back, command will be lost" << std::endl;
                        }
                    }
                }
                return std::nullopt;
            }
        } else {
            // Автор существует
            params.author_name = author->name;
            std::cerr << "DEBUG: Existing author found: " << author->name << std::endl;
        }
    }

    // Если мы дошли до этого места, значит автор выбран/добавлен
    // Теперь запрашиваем теги
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

}