#include <opt_iter/opt_iter.hpp>

#include <print>
#include <ranges>

int main()
{
    // auto int_seq = [i = 100] mutable { return std::optional{ i++ }; };
    // auto iter    = opt_iter::make(int_seq);
    //
    // std::println("first iteration");
    // for (auto v : iter | std::views::take(10)) {
    //     std::println(" v: {}", v);
    // }
    //
    // std::println("second iteration");
    // for (auto v : iter | std::views::take(10)) {
    //     std::println(" v: {}", v);
    // }

    // the call operator of the lambda must be mutable, but if it's mutable the lambda
    // can't be stored in OwnedRangeFn since it requires the type to be movable
    auto even_gen = [i = 0] mutable { return std::optional{ std::exchange(i, i + 2) }; };
    auto iter     = opt_iter::make(even_gen);

    for (auto v : iter | std::views::take(10)) {
        std::println(" v: {}", v);
    }
}
