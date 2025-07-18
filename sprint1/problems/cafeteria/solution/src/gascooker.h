#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <cassert>
#include <deque>
#include <memory>
#include <mutex>
#include <atomic>

namespace net = boost::asio;
namespace sys = boost::system;

class GasCooker : public std::enable_shared_from_this<GasCooker> {
public:
    using Handler = std::function<void()>;

    GasCooker(net::io_context& io, int num_burners = 8)
        : io_{ io }
        , number_of_burners_{ num_burners }
        , strand_{ net::make_strand(io) } {
        if (num_burners <= 0) {
            throw std::invalid_argument("Number of burners must be positive");
        }
    }

    GasCooker(const GasCooker&) = delete;
    GasCooker& operator=(const GasCooker&) = delete;

    ~GasCooker() {
        assert(burners_in_use_ == 0 && "All burners should be released before destruction");
    }

    void UseBurner(Handler handler) {
        net::dispatch(strand_,
            [this, self = shared_from_this(), handler = std::move(handler)]() mutable {
                assert(strand_.running_in_this_thread());

                if (burners_in_use_ < number_of_burners_) {
                    ++burners_in_use_;
                    net::post(io_, std::move(handler));
                }
                else {
                    pending_handlers_.emplace_back(std::move(handler));
                }

                assert(burners_in_use_ >= 0 && burners_in_use_ <= number_of_burners_);
            });
    }

    void ReleaseBurner() {
        net::dispatch(strand_, [this, self = shared_from_this()] {
            assert(strand_.running_in_this_thread());
            assert(burners_in_use_ > 0 && "Cannot release non-used burner");

            if (!pending_handlers_.empty()) {
                net::post(io_, std::move(pending_handlers_.front()));
                pending_handlers_.pop_front();
            }
            else {
                --burners_in_use_;
            }
            });
    }

    int GetAvailableBurners() const {
        return number_of_burners_ - burners_in_use_.load();
    }

private:
    using Strand = net::strand<net::io_context::executor_type>;

    net::io_context& io_;
    Strand strand_;
    const int number_of_burners_;
    std::atomic<int> burners_in_use_{ 0 };
    std::deque<Handler> pending_handlers_;
};

class GasCookerLock {
public:
    GasCookerLock() = default;

    explicit GasCookerLock(std::shared_ptr<GasCooker> cooker) noexcept
        : cooker_{ std::move(cooker) } {
    }

    GasCookerLock(GasCookerLock&& other) noexcept
        : cooker_{ std::exchange(other.cooker_, nullptr) } {
    }

    GasCookerLock& operator=(GasCookerLock&& rhs) noexcept {
        if (this != &rhs) {
            Unlock();
            cooker_ = std::exchange(rhs.cooker_, nullptr);
        }
        return *this;
    }

    GasCookerLock(const GasCookerLock&) = delete;
    GasCookerLock& operator=(const GasCookerLock&) = delete;

    ~GasCookerLock() {
        Unlock();
    }

    void Unlock() noexcept {
        if (cooker_) {
            try {
                cooker_->ReleaseBurner();
            }
            catch (...) {
                // Suppress any exceptions during destruction
            }
            cooker_.reset();
        }
    }

    explicit operator bool() const noexcept {
        return cooker_ != nullptr;
    }

private:
    std::shared_ptr<GasCooker> cooker_;
};