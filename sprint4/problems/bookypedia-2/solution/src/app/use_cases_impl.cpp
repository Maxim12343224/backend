// solution/src/app/use_cases_impl.cpp
#include "use_cases_impl.h"
#include "../domain/author.h"
#include <pqxx/pqxx>

namespace app {

using namespace domain;

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

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year, const std::vector<std::string>& tags) {
    auto book = Book{BookId::New(), AuthorId::FromString(author_id), title, publication_year};
    books_.Save(book);
    if (!tags.empty()) {
        books_.SetBookTags(book.GetId(), tags);
    }
}

std::vector<BookInfo> UseCasesImpl::GetBooks() {
    auto domain_books = books_.GetAll();
    std::vector<BookInfo> books;
    for (const auto& book : domain_books) {
        auto author = authors_.GetById(book.GetAuthorId());
        if (author) {
            auto tags = books_.GetBookTags(book.GetId());
            books.push_back(BookInfo{
                book.GetId().ToString(),
                book.GetTitle(),
                author->GetName(),
                book.GetPublicationYear(),
                tags
            });
        }
    }
    
    // Sort by title, then by author name, then by publication year
    std::sort(books.begin(), books.end(), [](const BookInfo& a, const BookInfo& b) {
        if (a.title != b.title) return a.title < b.title;
        if (a.author_name != b.author_name) return a.author_name < b.author_name;
        return a.publication_year < b.publication_year;
    });
    
    return books;
}

std::vector<BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    auto domain_books = books_.GetByAuthorId(AuthorId::FromString(author_id));
    std::vector<BookInfo> books;
    auto author = authors_.GetById(AuthorId::FromString(author_id));
    if (!author) return books;
    
    for (const auto& book : domain_books) {
        auto tags = books_.GetBookTags(book.GetId());
        books.push_back(BookInfo{
            book.GetId().ToString(),
            book.GetTitle(),
            author->GetName(),
            book.GetPublicationYear(),
            tags
        });
    }
    return books;
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    authors_.Delete(AuthorId::FromString(author_id));
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name) {
    auto author = authors_.GetById(AuthorId::FromString(author_id));
    if (author) {
        authors_.Update(Author{author->GetId(), new_name});
    } else {
        throw std::runtime_error("Author not found");
    }
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    books_.Delete(BookId::FromString(book_id));
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::string& new_title, int new_publication_year, const std::vector<std::string>& tags) {
    auto book = books_.GetById(BookId::FromString(book_id));
    if (book) {
        books_.Update(Book{book->GetId(), book->GetAuthorId(), new_title, new_publication_year});
        books_.SetBookTags(book->GetId(), tags);
    } else {
        throw std::runtime_error("Book not found");
    }
}

std::optional<AuthorInfo> UseCasesImpl::GetAuthorById(const std::string& author_id) {
    auto author = authors_.GetById(AuthorId::FromString(author_id));
    if (author) {
        return AuthorInfo{author->GetId().ToString(), author->GetName()};
    }
    return std::nullopt;
}

std::optional<AuthorInfo> UseCasesImpl::GetAuthorByName(const std::string& name) {
    auto author = authors_.GetByName(name);
    if (author) {
        return AuthorInfo{author->GetId().ToString(), author->GetName()};
    }
    return std::nullopt;
}

std::vector<BookInfo> UseCasesImpl::GetBooksByTitle(const std::string& title) {
    auto domain_books = books_.GetByTitle(title);
    std::vector<BookInfo> books;
    for (const auto& book : domain_books) {
        auto author = authors_.GetById(book.GetAuthorId());
        if (author) {
            auto tags = books_.GetBookTags(book.GetId());
            books.push_back(BookInfo{
                book.GetId().ToString(),
                book.GetTitle(),
                author->GetName(),
                book.GetPublicationYear(),
                tags
            });
        }
    }
    return books;
}

std::optional<BookInfo> UseCasesImpl::GetBookById(const std::string& book_id) {
    auto book = books_.GetById(BookId::FromString(book_id));
    if (book) {
        auto author = authors_.GetById(book->GetAuthorId());
        if (author) {
            auto tags = books_.GetBookTags(book->GetId());
            return BookInfo{
                book->GetId().ToString(),
                book->GetTitle(),
                author->GetName(),
                book->GetPublicationYear(),
                tags
            };
        }
    }
    return std::nullopt;
}

std::vector<std::string> UseCasesImpl::GetBookTags(const std::string& book_id) {
    return books_.GetBookTags(BookId::FromString(book_id));
}

}  // namespace app