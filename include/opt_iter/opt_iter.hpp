#ifndef OPT_ITER_OPT_ITER_HPP
#define OPT_ITER_OPT_ITER_HPP

#include <cassert>
#include <concepts>
#include <iterator>
#include <optional>
#include <type_traits>

namespace opt_iter
{
    /**
     * @brief Checks if a type is compatible to be std::optional-based iterator.
     *
     * @tparam T The type to be checked.
     * @tparam R The return type of the iterable.
     */
    template <typename T, typename R>
    concept OptIter = requires (T t, R u) {
        { t.next() } -> std::same_as<std::optional<R>>;

        requires std::move_constructible<R>;
        requires not std::is_reference_v<R>;
    };

    /**
     * @class Sentinel
     *
     * @brief A sentinel type to mark the end of the range.
     */
    struct Sentinel
    {
    };

    /**
     * @class Iterator
     *
     * @brief Wraps an optional-based iterator to be used in C++ functionalities that use C++ iterator model.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable.
     */
    template <typename T, typename R>
        requires OptIter<T, R>
    class [[nodiscard]] Iterator
    {
    public:
        using value_type        = R;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        Iterator() = delete;

        Iterator(T& t, std::optional<R>& storage)
            : m_t{ &t }
            , m_storage{ &storage }
        {
        }

        R&& operator*() const
        {
            assert(*m_storage != std::nullopt);
            return std::move(*m_storage).value();
        }

        Iterator& operator++()
        {
            *m_storage = std::move(m_t->next());
            return *this;
        }

        Iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Sentinel&) const { return *m_storage == std::nullopt; }

    private:
        T*                m_t       = nullptr;
        std::optional<R>* m_storage = nullptr;
    };

    /**
     * @class Range
     *
     * @brief Represents a range of optional-based iterable.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable.
     */
    template <typename T, typename R>
        requires OptIter<T, R>
    struct [[nodiscard]] Range
    {
        explicit Range(T& t)
            : m_t{ t }
            , m_storage{}
        {
            m_storage = std::move(m_t.next());
        }

        Iterator<T, R> begin() { return Iterator{ m_t, m_storage }; }
        Sentinel       end() { return Sentinel{}; }

        T&               m_t;
        std::optional<R> m_storage = std::nullopt;
    };

    /**
     * @brief Helper function to create a Range.
     *
     * @tparam U The return type of the iterable.
     * @tparam T The type of the iterable.
     *
     * @param t The iterable.
     *
     * @return Range.
     */
    template <typename R, typename T>
        requires OptIter<T, R>
    Range<std::remove_reference_t<T>, R> make(T&& t)
    {
        return Range<std::remove_reference_t<T>, R>{ t };
    }

#if __cpp_explicit_this_parameter
    /**
     * @class Generator
     *
     * @brief C++23 deducing-this-based CRTP helper.
     *
     * @tparam R The return type of the iterable.
     */
    template <typename R>
    class Generator
    {
    public:
        Generator() = default;

        template <typename Self>
        Iterator<std::remove_reference_t<Self>, R> begin(this Self& self)
        {
            static_assert(
                requires {
                    { self.next() } -> std::same_as<std::optional<R>>;
                }, "Generator must have a next() member function that returns std::optional<R>"
            );

            // fill the storage once only when the generator is started
            if (not self.m_started) {
                self.m_started = true;
                self.m_storage = std::move(self.next());
            }

            return Iterator{ self, self.m_storage };
        }

        Sentinel end() { return Sentinel{}; }

    private:
        std::optional<R> m_storage = std::nullopt;
        bool             m_started = false;
    };
#endif
}

#endif /* end of include guard: OPT_ITER_OPT_ITER_HPP */
