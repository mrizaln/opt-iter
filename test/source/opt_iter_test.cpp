#include <opt_iter/opt_iter.hpp>

#include <boost/ut.hpp>
#include <fmt/base.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <concepts>
#include <optional>
#include <ranges>
#include <vector>

namespace ut = boost::ut;
namespace sr = std::ranges;
namespace sv = std::views;

class IntSeq
{
public:
    IntSeq(int limit)
        : m_limit{ limit }
    {
    }

    std::optional<int> next()
    {
        if (m_value >= m_limit) {
            return std::nullopt;
        }
        return m_value++;
    }

    void reset() { m_value = 0; }

private:
    int m_value = 0;
    int m_limit = 0;
};

class IntSeq2
{
public:
    IntSeq2(int limit)
        : m_limit{ limit }
    {
    }

    std::optional<int> operator()()
    {
        if (m_value >= m_limit) {
            return std::nullopt;
        }
        return m_value++;
    }

    void reset() { m_value = 0; }

private:
    int m_value = 0;
    int m_limit = 0;
};

class IntSeq3
{
public:
    IntSeq3(int limit)
        : m_limit{ limit }
    {
    }

    std::optional<int> next()
    {
        if (m_value >= m_limit) {
            return std::nullopt;
        }
        return m_value++;
    }

    std::optional<int> operator()()
    {
        if (m_value >= m_limit) {
            return std::nullopt;
        }
        return m_value++;
    }

    void reset() { m_value = 0; }

private:
    int m_value = 0;
    int m_limit = 0;
};

// I need to use this since the paramterized tests for type provided by ut by default require the type to be
// default-initializable and copyable
template <typename Tuple, typename Fn>
constexpr void for_each_tuple(Fn&& fn)
{
    const auto handler = [&]<std::size_t... I>(std::index_sequence<I...>) {
        (fn.template operator()<std::tuple_element_t<I, Tuple>>(), ...);
    };
    handler(std::make_index_sequence<std::tuple_size_v<Tuple>>());
}

namespace std
{
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const std::pair<T, T>& pair)
    {
        return os << "(" << pair.first << ", " << pair.second << ")";
    }
}

int main()
{
    using ut::expect, ut::that;
    using namespace ut::literals;
    using namespace ut::operators;

    "IntSeq, IntSeq2, and IntSeq3 should be considered as satisfying OptIter concept"_test = [] {
        static_assert(opt_iter::OptIter<IntSeq>);
        static_assert(opt_iter::traits::HasNext<IntSeq>);
        static_assert(not opt_iter::traits::HasNext<IntSeq2>);

        static_assert(opt_iter::OptIter<IntSeq2>);
        static_assert(not opt_iter::traits::HasCallOp<IntSeq>);
        static_assert(opt_iter::traits::HasCallOp<IntSeq2>);

        static_assert(opt_iter::OptIter<IntSeq3>);
        static_assert(opt_iter::traits::HasNext<IntSeq3>);
        static_assert(opt_iter::traits::HasCallOp<IntSeq3>);
    };

    "Iterator for IntSeq should satisfy input iterator concept"_test = [] {
        using Iterator = opt_iter::Iterator<IntSeq, int>;
        static_assert(std::input_iterator<Iterator>);
    };

    "Iterator for IntSeq2 should satisfy input iterator only after wrapping it in FnWrapper"_test = [] {
        // > fail to compile, which is the result I want
        // using Iterator = opt_iter::Iterator<IntSeq2, int>;

        using Wrapper  = opt_iter::FnWrapper<IntSeq2, int>;
        using Iterator = opt_iter::Iterator<Wrapper, int>;
        static_assert(std::input_iterator<Iterator>);
    };

    "Range and RangeFn should satisfy input range and viewable range concept"_test = [] {
        using Range = opt_iter::Range<IntSeq, int>;
        static_assert(std::ranges::range<Range>);
        static_assert(std::ranges::input_range<Range>);
        static_assert(std::ranges::viewable_range<Range>);

        using RangeFn = opt_iter::RangeFn<IntSeq2, int>;
        static_assert(std::ranges::range<Range>);
        static_assert(std::ranges::input_range<RangeFn>);
        static_assert(std::ranges::viewable_range<RangeFn>);

        // > these should fail to compile
        // using Range2        = opt_iter::Range<IntSeq2, int>;
        // using RangeFn2 = opt_iter::RangeFn<IntSeq, int>;
    };

    "Range and RangeFn shold be able to be constructed for IntSeq and IntSeq2"_test = [] {
        using Range = opt_iter::Range<IntSeq, int>;
        static_assert(std::constructible_from<Range, IntSeq&>);

        using RangeFn = opt_iter::RangeFn<IntSeq2, int>;
        static_assert(std::constructible_from<RangeFn, IntSeq2&>);

        // > these should fail to compile
        // using Range2        = opt_iter::Range<IntSeq2, int>;
        // using RangeFn2 = opt_iter::RangeFn<IntSeq, int>;
    };

    "OwnedRange and OwnedRangeFn should satisfy input range and viewable range concept"_test = [] {
        using OwnedRange = opt_iter::OwnedRange<IntSeq, int>;
        static_assert(std::ranges::range<OwnedRange>);
        static_assert(std::ranges::input_range<OwnedRange>);
        static_assert(std::ranges::viewable_range<OwnedRange>);

        using OwnedRangeFn = opt_iter::OwnedRangeFn<IntSeq2, int>;
        static_assert(std::ranges::range<OwnedRange>);
        static_assert(std::ranges::input_range<OwnedRangeFn>);
        static_assert(std::ranges::viewable_range<OwnedRangeFn>);

        // > these should fail to compile
        // using OwnedRange2        = opt_iter::OwnedRange<IntSeq2, int>;
        // using OwnedRangeFn2 = opt_iter::OwnedRangeFn<IntSeq, int>;
    };

    "OwnedRange and OwnedRangeFn shold be able to be constructed for IntSeq and IntSeq2"_test = [] {
        using OwnedRange = opt_iter::OwnedRange<IntSeq, int>;
        static_assert(std::constructible_from<OwnedRange, IntSeq&&>);

        using OwnedRangeFn = opt_iter::OwnedRangeFn<IntSeq2, int>;
        static_assert(std::constructible_from<OwnedRangeFn, IntSeq2&&>);

        // > these should fail to compile
        // using OwnedRange2        = opt_iter::OwnedRange<IntSeq2, int>;
        // using OwnedRangeFn2 = opt_iter::OwnedRangeFn<IntSeq, int>;
    };

    "make should construct Range/RangeFn depending on next()/operator()() exists"_test = [] {
        auto int_seq = IntSeq{ 5 };
        auto range   = opt_iter::make(int_seq);
        static_assert(std::same_as<decltype(range), opt_iter::Range<IntSeq, int>>);

        auto int_seq2 = IntSeq2{ 5 };
        auto range2   = opt_iter::make(int_seq2);
        static_assert(std::same_as<decltype(range2), opt_iter::RangeFn<IntSeq2, int>>);
    };

    "make_owned should construct OwnedRange/OwnedRangeFn depending on next()/operator()() exists"_test = [] {
        auto range = opt_iter::make_owned<IntSeq>(5);
        static_assert(std::same_as<decltype(range), opt_iter::OwnedRange<IntSeq, int>>);

        auto range2 = opt_iter::make_owned<IntSeq2>(5);
        static_assert(std::same_as<decltype(range2), opt_iter::OwnedRangeFn<IntSeq2, int>>);
    };

    "make should prioritize next() member function over operator()"_test = [] {
        auto int_seq3 = IntSeq3{ 5 };
        auto range    = opt_iter::make(int_seq3);
        static_assert(std::same_as<decltype(range), opt_iter::Range<IntSeq3, int>>);
    };

    "make_owned should prioritize next() member function over operator()"_test = [] {
        auto range = opt_iter::make_owned<IntSeq3>(5);
        static_assert(std::same_as<decltype(range), opt_iter::OwnedRange<IntSeq3, int>>);
    };

    "RangeFn and OwnedRangeFn should still be able to be constructed for IntSeq3"_test = [] {
        static_assert(std::constructible_from<opt_iter::RangeFn<IntSeq3, int>, IntSeq3&>);
        static_assert(std::constructible_from<opt_iter::OwnedRangeFn<IntSeq3, int>, IntSeq3&&>);
    };

    auto int_seq  = IntSeq{ 100 };
    auto int_seq2 = IntSeq2{ 100 };

    auto range  = opt_iter::make(int_seq);
    auto range2 = opt_iter::make(int_seq2);
    auto owned  = opt_iter::make_owned<IntSeq>(100);
    auto owned2 = opt_iter::make_owned<IntSeq2>(100);

    "*Range* should be compatible with C++ iterator and ranges library"_test = []<typename R>(R range) {
        range->underlying().reset();
        "*Range* should be able to be used in range-based for loop and has expected output"_test = [&] {
            const auto expected = sv::iota(0, 100) | sr::to<std::vector>();
            auto       actual   = std::vector<int>{};
            for (auto v : *range) {
                actual.push_back(v);
            }
            expect(that % actual == expected);
        };

        range->underlying().reset();
        "*Range* can be collected to a container"_test = [&] {
            const auto expected = sv::iota(0, 100) | sr::to<std::vector>();
            const auto actual   = *range | sr::to<std::vector>();
            expect(that % actual == expected);
        };

        range->underlying().reset();
        "*Range* should be able to use enumerate views"_test = [&] {
            using Pair          = std::pair<int, int>;
            const auto expected = sv::iota(0, 100) | sv::enumerate | sr::to<std::vector<Pair>>();
            auto       actual   = *range | sv::enumerate | sr::to<std::vector<Pair>>();
            expect(that % actual == expected);
        };

        range->underlying().reset();
        "*Range* should be able to use filter views"_test = [&] {
            const auto is_even  = [](int v) { return v % 2 == 0; };
            const auto expected = sv::iota(0, 100) | sv::filter(is_even) | sr::to<std::vector>();
            auto       actual   = *range | sv::filter(is_even) | sr::to<std::vector>();
            expect(that % actual == expected);
        };

        range->underlying().reset();
        "*Range* should be able to use take views repeatedly and has expected output"_test = [&] {
            const auto expected_1 = sv::iota(0, 100) | sv::drop(0) | sv::take(10) | sr::to<std::vector>();
            const auto expected_2 = sv::iota(0, 100) | sv::drop(10) | sv::take(10) | sr::to<std::vector>();
            const auto expected_3 = sv::iota(0, 100) | sv::drop(20) | sv::take(10) | sr::to<std::vector>();

            // remember, the range is an input iterator, means it's single-pass (the iteration is stateful)
            const auto actual_1 = *range | sv::take(10) | sr::to<std::vector>();
            const auto actual_2 = *range | sv::take(10) | sr::to<std::vector>();
            const auto actual_3 = *range | sv::take(10) | sr::to<std::vector>();

            expect(that % actual_1 == expected_1);
            expect(that % actual_2 == expected_2);
            expect(that % actual_3 == expected_3);
        };
    } | std::tuple{ &range, &range2, &owned, &owned2 };
}
