#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<AuthorInfo> UseCasesImpl::GetAuthors() {
    auto domain_authors = authors_.GetAll();
    std::vector<AuthorInfo> result;
    for (const auto& author : domain_authors) {
        result.push_back({author.GetId().ToString(), author.GetName()});
    }
    return result;
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    books_.Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
}

std::vector<BookInfo> UseCasesImpl::GetBooks() {
    auto domain_books = books_.GetAll();
    std::vector<BookInfo> result;
    for (const auto& book : domain_books) {
        result.push_back({book.GetTitle(), book.GetPublicationYear()});
    }
    return result;
}

std::vector<BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    auto domain_books = books_.GetByAuthorId(AuthorId::FromString(author_id));
    std::vector<BookInfo> result;
    for (const auto& book : domain_books) {
        result.push_back({book.GetTitle(), book.GetPublicationYear()});
    }
    return result;
}

}  // namespace app