#include <opt_iter/opt_iter.hpp>

#include <print>
#include <ranges>

int main()
{
    auto int_seq = opt_iter::make_lambda([i = 0uz] mutable -> std::optional<std::size_t> { return i++; });

    std::println("int_seq first iteration");
    for (auto v : int_seq | std::views::take(10)) {
        std::println(" v: {}", v);
    }

    std::println("int_seq second iteration");
    for (auto v : int_seq | std::views::take(10)) {
        std::println(" v: {}", v);
    }

    std::println("fibonacci");
    auto fibonacci = opt_iter::make_lambda([i = 0uz, j = 1uz] mutable -> std::optional<std::size_t> {
        return std::exchange(i, std::exchange(j, i + j));
    });
    for (auto v : fibonacci | std::views::take(20)) {
        std::println(" v: {}", v);
    }

    std::println("teens");
    auto teens = opt_iter::make_lambda([i = 13uz] mutable -> std::optional<std::size_t> {
        return i < 20 ? std::optional{ i++ } : std::nullopt;
    });
    for (auto v : teens) {
        std::println(" v: {}", v);
    }
}
