#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <iostream>
#include <latch>
#include <mutex>
#include <syncstream>
#include <thread>
#include <unordered_set>
#include <vector>

#include "cafeteria.h"

using namespace std::literals;

namespace {

    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

    void PrintHotDogResult(const Result<HotDog>& result, Clock::duration order_duration) {
        std::osyncstream os{ std::cout };
        using namespace std::chrono;

        auto as_seconds = [](auto d) {
            return duration_cast<duration<double>>(d).count();
            };

        os << as_seconds(order_duration) << "> ";

        if (result.HasValue()) {
            auto& hot_dog = result.GetValue();
            os << "Hot dog #" << hot_dog.GetId()
                << ": bread bake time: " << as_seconds(hot_dog.GetBread().GetBakingDuration())
                << "s, sausage bake time: " << as_seconds(hot_dog.GetSausage().GetCookDuration()) << "s"
                << std::endl;
        }
        else {
            try {
                result.ThrowIfHoldsError();
            }
            catch (const std::exception& e) {
                os << "Error: " << e.what() << std::endl;
            }
            catch (...) {
                os << "Unknown error" << std::endl;
            }
        }
    }

    std::vector<HotDog> PrepareHotDogs(int num_orders, unsigned num_threads) {
        net::io_context io{ static_cast<int>(num_threads) };
        Cafeteria cafeteria{ io };

        std::mutex mut;
        std::vector<HotDog> hotdogs;

        const auto start_time = Clock::now();
        auto num_waiting_threads = std::min<int>(num_threads, num_orders);
        std::latch start{ num_waiting_threads };

        for (int i = 0; i < num_orders; ++i) {
            net::dispatch(io, [&cafeteria, &hotdogs, &mut, i, start_time, &start, num_waiting_threads] {
                std::osyncstream{ std::cout } << "Order #" << i << " is scheduled on thread #"
                    << std::this_thread::get_id() << std::endl;

                if (i < num_waiting_threads) {
                    start.arrive_and_wait();
                }

                cafeteria.OrderHotDog([&hotdogs, &mut, start_time](Result<HotDog> result) {
                    const auto duration = Clock::now() - start_time;
                    PrintHotDogResult(result, duration);
                    if (result.HasValue()) {
                        std::lock_guard lk{ mut };
                        hotdogs.emplace_back(std::move(result).GetValue());
                    }
                    });
                });
        }

        RunWorkers(num_threads, [&io] {
            io.run();
            });

        return hotdogs;
    }

    void VerifyHotDogs(const std::vector<HotDog>& hotdogs) {
        std::unordered_set<int> hotdog_ids;
        std::unordered_set<int> sausage_ids;
        std::unordered_set<int> bread_ids;

        for (auto& hotdog : hotdogs) {
            {
                auto [_, inserted] = hotdog_ids.insert(hotdog.GetId());
                if (!inserted) {
                    std::cerr << "Duplicate hotdog ID: " << hotdog.GetId() << std::endl;
                    std::abort();
                }
            }

            {
                auto [_, inserted] = sausage_ids.insert(hotdog.GetSausage().GetId());
                if (!inserted) {
                    std::cerr << "Duplicate sausage ID: " << hotdog.GetSausage().GetId()
                        << " in hotdog #" << hotdog.GetId() << std::endl;
                    std::abort();
                }
            }

            {
                auto [_, inserted] = bread_ids.insert(hotdog.GetBread().GetId());
                if (!inserted) {
                    std::cerr << "Duplicate bread ID: " << hotdog.GetBread().GetId()
                        << " in hotdog #" << hotdog.GetId() << std::endl;
                    std::abort();
                }
            }
        }
    }

} // namespace

int main() {
    using namespace std::chrono;

    constexpr unsigned num_threads = 4;
    constexpr int num_orders = 20;

    const auto start_time = Clock::now();
    auto hotdogs = PrepareHotDogs(num_orders, num_threads);
    const auto cook_duration = Clock::now() - start_time;

    std::cout << "Cook duration: " << duration_cast<duration<double>>(cook_duration).count() << 's'
        << std::endl;

    assert(hotdogs.size() == num_orders);
    assert(cook_duration >= 7s && cook_duration <= 7.5s);

    VerifyHotDogs(hotdogs);

    return 0; // Добавлен явный возврат из main()
}