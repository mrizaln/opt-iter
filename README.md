# opt-iter

Optional-based input range/generator wrapper for C++20 and above.

## Motivation

It's not uncommon to want to create a generator for a particular task and stop when some condition is met.

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

This might be sufficient for some people, but for some it's not--the type is not compatible with range-based for loop or any C++20 (or above) ranges library.

> using while loop

```cpp
// ...

std::string file_read(const std::filesystem::path& path);

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

This library attempts to provide wrappers for input range/generator pattern, similar to the one above that are compatible with range-based for loop and the C++20 ranges library. Creating the wrapper yourself is tedious due to the amount of boilerplate required and is error-prone, so providing an easy to use wrapper for this pattern is the main motivation of creating this library.

> using `opt-iter` range wrapper

```cpp
// ...

std::string file_read(const std::filesystem::path& path);

int main()
{
    auto string   = file_read(__FILE__);
    auto splitter = opt_iter::make_owned<StringSplitter>(string, '\n');

    for (auto [i, line] : splitter | std::views::enumerate) {
        std::println("{:>8} | {}", i + 1, line);
    }
}

```

The making of this library is inspired by the Rust iterator model. Rust, use `next()` method for acquiring the next iteration of an iterator. This makes it trivial to create a simple single-pass iterator like an input range/generator.

## Usage

This library uses `next()` member function as the function that generate the value, similar to Rust's iterator. But, since using `operator()` is common in creating generator pattern in C++ (as functor), I decided to support it as well. Furthermore, by allowing using `operator()` creating a short simple generator using lambda is trivial (see [example](#example)).

This library defines a concept `OptIter`` that checks whether a type is compatible with being wrapped by this library. The type produced by that type must satisfy `OptIterRet`.

```cpp
template <typename T>
concept OptIter = /* has next member function or a call operator that returns an optional */;

template <typename R>
concept OptIterRet = std::movable<R> and not std::is_reference_v<R>;
```

> see the definition in the [source file](include/opt_iter/opt_iter.hpp)

The main functionality of this library is provided by these wrapper types:

- `Range<T, R, bool>`: stores a pointer to the wrapped type with `next()` member function,
- `RangeFn<f, R, bool>`: stores a pointer to the wrapped type with `operator()`,
- `OwnedRange<T, R>`: owns the instance of the type with `next()` member function, and
- `OwnedRangeFn<F, R>`: owns the instance of the type with operator().

You can of construct the wrapper types yourself. But, using the helper functions is preferable.

- `opt_iter::make`

  This function wraps an already instantiated `OptIter` by constructing a `Range` or `RangeFn` depending on the `OptIter` kind: regular class/struct with `next()` member function for the former and a functor (or lambda) for the latter. This function will instantiate `Range` or `RangeFn` variation that allocate the storage for the returned by its own.

- `opt_iter::make_with`

  This function also wraps `OptIter` in a `Range` or `RangeFn`, but it requires you to provide the storage for the return value. The variation of `Range` or `RangeFn` will use that storage for the lifetime of it (including the iterators produced by it). You might want to use this one if you don't want to pay the cost of allocation.

- `opt_iter::make_owned`

  This function works like the previous function but it owns the `OptIter` itself. It constructs `OwnedRange` or `OwnedRangeFn` depending on the `OptIter` kind: regular class/struct with `next()` member function for the former and functor for the latter. Since it's already allocating the `OptIter`, I decided the storage for this one is in the heap as well, there's no real benefit of doing otherwise.

- `opt_iter::make_lambda`

  This function is a convenience function for creating a lambda-based generator. It creates `OwnedRangeFn`.

> `*Range*` needs to be `std::movable` to satisfy `std::ranges::viewable_range` so that it can be used with `std::views::*` functionalities. But, we need the storage to be static. So we can only use the heap or user provided storage to make the classes safe to use.


## Example

> typical use

```cpp
struct IntGen
{
    IntGen(std::mt19937* rng)
        : m_rng{ rng }
        , m_int_dist{ std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }
    {
    }

    std::optional<int> next()       // using next member function
    {
        // never returns a std::nullopt, infinite generator
        return m_int_dist(*m_rng);
    }

    std::mt19937*                      m_rng;
    std::uniform_int_distribution<int> m_int_dist;
};

// OptIter is required to be movable if it is to be wrapped by OwnedRange or OwnedRangeFn.
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

> short generator using lambda

```cpp
int main()
{
    // this is just a silly example
    // you can use std::views::iota and std::views::transform to get the same result

    // the call operator of the lambda must be mutable, but if it's mutable the lambda
    // can't be stored in OwnedRangeFn since it requires the type to be movable
    auto even = [i = 0] mutable { return std::optional{ std::exchange(i, i + 2) }; };
    auto iter = opt_iter::make(even);

    for (auto v : iter | std::views::take(10)) {
        std::println(" v: {}", v);
    }
}
```

## How does it work?

The `opt-iter` library wraps an `OptIter` type into a `Range`, `RangeFn`, `OwnedRange`, or `OwnedRangeFn` type (range wrapper type). These types have storage for the `OptIter::next()` return value. The storage is located in the heap since the range wrapper types need to be movable but the storage itself needs to be static (the location must not change even if the range wrapper instance is moved). To iterate this input range it needs an `Iterator` type which is returned by `begin()` member function. To mark the end of iterator (`std::nullopt` returned), `Sentinel` type is used.

> rough sketch of the `Range` type

```cpp
template <OptIter T, OptIterRet R>
struct Range
{
    T&             underlying();    // get the underlying OptIter
    void           clear();         // clear the storage
    Iterator<T, R> begin();
    Sentinel       end();

    T*                                m_t;
    std::unique_ptr<std::optional<T>> m_storage;
};
```

`Iterator` is responsible for the heavy lifting of the iteration process. `Iterator` can only produced by calling `begin()` on the range wrapper or from operating with existing `Iterator` instance. Each iterator has a pointer to the `OptIter` and the storage in the wrapper. Every time `operator++` is called it will call `OptIter::next` and store the result in the storage, overwriting it. The `operator*` then can be used to access the value.

> rough sketch of the `Iterator` type

```cpp
struct Sentinel {};

template <OptIter T, OptIterRet R>
struct Iterator
{
    R&&       operator*();
    Iterator& operator++();
    bool      operator==(const Sentinel&) const;

    T*                m_t;
    std::optional<T>* m_storage;
};
```

The fact that C++ iterator needs two distinct step to get to the next iteration means that at the start of the iteration, storage must be filled with the first value. This is done in the `begin()` call. This call will fill the storage by calling `OptIter::next()` function only and only if the storage is empty (`std::nullopt`).

## Limitation

The need to have a storage that is separate from the `OptIter` itself means maintaining a correct invariant requires some care from the user. For example, if you change the state of the `OptIter` after the `begin()` call, the storage will be out of sync with the `OptIter` itself. This is why the `Range` type has a `clear()` member function to clear the storage. This is useful when you want to reset the iteration process.

```cpp
struct IntSeq
{
    std::optional<int> next() { return m_value++; } // infinite generator
    void               reset() { m_value = 0; }

    int m_value = 0;
};

int main()
{
    namespace sv = std::views;
    namespace sr = std::ranges;

    // storage out of sync
    {
        auto seq   = opt_iter::make_owned<IntSeq>();

        auto begin = seq.begin();               // begin() will fill the storage by calling next() beforehand
        seq.underlying().reset();               // but the user decided to reset the state of the OptIter

        auto values = seq | sv::take(10) | sr::to<std::vector>();     // the values will be out of sync!

        // values now contains: [0, 0, 1, 2, 3, 4, 5, 6, 7, 8]
        // instead of           [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        // which might be not what the user expected
    }

    // fix: clear the storage first before using the wrapper again
    {
        auto seq = opt_iter::make_owned<IntSeq>();

        auto begin = seq.begin();
        seq.underlying().reset();
        seq.clear();

        auto values = seq | sv::take(10) | sr::to<std::vector>();
    }
}
```
