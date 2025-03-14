#ifndef OPT_ITER_OPT_ITER_HPP
#define OPT_ITER_OPT_ITER_HPP

#include "traits.hpp"

#include <cassert>
#include <concepts>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace opt_iter
{
    /**
     * @brief Checks if a type is compatible to be std::optional-based iterator.
     *
     * @tparam T The type to be checked.
     * @tparam R The return type of the iterable (unwrapped).
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
    struct [[nodiscard]] Sentinel
    {
    };

    /**
     * @class Iterator
     *
     * @brief Wraps an optional-based iterator to be used in C++ functionalities that use C++ iterator model.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable (unwrapped).
     */
    template <typename T, typename R>
        requires OptIter<T, R>
    class [[nodiscard]] Iterator
    {
    public:
        using value_type        = R;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        Iterator()                                 = default;
        Iterator(const Iterator& other)            = default;
        Iterator& operator=(const Iterator& other) = default;

        Iterator(Iterator&& other) noexcept
            : m_t{ std::exchange(other.m_t, nullptr) }
            , m_storage{ std::exchange(other.m_storage, nullptr) }
        {
        }

        Iterator& operator=(Iterator&& other) noexcept
        {
            m_t       = std::exchange(other.m_t, nullptr);
            m_storage = std::exchange(other.m_storage, nullptr);
            return *this;
        }

        Iterator(T* t, std::optional<R>* storage)
            : m_t{ t }
            , m_storage{ storage }
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

        friend bool operator==(const Iterator& it, const Sentinel&)
        {
            return !it.m_storage || *it.m_storage == std::nullopt;
        }

        friend bool operator==(const Sentinel&, const Iterator& it) { return it == Sentinel{}; }

    private:
        T*                m_t       = nullptr;
        std::optional<R>* m_storage = nullptr;
    };

    /**
     * @class FunctorWrapper
     *
     * @brief Wraps a functor to be compatible with opt_iter functionalities.
     *
     * @tparam F The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     */
    template <typename F, typename R>
        requires traits::HasCallOp<F> and std::same_as<typename traits::OptIterTrait<F>::Ret, R>
    struct [[nodiscard]] FunctorWrapper
    {
        std::optional<R> next()
        {
            assert(m_f != nullptr);
            return m_f->operator()();
        }

        F* m_f;
    };

    /**
     * @class Range
     *
     * @brief Represents a range of optional-based iterable.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable (unwrapped).
     */
    template <typename T, typename R>
        requires OptIter<T, R>
    class [[nodiscard]] Range
    {
    public:
        explicit Range(T& t)
            : m_t{ &t }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
            *m_storage = std::move(m_t->next());
        }

        T& underlying() const
        {
            assert(m_t != nullptr);
            return *m_t;
        }

        Iterator<T, R> begin() { return Iterator{ m_t, m_storage.get() }; }
        Sentinel       end() { return Sentinel{}; }

    private:
        T*                                m_t       = nullptr;
        std::unique_ptr<std::optional<R>> m_storage = nullptr;
    };

    /**
     * @class FunctorWrapper
     *
     * @brief Wraps a functor to be compatible with opt_iter functionalities.
     *
     * @tparam T The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     */
    template <typename F, typename R>
        requires traits::HasCallOp<F> and std::same_as<typename traits::OptIterTrait<F>::Ret, R>
    class [[nodiscard]] RangeFunctor
    {
    public:
        explicit RangeFunctor(F& f)
            : m_wrapper{ &f }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
            *m_storage = std::move(m_wrapper.next());
        }

        F& underlying() const
        {
            assert(m_wrapper.m_f != nullptr);
            return *m_wrapper.m_f;
        }

        Iterator<FunctorWrapper<F, R>, R> begin() { return Iterator{ &m_wrapper, m_storage.get() }; }
        Sentinel                          end() { return Sentinel{}; }

    private:
        FunctorWrapper<F, R>              m_wrapper;
        std::unique_ptr<std::optional<R>> m_storage = nullptr;
    };

    /**
     * @class OwnedRange
     *
     * @brief Represents a range of optional-based iterable while owning the iterable.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable (unwrapped).
     */
    template <typename T, typename R>
        requires std::movable<T> and OptIter<T, R>
    class [[nodiscard]] OwnedRange
    {
    public:
        explicit OwnedRange(T&& t)
            : m_t{ std::move(t) }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
            *m_storage = std::move(m_t.next());
        }

        T&       underlying() { return m_t; }
        const T& underlying() const { return m_t; }

        Iterator<T, R> begin() { return Iterator{ &m_t, m_storage.get() }; }
        Sentinel       end() { return Sentinel{}; }

    private:
        T                                 m_t;
        std::unique_ptr<std::optional<R>> m_storage = nullptr;
    };

    /**
     * @class OwnedRangeFunctor
     *
     * @brief Represents a range of optional-based functor while owning the functor.
     *
     * @tparam F The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     */
    template <typename F, typename R>
        requires traits::HasCallOp<F> and std::same_as<typename traits::OptIterTrait<F>::Ret, R>
    class [[nodiscard]] OwnedRangeFunctor
    {
    public:
        explicit OwnedRangeFunctor(F&& f)
            : m_f{ std::move(f) }
            , m_wrapper{ &m_f }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
            *m_storage = std::move(m_wrapper.next());
        }

        F&       underlying() { return m_f; }
        const F& underlying() const { return m_f; }

        Iterator<FunctorWrapper<F, R>, R> begin() { return Iterator{ &m_wrapper, m_storage.get() }; }
        Sentinel                          end() { return Sentinel{}; }

    private:
        F                                 m_f;
        FunctorWrapper<F, R>              m_wrapper;
        std::unique_ptr<std::optional<R>> m_storage = nullptr;
    };

    /**
     * @brief Helper function to create a Range or RangeFunctor.
     *
     * @tparam T The type of the iterable.
     *
     * @param t The iterable to be wrapped.
     *
     * @return Range if the iterable has `next()` member function, RangeFunctor if the iterable is a functor.
     */
    template <typename T>
        requires traits::OptIterTrait<T>::value
    auto make(T& t)
    {
        using Ret = traits::OptIterTrait<T>::Ret;

        if constexpr (traits::HasNext<T>) {
            return Range<T, Ret>{ t };
        } else if constexpr (traits::HasCallOp<T>) {
            return RangeFunctor<T, Ret>{ t };
        } else {
            static_assert(false, "Invalid type, should not reach here.");
        }
    }

    /**
     * @brief Helper function to create an OwnedRange or OwnedRangeFunctor.
     *
     * @tparam T The type of the iterable.
     * @tparam Args The arguments to construct the iterable.
     *
     * @param args The arguments to construct the iterable.
     *
     * @return OwnedRange if the iterable has `next()` member function, OwnedRangeFunctor if the iterable is a
     * functor.
     */
    template <typename T, typename... Args>
        requires traits::OptIterTrait<T>::value and std::constructible_from<T, Args...>
    auto make_owned(Args&&... args)
    {
        using Ret = traits::OptIterTrait<T>::Ret;
        if constexpr (traits::HasNext<T>) {
            return OwnedRange<T, Ret>{ T{ std::forward<Args>(args)... } };
        } else if constexpr (traits::HasCallOp<T>) {
            return OwnedRangeFunctor<T, Ret>{ T{ std::forward<Args>(args)... } };
        } else {
            static_assert(false, "Invalid type, should not reach here.");
        }
    }
}

#endif /* end of include guard: OPT_ITER_OPT_ITER_HPP */
