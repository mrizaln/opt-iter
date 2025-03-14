#include "opt_iter/opt_iter.hpp"

#include <print>
#include <random>
#include <ranges>

struct IntGen
{
    IntGen(std::mt19937* rng)
        : m_rng{ rng }
        , m_int_dist{ std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }
    {
    }

    std::optional<int> next()
    {
        // never returns a std::nullopt, infinite generator
        return m_int_dist(*m_rng);
    }

    std::mt19937*                      m_rng;
    std::uniform_int_distribution<int> m_int_dist;
};

// OptIter is required to be movable if it is to be wrapped by OwnedRange or OwnedRangeFunctor.
static_assert(std::movable<IntGen>);

int main()
{
    auto rng = std::mt19937{ std::random_device{}() };
    auto gen = opt_iter::make_owned<IntGen>(&rng);

    // using take
    std::println("\n> using take");
    for (auto v : gen | std::views::take(10)) {
        std::println("v = {}", v);
    }

    // using filter
    std::println("\n> using filter");
    auto is_even = [](auto&& v) { return v % 2 == 0; };
    for (auto v : gen | std::views::filter(is_even) | std::views::take(10)) {
        std::println("v = {}", v);
    }

    // using transform
    std::println("\n> using transform");
    auto negate = [](auto&& v) { return -v; };
    for (auto v : gen | std::views::transform(negate) | std::views::take(10)) {
        std::println("v = {}", v);
    }

    // collect into a vector
    std::println("\n> collect into vector");
    auto ten_int = gen | std::views::take(10) | std::ranges::to<std::vector>();

    // etc...
}
