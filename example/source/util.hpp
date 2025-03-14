#pragma once

#include <chrono>
#include <format>
#include <ranges>

namespace util
{
    using Ms = std::chrono::duration<double, std::milli>;

    Ms to_ms(auto dur)
    {
        return std::chrono::duration_cast<Ms>(dur);
    }

    template <std::ranges::viewable_range R>
    struct TakeElipsis
    {
        R&&         m_range;
        std::size_t m_limit;
    };

    template <std::ranges::viewable_range R>
    TakeElipsis<R> take_elipsis(R&& range, std::size_t limit = std::numeric_limits<std::size_t>::max())
    {
        return TakeElipsis<R>{ std::forward<R>(range), limit };
    }

    std::pair<Ms, std::size_t> time_repeated(std::size_t count, std::invocable auto fn)
    {
        using Clock = std::chrono::steady_clock;

        auto duration = Ms{ 0 };
        auto size     = 0uz;

        // warmup
        for (auto _ : std::views::iota(0uz, 3uz)) {
            size += fn();
        }

        size = 0uz;

        for (auto _ : std::views::iota(0uz, count)) {
            auto start  = Clock::now();
            size       += fn();
            duration   += to_ms(Clock::now() - start);
        }

        return { duration / count, size };
    }
}

template <std::ranges::viewable_range R>
struct std::formatter<util::TakeElipsis<R>>
{
    std::formatter<std::ranges::range_value_t<R>> m_elem_fmt;

    constexpr format_parse_context::iterator parse(format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end()) {
            throw std::format_error{ "empty format string, you probably missing '}'" };
        }

        switch (*it) {
        case '}': return it;
        case ':': {
            ctx.advance_to(++it);
            return m_elem_fmt.parse(ctx);
        }
        default: throw std::format_error{ "invalid format, should be '}' or ':'" };
        };
    }

    format_context::iterator format(const util::TakeElipsis<R>& take, format_context& ctx) const
    {
        std::format_to(ctx.out(), "[");

        auto limit = take.m_limit;
        auto end   = std::ranges::end(take.m_range);
        auto first = std::ranges::begin(take.m_range);

        if (first != end) {
            m_elem_fmt.format(*first, ctx);
            ++first;
            --limit;

            while (first != end && limit > 0) {
                std::format_to(ctx.out(), ", ");
                m_elem_fmt.format(*first, ctx);
                ++first;
                --limit;
            }
        }

        if (first != end) {
            std::format_to(ctx.out(), ", ...]");
        } else {
            std::format_to(ctx.out(), "]");
        }

        return ctx.out();
    }
};
