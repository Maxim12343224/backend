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
    if (!connection_) return {};
    
    try {
        pqxx::nontransaction work{*connection_};
        auto result = work.exec("SELECT id, name FROM authors ORDER BY name");
        std::vector<AuthorInfo> authors;
        
        for (int i = 0; i < result.size(); ++i) {
            std::string id = result[i][0].as<std::string>();
            std::string name = result[i][1].as<std::string>();
            authors.push_back(AuthorInfo{std::move(id), std::move(name)});
        }
        
        return authors;
    } catch (const std::exception& e) {
        // В реальной программе здесь должно быть логирование ошибки
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

}  // namespace app