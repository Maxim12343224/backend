#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono> // Добавлено
#include <thread> // Добавлено

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;

using namespace std::chrono_literals; // Добавлено для поддержки литералов времени

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{ io } {
    }

    void OrderHotDog(HotDogHandler handler) {
        struct CookState {
            CookState(int hot_dog_id, HotDogHandler handler, net::io_context& io,
                std::shared_ptr<Bread> bread, std::shared_ptr<Sausage> sausage)
                : hot_dog_id{ hot_dog_id }
                , handler{ std::move(handler) }
                , io{ io }
                , bread{ std::move(bread) }
                , sausage{ std::move(sausage) } {
            }

            int hot_dog_id;
            HotDogHandler handler;
            net::io_context& io;
            std::shared_ptr<Bread> bread;
            std::shared_ptr<Sausage> sausage;
            std::atomic<int> count{ 0 };
            std::mutex mut;
            std::exception_ptr error;
        };

        int hot_dog_id = next_hotdog_id_++;
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        auto state_ptr = std::make_shared<CookState>(hot_dog_id, std::move(handler), io_,
            std::move(bread), std::move(sausage));

        auto TryAssembleHotDog = [](std::shared_ptr<CookState> state_ptr) {
            std::exception_ptr error;
            {
                std::lock_guard lk(state_ptr->mut);
                error = state_ptr->error;
            }
            if (error) {
                net::post(state_ptr->io, [handler = state_ptr->handler, error] {
                    handler(Result<HotDog>(error));
                    });
            }
            else {
                try {
                    HotDog hot_dog(state_ptr->hot_dog_id, state_ptr->sausage, state_ptr->bread);
                    net::post(state_ptr->io, [handler = state_ptr->handler,
                        hot_dog = std::move(hot_dog)]() mutable {
                            handler(std::move(hot_dog));
                        });
                }
                catch (...) {
                    net::post(state_ptr->io, [handler = state_ptr->handler,
                        e = std::current_exception()] {
                            handler(Result<HotDog>(e));
                        });
                }
            }
            };

        // Start cooking sausage
        state_ptr->sausage->StartFry(*gas_cooker_, [state_ptr, TryAssembleHotDog] {
            auto timer = std::make_shared<net::steady_timer>(state_ptr->io, 1500ms);
            timer->async_wait([state_ptr, timer, TryAssembleHotDog](sys::error_code ec) {
                if (ec) {
                    return;
                }
                try {
                    state_ptr->sausage->StopFry();
                }
                catch (...) {
                    std::lock_guard lk(state_ptr->mut);
                    if (!state_ptr->error) {
                        state_ptr->error = std::current_exception();
                    }
                }
                if (++state_ptr->count == 2) {
                    TryAssembleHotDog(state_ptr);
                }
                });
            });

        // Start baking bread
        state_ptr->bread->StartBake(*gas_cooker_, [state_ptr, TryAssembleHotDog] {
            auto timer = std::make_shared<net::steady_timer>(state_ptr->io, 1000ms);
            timer->async_wait([state_ptr, timer, TryAssembleHotDog](sys::error_code ec) {
                if (ec) {
                    return;
                }
                try {
                    state_ptr->bread->StopBaking();
                }
                catch (...) {
                    std::lock_guard lk(state_ptr->mut);
                    if (!state_ptr->error) {
                        state_ptr->error = std::current_exception();
                    }
                }
                if (++state_ptr->count == 2) {
                    TryAssembleHotDog(state_ptr);
                }
                });
            });
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    std::atomic<int> next_hotdog_id_{ 1 };
};