#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {

using namespace domain;

domain::BookRepository* UseCasesImpl::books_ = nullptr;
domain::AuthorQueries* UseCasesImpl::author_queries_ = nullptr;

void UseCasesImpl::SetBookRepository(domain::BookRepository* book_repo) {
    books_ = book_repo;
}

void UseCasesImpl::SetAuthorQueries(domain::AuthorQueries* author_queries) {
    author_queries_ = author_queries;
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<AuthorInfo> UseCasesImpl::GetAuthors() {
    if (!author_queries_) return {};
    
    auto domain_authors = author_queries_->GetAllAuthors();
    std::vector<AuthorInfo> result;
    for (const auto& author : domain_authors) {
        result.push_back({author.GetId().ToString(), author.GetName()});
    }
    return result;
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    if (books_) {
        books_->Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
    }
}

std::vector<BookInfo> UseCasesImpl::GetBooks() {
    if (!books_) return {};
    
    auto domain_books = books_->GetAll();
    std::vector<BookInfo> result;
    for (const auto& book : domain_books) {
        result.push_back({book.GetTitle(), book.GetPublicationYear()});
    }
    return result;
}

std::vector<BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    if (!books_) return {};
    
    auto domain_books = books_->GetByAuthorId(AuthorId::FromString(author_id));
    std::vector<BookInfo> result;
    for (const auto& book : domain_books) {
        result.push_back({book.GetTitle(), book.GetPublicationYear()});
    }
    return result;
}

}  // namespace app