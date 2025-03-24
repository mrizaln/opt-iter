#include "util.hpp"

#include "opt_iter/opt_iter.hpp"

#include <array>
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

#if not ENABLE_SPECIAL_MEMBER_FUNCTIONS
static_assert(std::is_trivial_v<Val>);
#endif

class RandGen
{
public:
    RandGen(std::mt19937& rng, std::size_t count)
        : m_rng{ rng }
        , m_int_dist{ std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }
        , m_float_dist{ 0.0f, 1.0f }
        , m_limit{ count }
    {
    }

    // using next() member function
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

template <std::size_t N, std::integral Index = std::size_t>
    requires (N > 0)
class FlatIndex
{
public:
    template <std::convertible_to<Index>... Ts>
        requires (sizeof...(Ts) == N)
    FlatIndex(Ts... dims)
        : m_dims{ static_cast<Index>(dims)... }
        , m_current{}
    {
    }

    std::optional<std::array<Index, N>> next()
    {
        if (m_current == m_dims) {
            return std::nullopt;
        }

        auto prev = m_current;

        for (auto i = 0u; i < N; ++i) {
            if (++m_current[i] >= m_dims[i]) {
                m_current[i] = 0;
            } else {
                return prev;
            }
        }

        m_current = m_dims;
        return prev;
    }

    void                 reset() { m_current = {}; }
    std::array<Index, N> dims() const { return m_dims; }
    static Index         size() { return N; }

private:
    std::array<Index, N> m_dims;
    std::array<Index, N> m_current;
};

template <typename... Ts>
FlatIndex(Ts...) -> FlatIndex<sizeof...(Ts)>;

static_assert(opt_iter::traits::HasNext<RandGen>);
static_assert(std::input_iterator<opt_iter::Iterator<RandGen, Val>>);
static_assert(std::ranges::range<opt_iter::Range<RandGen, Val>>);
static_assert(std::ranges::viewable_range<opt_iter::Range<RandGen, Val>>);

std::generator<Val> rand_gen_2(std::mt19937& rng, std::size_t limit)
{
    auto int_dist = std::uniform_int_distribution{
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max(),
    };
    auto float_dist = std::uniform_real_distribution<float>{
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max(),
    };

    auto count = 0u;
    while (count++ < limit) {
        co_yield Val{ int_dist(rng), float_dist(rng) };
    }
}

struct SeqUIntGen
{
    // using call operator
    std::optional<int> operator()()
    {
        if (m_value == std::numeric_limits<unsigned int>::max()) {
            return std::nullopt;
        }
        return m_value++;
    }

    unsigned int m_value = 0;
};
static_assert(opt_iter::traits::HasCallOp<SeqUIntGen>);

int main()
{
    auto num_iter = 2'000'000u;

    auto rng = std::mt19937{ std::random_device{}() };
    auto gen = RandGen{ rng, num_iter };

    auto [time1, size1] = util::time_repeated(10, [&] {
        auto vec = opt_iter::make(gen) | std::ranges::to<std::vector>();
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
        auto vec = rand_gen_2(rng, num_iter) | std::ranges::to<std::vector>();
        return vec.size();
    });
    std::println("using std::generator: {}, {}", time3, size3);

    // an owned range
    auto iter = opt_iter::make_owned<SeqUIntGen>();

    std::println("using new gen: {}", util::take_elipsis(iter, 20));
    std::println("using new gen: {}", util::take_elipsis(iter, 20));
    std::println("using new gen: {}", util::take_elipsis(iter, 20));
    std::println("using new gen: {}", util::take_elipsis(iter, 20));

    num_iter       = 100;
    auto flat_iter = FlatIndex{ num_iter, num_iter, num_iter };

    auto [time4, size4] = util::time_repeated(10, [&] {
        auto vec = opt_iter::make(flat_iter) | std::views::join | std::ranges::to<std::vector>();
        flat_iter.reset();
        return vec.size();
    });
    std::println("using opt_iter: {}, {}", time4, size4);

    auto [time5, size5] = util::time_repeated(10, [&] {
        auto vec = std::vector<std::size_t>();
        while (auto v = flat_iter.next()) {
            vec.insert(vec.end(), v->begin(), v->end());
        }
        flat_iter.reset();
        return vec.size();
    });
    std::println("using while loop: {}, {}", time5, size5);

    flat_iter.reset();

    for (auto [x, y, z] : opt_iter::make_owned<FlatIndex<3>>(3, 2, 3)) {
        std::println("({}, {}, {})", x, y, z);
    }

    return 0;
}
