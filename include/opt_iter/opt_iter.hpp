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
     * @brief Checks if a type is compatible to be std::optional-based iterator return type.
     *
     * @tparam R The type to be checked.
     */
    template <typename R>
    concept OptIterRet = std::movable<R> and not std::is_reference_v<R>;

    /**
     * @brief Checks if a type is compatible to be std::optional-based iterator.
     *
     * @tparam T The type to be checked.
     * @tparam R The return type of the iterable (unwrapped).
     */
    template <typename T>
    concept OptIter = traits::HasNext<T> or traits::HasCallOp<T>;

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
    template <traits::HasNext T, OptIterRet R>
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

        [[nodiscard]] R operator*() const
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
     * @class FnWrapper
     *
     * @brief Wraps a functor to be compatible with opt_iter functionalities.
     *
     * @tparam F The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     */
    template <traits::HasCallOp F, OptIterRet R>
        requires std::same_as<typename traits::OptIterTrait<F>::Ret, R>
    struct [[nodiscard]] FnWrapper
    {
        std::optional<R> next()
        {
            assert(fn != nullptr);
            return fn->operator()();
        }

        F* fn = nullptr;
    };

    /**
     * @class Range
     *
     * @brief Represents a range of optional-based iterable.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable (unwrapped).
     * @tparam OwnStorage Whether the range should create the storage of the optional by its own.
     */
    template <traits::HasNext T, OptIterRet R, bool OwnStorage>
    class [[nodiscard]] Range
    {
    public:
        using Ret   = R;
        using Store = std::conditional_t<OwnStorage, std::unique_ptr<std::optional<R>>, std::optional<R>*>;

        Range(std::optional<R>& storage, T& t)
            requires std::same_as<Store, std::optional<R>*>
            : m_t{ &t }
            , m_storage{ &storage }
        {
        }

        Range(T& t)
            requires std::same_as<Store, std::unique_ptr<std::optional<R>>>
            : m_t{ &t }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
        }

        T& underlying() const
        {
            assert(m_t != nullptr);
            return *m_t;
        }

        void clear()
        {
            assert(m_storage != nullptr);
            *m_storage = std::nullopt;
        }

        Iterator<T, R> begin()
        {
            assert(m_storage != nullptr);
            if (*m_storage == std::nullopt) {
                *m_storage = std::move(m_t->next());
            }
            return Iterator{ m_t, &*m_storage };
        }

        Sentinel end() { return Sentinel{}; }

    private:
        T*    m_t       = nullptr;
        Store m_storage = nullptr;
    };

    /**
     * @class FnWrapper
     *
     * @brief Wraps a functor to be compatible with opt_iter functionalities.
     *
     * @tparam Fn The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     * @tparam OwnStorage Whether the range should create the storage of the optional by its own.
     */
    template <traits::HasCallOp Fn, OptIterRet R, bool OwnStorage>
        requires std::same_as<typename traits::OptIterTrait<Fn>::Ret, R>
    class [[nodiscard]] RangeFn
    {
    public:
        using Ret   = R;
        using Store = std::conditional_t<OwnStorage, std::unique_ptr<std::optional<R>>, std::optional<R>*>;

        RangeFn(std::optional<R>& storage, Fn& fn)
            requires std::same_as<Store, std::optional<R>*>
            : m_wrapper{ &fn }
            , m_storage{ &storage }
        {
        }

        RangeFn(Fn& fn)
            requires std::same_as<Store, std::unique_ptr<std::optional<R>>>
            : m_wrapper{ &fn }
            , m_storage{ std::make_unique<std::optional<R>>() }
        {
        }

        Fn& underlying() const
        {
            assert(m_wrapper.fn != nullptr);
            return *m_wrapper.fn;
        }

        void clear()
        {
            assert(m_storage != nullptr);
            *m_storage = std::nullopt;
        }

        Iterator<FnWrapper<Fn, R>, R> begin()
        {
            assert(m_storage != nullptr);
            if (*m_storage == std::nullopt) {
                *m_storage = std::move(m_wrapper.next());
            }
            return Iterator{ &m_wrapper, &*m_storage };
        }

        Sentinel end() { return Sentinel{}; }

    private:
        FnWrapper<Fn, R> m_wrapper;
        Store            m_storage = nullptr;
    };

    /**
     * @class OwnedRange
     *
     * @brief Represents a range of optional-based iterable while owning the iterable.
     *
     * @tparam T The type of the iterable.
     * @tparam R The return type of the iterable (unwrapped).
     */
    template <traits::HasNext T, OptIterRet R>
    class [[nodiscard]] OwnedRange
    {
    public:
        using Ret = R;

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        OwnedRange(Args&&... args)
            : m_data{ std::make_unique<Data>(T{ std::forward<Args>(args)... }) }
        {
        }

        T&       underlying() { return m_data->t; }
        const T& underlying() const { return m_data->t; }

        void clear() { m_data->store = std::nullopt; }

        Iterator<T, R> begin()
        {
            if (m_data->store == std::nullopt) {
                m_data->store = std::move(m_data->t.next());
            }
            return Iterator{ &m_data->t, &m_data->store };
        }

        Sentinel end() { return Sentinel{}; }

    private:
        struct Data
        {
            T                t;
            std::optional<R> store = std::nullopt;
        };

        std::unique_ptr<Data> m_data = nullptr;
    };

    /**
     * @class OwnedRangeFn
     *
     * @brief Represents a range of optional-based functor while owning the functor.
     *
     * @tparam Fn The type of the functor.
     * @tparam R The return type of the functor (unwrapped).
     */
    template <traits::HasCallOp Fn, OptIterRet R>
        requires std::same_as<typename traits::OptIterTrait<Fn>::Ret, R>
    class [[nodiscard]] OwnedRangeFn
    {
    public:
        using Ret = R;

        template <typename... Args>
            requires std::constructible_from<Fn, Args...>
        OwnedRangeFn(Args&&... args)
            : m_data{ std::make_unique<Data>(Fn{ std::forward<Args>(args)... }) }
        {
            m_data->fn_wrap.fn = &m_data->fn;
        }

        Fn&       underlying() { return m_data->fn; }
        const Fn& underlying() const { return m_data->fn; }

        void clear() { m_data->store = std::nullopt; }

        Iterator<FnWrapper<Fn, R>, R> begin()
        {
            if (m_data->store == std::nullopt) {
                m_data->store = std::move(m_data->fn_wrap.next());
            }
            return Iterator{ &m_data->fn_wrap, &m_data->store };
        }

        Sentinel end() { return Sentinel{}; }

    private:
        struct Data
        {
            Fn               fn;
            FnWrapper<Fn, R> fn_wrap = {};
            std::optional<R> store   = std::nullopt;
        };

        std::unique_ptr<Data> m_data = nullptr;
    };

    /**
     * @brief Helper function to create a Range or RangeFn.
     *
     * @tparam T The type of the iterable.
     *
     * @param t The iterable to be wrapped.
     *
     * @return Range if the iterable has `next()` member function, RangeFn if the iterable is a functor.
     *
     * The returned object will make its own storage for the optional value. Since the type itself needs to
     * be movable while the storage must be stay in one place, the storage is allocated in the heap. Use
     * `make_with()` if you want to use your own storage. Or since the storage is already on the heap
     * anyway, you can let the `Range` itself to also own the iterable by using `make_owned()`.
     */
    template <OptIter T>
    auto make(T& t)
    {
        using Ret = traits::OptIterTrait<T>::Ret;
        if constexpr (traits::HasNext<T> and traits::HasCallOp<T>) {
            return Range<T, Ret, true>{ t };
        } else if constexpr (traits::HasNext<T>) {
            return Range<T, Ret, true>{ t };
        } else if constexpr (traits::HasCallOp<T>) {
            return RangeFn<T, Ret, true>{ t };
        } else {
            static_assert(false, "Invalid type, should not reach here.");
        }
    }

    /**
     * @brief Helper function to create an OwnedRange or OwnedRangeFn.
     *
     * @tparam T The type of the iterable.
     * @tparam Args The arguments to construct the iterable.
     *
     * @param args The arguments to construct the iterable.
     *
     * @return OwnedRange if the iterable has `next()` member function, OwnedRangeFn if the iterable is a
     * functor.
     *
     * The returned object will own the iterable and make its own storage for the optional value.
     */
    template <OptIter T, typename... Args>
        requires std::constructible_from<T, Args...>
    auto make_owned(Args&&... args)
    {
        using Ret = traits::OptIterTrait<T>::Ret;
        if constexpr (traits::HasNext<T> and traits::HasCallOp<T>) {
            return OwnedRange<T, Ret>{ std::forward<Args>(args)... };
        } else if constexpr (traits::HasNext<T>) {
            return OwnedRange<T, Ret>{ std::forward<Args>(args)... };
        } else if constexpr (traits::HasCallOp<T>) {
            return OwnedRangeFn<T, Ret>{ std::forward<Args>(args)... };
        } else {
            static_assert(false, "Invalid type, should not reach here.");
        }
    }

    /**
     * @brief Helper function to create a Range or RangeFn.
     *
     * @tparam T The type of the iterable.
     *
     * @param storage The storage for the optional value.
     * @param t The iterable to be wrapped.
     *
     * @return Range if the iterable has `next()` member function, RangeFn if the iterable is a functor.
     *
     * The returned object will use the provided storage for the optional value. The storage must be valid
     * for the lifetime of the returned object.
     */
    template <OptIter T>
    auto make_with(std::optional<typename traits::OptIterTrait<T>::Ret>& storage, T& t)
    {
        using Ret = traits::OptIterTrait<T>::Ret;
        if constexpr (traits::HasNext<T> and traits::HasCallOp<T>) {
            return Range<T, Ret, false>{ storage, t };
        } else if constexpr (traits::HasNext<T>) {
            return Range<T, Ret, false>{ storage, t };
        } else if constexpr (traits::HasCallOp<T>) {
            return RangeFn<T, Ret, false>{ storage, t };
        } else {
            static_assert(false, "Invalid type, should not reach here.");
        }
    }

    /**
     * @brief Helper function to create an OwnedRangeFn from a lambda.
     *
     * @param fn A lambda function to be wrapped.
     *
     * @return OwnedRangeFn that wraps the lambda function.
     */
    template <traits::HasCallOp Fn>
    auto make_lambda(Fn&& fn)
    {
        using Ret = traits::OptIterTrait<Fn>::Ret;
        return OwnedRangeFn<Fn, Ret>{ std::forward<Fn>(fn) };
    }
}

#endif /* end of include guard: OPT_ITER_OPT_ITER_HPP */
