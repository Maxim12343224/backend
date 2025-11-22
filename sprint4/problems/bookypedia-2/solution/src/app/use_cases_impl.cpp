#include "use_cases_impl.h"

#include "../domain/author.h"
#include <pqxx/pqxx>
#include <regex>

namespace app {

using namespace domain;

namespace {

bool IsUUID(const std::string& str) {
    // Проверяем, является ли строка UUID (формат: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)
    static const std::regex uuid_regex(
        "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$", 
        std::regex::icase
    );
    return std::regex_match(str, uuid_regex);
}

} // namespace

domain::BookRepository* UseCasesImpl::books_ = nullptr;
pqxx::connection* UseCasesImpl::connection_ = nullptr;

void UseCasesImpl::SetBookRepository(domain::BookRepository* book_repo) {
    books_ = book_repo;
}

void UseCasesImpl::SetConnection(pqxx::connection* connection) {
    connection_ = connection;
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<AuthorInfo> UseCasesImpl::GetAuthors() {
    if (!connection_) return {};
    
    try {
        pqxx::read_transaction work{*connection_};
        auto result = work.exec("SELECT id, name FROM authors ORDER BY name");
        std::vector<AuthorInfo> authors;
        
        for (const auto& row : result) {
            std::string id = row[0].as<std::string>();
            std::string name = row[1].as<std::string>();
            authors.push_back(AuthorInfo{std::move(id), std::move(name)});
        }
        
        return authors;
    } catch (const std::exception& e) {
        return {};
    }
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    if (books_) {
        books_->Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
    }
}

std::vector<BookInfo> UseCasesImpl::GetBooks() {
    if (!books_) return {};
    
    try {
        auto domain_books = books_->GetAll();
        std::vector<BookInfo> result;
        for (const auto& book : domain_books) {
            result.push_back(BookInfo{book.GetTitle(), book.GetPublicationYear()});
        }
        return result;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    if (!books_) return {};
    
    try {
        auto domain_books = books_->GetByAuthorId(AuthorId::FromString(author_id));
        std::vector<BookInfo> result;
        for (const auto& book : domain_books) {
            result.push_back(BookInfo{book.GetTitle(), book.GetPublicationYear()});
        }
        return result;
    } catch (const std::exception& e) {
        return {};
    }
}

void UseCasesImpl::AddBookWithAuthorAndTags(const std::string& author_name, const std::string& title, 
                                           int publication_year, const std::vector<std::string>& tags) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        // Поиск автора по имени
        auto author_result = work.exec("SELECT id FROM authors WHERE name = " + work.quote(author_name));
        domain::AuthorId author_id;
        
        if (author_result.empty()) {
            // Автор не найден - создаем нового
            author_id = AuthorId::New();
            work.exec("INSERT INTO authors (id, name) VALUES (" +
                      work.quote(author_id.ToString()) + ", " +
                      work.quote(author_name) + ")");
        } else {
            author_id = domain::AuthorId::FromString(author_result[0][0].as<std::string>());
        }
        
        // Создание книги
        auto book_id = BookId::New();
        work.exec("INSERT INTO books (id, author_id, title, publication_year) VALUES (" +
                  work.quote(book_id.ToString()) + ", " +
                  work.quote(author_id.ToString()) + ", " +
                  work.quote(title) + ", " +
                  work.quote(publication_year) + ")");
        
        // Добавление тегов
        for (const auto& tag : tags) {
            if (!tag.empty()) {
                work.exec("INSERT INTO book_tags (book_id, tag) VALUES (" +
                          work.quote(book_id.ToString()) + ", " +
                          work.quote(tag) + ")");
            }
        }
        
        work.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to add book");
    }
}





void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        // Явно удаляем все связанные данные
        work.exec("DELETE FROM book_tags WHERE book_id IN "
                 "(SELECT id FROM books WHERE author_id = " + work.quote(author_id) + ")");
        work.exec("DELETE FROM books WHERE author_id = " + work.quote(author_id));
        
        // Затем удаляем автора
        auto result = work.exec("DELETE FROM authors WHERE id = " + work.quote(author_id));
        work.commit();
        
        if (result.affected_rows() == 0) {
            throw std::runtime_error("Author not found");
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to delete author");
    }
}







void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        auto result = work.exec("UPDATE authors SET name = " + work.quote(new_name) + 
                  " WHERE id = " + work.quote(author_id));
        work.commit();
        
        if (result.affected_rows() == 0) {
            throw std::runtime_error("Author not found");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to edit author");
    }
}







//Базовое решение
/*void UseCasesImpl::DeleteBook(const std::string& book_id_or_title) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        std::string book_id;
        
        // Сначала проверяем, является ли ввод ID книги
        auto book_by_id = work.exec("SELECT id FROM books WHERE id = " + work.quote(book_id_or_title));
        if (!book_by_id.empty()) {
            book_id = book_by_id[0][0].as<std::string>();
        } else {
            // Если не нашли по ID, ищем по названию
            auto book_by_title = work.exec("SELECT id FROM books WHERE title = " + work.quote(book_id_or_title));
            if (book_by_title.empty()) {
                throw std::runtime_error("Book not found");
            }
            // Если найдено несколько книг с одинаковым названием, берем первую
            book_id = book_by_title[0][0].as<std::string>();
        }
        
        auto result = work.exec("DELETE FROM books WHERE id = " + work.quote(book_id));
        work.commit();
        
        if (result.affected_rows() == 0) {
            throw std::runtime_error("Book not found");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to delete book");
    }
}*/




void UseCasesImpl::DeleteBook(const std::string& identifier) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        std::string book_id;
        
        // Проверяем, является ли identifier UUID
        bool is_uuid = false;
        try {
            domain::BookId::FromString(identifier);
            is_uuid = true;
        } catch (...) {
            is_uuid = false;
        }
        
        if (is_uuid) {
            book_id = identifier;
        } else {
            // Это название - находим ID
            auto book_result = work.exec(
                "SELECT id FROM books WHERE title = " + work.quote(identifier)
            );
            
            if (book_result.empty()) {
                throw std::runtime_error("Book not found");
            }
            
            // Если несколько книг с одинаковым названием - берем первую
            book_id = book_result[0][0].as<std::string>();
        }
        
        // Явно удаляем теги
        work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id));
        
        // Удаляем книгу
        auto result = work.exec("DELETE FROM books WHERE id = " + work.quote(book_id));
        work.commit();
        
        if (result.affected_rows() == 0) {
            throw std::runtime_error("Book not found");
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to delete book");
    }
}






void UseCasesImpl::EditBook(const std::string& book_id, const std::string& new_title, 
                           int new_publication_year, const std::vector<std::string>& tags) {
    if (!connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        // Обновление информации о книге
        auto result = work.exec("UPDATE books SET title = " + work.quote(new_title) + 
                  ", publication_year = " + work.quote(new_publication_year) +
                  " WHERE id = " + work.quote(book_id));
        
        if (result.affected_rows() == 0) {
            throw std::runtime_error("Book not found");
        }
        
        // Обновление тегов
        work.exec("DELETE FROM book_tags WHERE book_id = " + work.quote(book_id));
        for (const auto& tag : tags) {
            if (!tag.empty()) {
                work.exec("INSERT INTO book_tags (book_id, tag) VALUES (" +
                          work.quote(book_id) + ", " + work.quote(tag) + ")");
            }
        }
        
        work.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to edit book");
    }
}

std::vector<BookInfoExtended> UseCasesImpl::GetBooksExtended() {
    if (!connection_) return {};
    
    try {
        pqxx::read_transaction work{*connection_};
        auto result = work.exec(
            "SELECT b.id, b.title, a.name, b.publication_year "
            "FROM books b "
            "JOIN authors a ON b.author_id = a.id "
            "ORDER BY b.title, a.name, b.publication_year" 
        );
        
        std::vector<BookInfoExtended> books;
        for (const auto& row : result) {
            auto book_id = row[0].as<std::string>();
            
            auto tags_result = work.exec(
                "SELECT tag FROM book_tags WHERE book_id = " + work.quote(book_id) + " ORDER BY tag"
            );
            std::vector<std::string> tags;
            for (const auto& tag_row : tags_result) {
                tags.push_back(tag_row[0].as<std::string>());
            }
            
            books.push_back(BookInfoExtended{
                book_id,
                row[1].as<std::string>(),
                row[2].as<std::string>(),
                row[3].as<int>(),
                tags
            });
        }
        return books;
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<BookInfoExtended> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    if (!connection_) return {};
    
    try {
        pqxx::read_transaction work{*connection_};
        auto result = work.exec(
            "SELECT b.id, b.title, a.name, b.publication_year "
            "FROM books b "
            "JOIN authors a ON b.author_id = a.id "
            "WHERE b.title = " + work.quote(title) + 
            " ORDER BY a.name, b.publication_year"
        );
        
        std::vector<BookInfoExtended> books;
        for (const auto& row : result) {
            auto book_id = row[0].as<std::string>();
            
            auto tags_result = work.exec(
                "SELECT tag FROM book_tags WHERE book_id = " + work.quote(book_id) + " ORDER BY tag"
            );
            std::vector<std::string> tags;
            for (const auto& tag_row : tags_result) {
                tags.push_back(tag_row[0].as<std::string>());
            }
            
            books.push_back(BookInfoExtended{
                book_id,
                row[1].as<std::string>(),
                row[2].as<std::string>(),
                row[3].as<int>(),
                tags
            });
        }
        return books;
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<BookInfoExtended> UseCasesImpl::GetBookById(const std::string& book_id) {
    if (!connection_) return std::nullopt;
    
    try {
        pqxx::read_transaction work{*connection_};
        auto result = work.exec(
            "SELECT b.title, a.name, b.publication_year "
            "FROM books b "
            "JOIN authors a ON b.author_id = a.id "
            "WHERE b.id = " + work.quote(book_id)
        );
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        // Получение тегов для книги
        auto tags_result = work.exec(
            "SELECT tag FROM book_tags WHERE book_id = " + work.quote(book_id) + " ORDER BY tag"
        );
        std::vector<std::string> tags;
        for (const auto& tag_row : tags_result) {
            tags.push_back(tag_row[0].as<std::string>());
        }
        
        return BookInfoExtended{
            book_id,
            result[0][0].as<std::string>(),
            result[0][1].as<std::string>(),
            result[0][2].as<int>(),
            tags
        };
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::optional<AuthorInfo> UseCasesImpl::GetAuthorByName(const std::string& name) {
    if (!connection_) return std::nullopt;
    
    try {
        pqxx::read_transaction work{*connection_};
        auto result = work.exec("SELECT id, name FROM authors WHERE name = " + work.quote(name));
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        return AuthorInfo{
            result[0][0].as<std::string>(),
            result[0][1].as<std::string>()
        };
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

}  // namespace app