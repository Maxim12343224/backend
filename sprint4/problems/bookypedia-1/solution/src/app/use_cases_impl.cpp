#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {

using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<AuthorInfo> UseCasesImpl::GetAuthors() {
    return {};
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    if (books_) {
        books_->Save({BookId::New(), AuthorId::FromString(author_id), title, publication_year});
    }
}

std::vector<BookInfo> UseCasesImpl::GetBooks() {
    return {};
}

std::vector<BookInfo> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return {};
}

}  // namespace app