#include "util.hpp"

#include "opt_iter/opt_iter.hpp"

#include <generator>
#include <iterator>
#include <limits>
#include <print>
#include <random>
#include <ranges>

#define ENABLE_SPECIAL_MEMBER_FUNCTIONS 0

struct Val
{
#if ENABLE_SPECIAL_MEMBER_FUNCTIONS
    Val(int val_int, float val_float)
        : m_int{ val_int }
        , m_float{ val_float }
    {
    }

    Val(Val&& val) noexcept
        : m_int{ val.m_int }
        , m_float{ val.m_float }
    {
        std::println(" (&&){{{}, {}}}", m_int, m_float);
    };

    Val& operator=(Val&& val) noexcept
    {
        m_int   = val.m_int;
        m_float = val.m_float;
        std::println("=(&&){{{}, {}}}", m_int, m_float);
        return *this;
    };

    Val(const Val& val) noexcept
        : m_int{ val.m_int }
        , m_float{ val.m_float }
    {
        std::println(" (c&){{{}, {}}}", m_int, m_float);
    };

    Val& operator=(const Val& val) noexcept
    {
        m_int   = val.m_int;
        m_float = val.m_float;
        std::println("=(c&){{{}, {}}}", m_int, m_float);
        return *this;
    };
#endif

    int   m_int;
    float m_float;
};

class Generator
{
public:
    Generator(std::mt19937& rng, std::size_t count)
        : m_rng{ rng }
        , m_int_dist{ std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }
        , m_float_dist{ 0.0f, 1.0f }
        , m_limit{ count }
    {
    }

    std::optional<Val> next()
    {
        if (m_count++ >= m_limit) {
            return std::nullopt;
        }

        return Val{ m_int_dist(m_rng), m_float_dist(m_rng) };
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
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max(),
    };
    auto float_dist = std::uniform_real_distribution<float>{
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max(),
    };

    auto count = 0;
    while (count++ < limit) {
        co_yield Val{ int_dist(rng), float_dist(rng) };
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

int main()
{
    constexpr auto num_iter = 1'000'000;

    auto rng = std::mt19937{ std::random_device{}() };
    auto gen = Generator{ rng, num_iter };

    auto [time1, size1] = util::time_repeated(10, [&] {
        auto vec = opt_iter::make<Val>(gen) | std::ranges::to<std::vector>();
        gen.reset();
        return vec.size();
    });
    std::println("using opt_iter: {}, {}", time1, size1);

    auto [time2, size2] = util::time_repeated(10, [&] {
        auto vec = std::vector<Val>();
        while (auto v = gen.next()) {
            vec.push_back(std::move(v).value());
        }
        gen.reset();
        return vec.size();
    });
    std::println("using while loop: {}, {}", time2, size2);

    gen.reset();

    auto [time3, size3] = util::time_repeated(10, [&] {
        auto vec = generator_2(rng, num_iter) | std::ranges::to<std::vector>();
        return vec.size();
    });
    std::println("using std::generator: {}, {}", time3, size3);

    auto new_gen = NewGen{ num_iter };

    std::println("using new gen: {}", util::take_elipsis(new_gen, 20));
    std::println("using new gen: {}", util::take_elipsis(new_gen, 20));
    std::println("using new gen: {}", util::take_elipsis(new_gen, 20));
    std::println("using new gen: {}", util::take_elipsis(new_gen, 20));

    return 0;
}
