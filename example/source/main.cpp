#include "opt_iter/opt_iter.hpp"

#include <chrono>
#include <iterator>
#include <limits>
#include <random>
#include <ranges>
#include <vector>
#include <generator>
#include <print>

namespace sr = std::ranges;
namespace sv = std::views;

using Ms = std::chrono::duration<double, std::milli>;

Ms to_ms(auto dur)
{
    return std::chrono::duration_cast<Ms>(dur);
}

struct Val
{
    int   m_int;
    float m_float;
};

class Generator
{
public:
    Generator(std::mt19937& rng, std::size_t count)
        : m_rng{ rng }
        , m_int_dist{ std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max() }
        , m_float_dist{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max() }
        , m_limit{ count }
    {
    }

    std::optional<Val> next()
    {
        if (m_count++ >= m_limit) {
            return std::nullopt;
        }

        return Val{
            .m_int   = m_int_dist(m_rng),
            .m_float = m_float_dist(m_rng),
        };
    }

    void reset() { m_count = 0; }

private:
    std::mt19937&                         m_rng;
    std::uniform_int_distribution<int>    m_int_dist;
    std::uniform_real_distribution<float> m_float_dist;

    std::size_t m_count = 0;
    std::size_t m_limit = 0;
};

std::generator<Val> generator_2(std::mt19937& rng, int limit)
{
    auto int_dist = std::uniform_int_distribution{
        std::numeric_limits<int>::lowest(),
        std::numeric_limits<int>::max(),
    };
    auto float_dist = std::uniform_real_distribution<float>{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::max(),
    };

    auto count = 0;
    while (count++ < limit) {
        co_yield Val{
            .m_int   = int_dist(rng),
            .m_float = float_dist(rng),
        };
    }
}

struct NewGen : public opt_iter::Generator<int>
{
    NewGen(int limit)
        : m_limit{ limit }
    {
    }

    std::optional<int> next()
    {
        if (m_count >= m_limit) {
            return std::nullopt;
        }
        return m_count++;
    }

    int m_limit = 0;
    int m_count = 0;
};

static_assert(std::input_iterator<opt_iter::Iterator<Generator, Val>>);
static_assert(std::ranges::range<opt_iter::Range<Generator, Val>>);

std::pair<Ms, std::size_t> time_repeated(std::size_t count, std::invocable auto fn)
{
    using Clock = std::chrono::steady_clock;

    auto duration = Ms{ 0 };
    auto size     = 0uz;

    // warmup
    for (auto _ : sv::iota(0uz, 3uz)) {
        size += fn();
    }

    size = 0uz;

    for (auto _ : sv::iota(0uz, count)) {
        auto start  = Clock::now();
        size       += fn();
        duration   += to_ms(Clock::now() - start);
    }

    return { duration / count, size };
}

int main()
{
    auto rng = std::mt19937{ std::random_device{}() };
    auto gen = Generator{ rng, 10000 };

    auto [time1, size1] = time_repeated(10, [&] {
        auto vec = opt_iter::make<Val>(gen) | sr::to<std::vector>();
        gen.reset();
        return vec.size();
    });
    std::println("using opt_iter: {}, {}", time1, size1);

    auto [time2, size2] = time_repeated(10, [&] {
        auto vec = std::vector<Val>();
        while (auto v = gen.next()) {
            vec.push_back(*v);
        }
        gen.reset();
        return vec.size();
    });
    std::println("using while loop: {}, {}", time2, size2);

    gen.reset();

    auto [time3, size3] = time_repeated(10, [&] {
        auto vec = generator_2(rng, 10000) | sr::to<std::vector>();
        return vec.size();
    });
    std::println("using std::generator: {}, {}", time3, size3);

    auto new_gen = NewGen{ 20 };
    for (auto v : new_gen) {
        std::print("{}, ", v);
    }
    std::println();

    return 0;
}
