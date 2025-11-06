// solution/src/app/use_cases_impl.cpp
#include "use_cases_impl.h"

#include "../domain/author.h"
#include <pqxx/pqxx>

namespace app {

using namespace domain;

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
    auto domain_authors = authors_.GetAll();
    std::vector<AuthorInfo> authors;
    for (const auto& author : domain_authors) {
        authors.push_back(AuthorInfo{author.GetId().ToString(), author.GetName()});
    }
    return authors;
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

// Реализации новых методов
void UseCasesImpl::AddBookWithAuthorAndTags(const std::string& author_name, const std::string& title, 
                                           int publication_year, const std::vector<std::string>& tags) {
    if (!books_ || !connection_) return;
    
    try {
        pqxx::work work{*connection_};
        
        // Получаем или создаем автора
        auto author = authors_.GetByName(author_name);
        domain::AuthorId author_id;
        
        if (author) {
            author_id = author->GetId();
        } else {
            author_id = AuthorId::New();
            work.exec("INSERT INTO authors (id, name) VALUES (" +
                      work.quote(author_id.ToString()) + ", " +
                      work.quote(author_name) + ")");
        }
        
        // Создаем книгу
        auto book_id = BookId::New();
        work.exec("INSERT INTO books (id, author_id, title, publication_year) VALUES (" +
                  work.quote(book_id.ToString()) + ", " +
                  work.quote(author_id.ToString()) + ", " +
                  work.quote(title) + ", " +
                  work.quote(publication_year) + ")");
        
        // Добавляем теги
        for (const auto& tag : tags) {
            work.exec("INSERT INTO book_tags (book_id, tag) VALUES (" +
                      work.quote(book_id.ToString()) + ", " +
                      work.quote(tag) + ")");
        }
        
        work.commit();
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to add book: " + std::string(e.what()));
    }
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    authors_.Delete(AuthorId::FromString(author_id));
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name) {
    auto author = authors_.GetById(AuthorId::FromString(author_id));
    if (author) {
        authors_.Update(domain::Author{author->GetId(), new_name});
    } else {
        throw std::runtime_error("Author not found");
    }
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    if (books_) {
        books_->Delete(BookId::FromString(book_id));
    }
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::string& new_title, 
                           int new_publication_year, const std::vector<std::string>& tags) {
    if (!books_) return;
    
    auto book = books_->GetById(BookId::FromString(book_id));
    if (book) {
        books_->Update(domain::Book{book->GetId(), book->GetAuthorId(), new_title, new_publication_year});
        books_->SetBookTags(book->GetId(), tags);
    } else {
        throw std::runtime_error("Book not found");
    }
}

std::vector<BookInfoExtended> UseCasesImpl::GetBooksExtended() {
    if (!books_ || !connection_) return {};
    
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
            auto book_id = domain::BookId::FromString(row[0].as<std::string>());
            auto tags = books_->GetBookTags(book_id);
            
            books.push_back(BookInfoExtended{
                row[0].as<std::string>(),
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
    if (!books_ || !connection_) return {};
    
    try {
        auto domain_books = books_->GetByTitle(title);
        std::vector<BookInfoExtended> books;
        
        for (const auto& book : domain_books) {
            auto author = authors_.GetById(book.GetAuthorId());
            if (author) {
                auto tags = books_->GetBookTags(book.GetId());
                books.push_back(BookInfoExtended{
                    book.GetId().ToString(),
                    book.GetTitle(),
                    author->GetName(),
                    book.GetPublicationYear(),
                    tags
                });
            }
        }
        return books;
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<BookInfoExtended> UseCasesImpl::GetBookById(const std::string& book_id) {
    if (!books_) return std::nullopt;
    
    try {
        auto book = books_->GetById(BookId::FromString(book_id));
        if (!book) return std::nullopt;
        
        auto author = authors_.GetById(book->GetAuthorId());
        if (!author) return std::nullopt;
        
        auto tags = books_->GetBookTags(book->GetId());
        
        return BookInfoExtended{
            book->GetId().ToString(),
            book->GetTitle(),
            author->GetName(),
            book->GetPublicationYear(),
            tags
        };
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::optional<AuthorInfo> UseCasesImpl::GetAuthorByName(const std::string& name) {
    auto author = authors_.GetByName(name);
    if (author) {
        return AuthorInfo{author->GetId().ToString(), author->GetName()};
    }
    return std::nullopt;
}

}  // namespace app