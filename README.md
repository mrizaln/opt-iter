# opt-iter

Optional-based input range wrapper for C++20 and above.

## Motivation

It's not uncommon to want to create a generator for a particular task and stop when condition is met. Traditionally the generator in C++ is defined as a functor like this.

```cpp
class StringSplitter
{
public:
    StringSplitter(std::string_view str, char delim = ' ')
        : m_str{ str }
        , m_delim{ delim }
    {
    }

    std::optional<std::string_view> operator()()
    {
        if (m_pos == std::string_view::npos) {
            return std::nullopt;
        }

        auto next_pos = m_str.find(m_delim, m_pos);
        auto result   = m_str.substr(m_pos, next_pos - m_pos);
        m_pos         = next_pos == std::string_view::npos ? next_pos : next_pos + 1;

        return result;
    }

    void        reset() { m_pos = 0; }
    std::size_t pos() const { return m_pos; }

private:
    std::string_view m_str;
    std::size_t      m_pos = 0;
    char             m_delim;
};
```

This might be sufficient for some people, but for some, this will introduce a clunky syntax since it's not compatible with range-based for loop.

```cpp
// ...

std::string_view load_string();

int main()
{
    auto string   = file_read(__FILE__);
    auto splitter = StringSplitter{ string, '\n' };

    auto i = 0uz;
    while (auto line = splitter()) {
        std::println("{:>8} | {}", ++i, *line);
    }
}

```

This library provides a way to wrap a generator that is compatible with range-based for loop and C++20 ranges library.

```cpp
// ...

std::string file_read(const std::filesystem::path& path);

int main()
{
    auto string   = file_read(__FILE__);
    auto splitter = opt_iter::make_owned(string, '\n');

    for (auto [i, line] : splitter | std::views::enumerate) {
        std::println("{:>8} | {}", i + 1, line);
    }
}

```

## Usage

This library considers a type that is compatible with this library as `OptIter`.

There are two essential functions for this library.

- `opt_iter::make`
  This function wraps an already instantiated `OptIter` which returns a `Range` or `RangeFunctor` depending on the `OptIter` kind, class/struct with `next()` member function for the former and a functor for the latter.

- `opt_iter::make_owned`
  This function works like the above function but it owns the `OptIter` itself. It returns `OwnedRange` or `OwnedRangeFunctor` depeding on the `OptIter` kind, class/struct with `next()` member function for the former and functor for the latter.
  > OptIter is required to be movable if it is to be wrapped by `OwnedRange` or `OwnedRangeFunctor`. This requirement is derived from the fact that to make `OwnedRange` and `OwnedRangeFunctor` fulfill `std::ranges::viewable_range` so that it can be used with `std::views::*` functionalities.

**_Why use `next()` member function instead of `operator()` function for the main functionality that returns the value?_** you might ask. Well, I steal this idea from Rust iterator model which uses `next()` function that returns an `Opt`. Also having a named function is just better in my opinion instead of using an unnamed function like `operator()`. But since using `operator()` is still common in creating generator (as functor), I decided to support it as well.

In the first place I created this library to mimic the Rust iterator model since it is easier to write especially for simple tasks. This is in contrast to C++ iterator model which is hard to implement and require a lot of boilerplate. Writing an random integer generator that works well with C++ range-based for loop and ranges library for example is as simple as creating a class with `next()` member function that returns an `std::optional`

```cpp
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
```
