#ifndef OPT_ITER_TRAITS_HPP
#define OPT_ITER_TRAITS_HPP

#include <optional>
#include <type_traits>

namespace opt_iter::traits
{
    template <typename T>
    concept HasNext = requires (T t) {
        { t.next() };
    };

    template <typename T>
    concept HasCallOp = requires (T t) {
        { t() };
    };

    template <typename>
    struct OptTrait : std::false_type
    {
    };

    template <typename T>
    struct OptTrait<std::optional<T>> : std::true_type
    {
        using Type = T;
    };

    template <typename>
    struct OptIterTrait : std::false_type
    {
        static_assert(false, "Type is not compatible with opt_iter.");
    };

    template <HasNext T>
        requires OptTrait<std::invoke_result_t<decltype(&T::next), T>>::value
    struct OptIterTrait<T> : std::true_type
    {
        using Ret = OptTrait<std::invoke_result_t<decltype(&T::next), T>>::Type;
    };

    template <HasCallOp T>
        requires OptTrait<std::invoke_result_t<T>>::value
    struct OptIterTrait<T> : std::true_type
    {
        using Ret = OptTrait<std::invoke_result_t<T>>::Type;
    };
}

#endif /* end of include guard: OPT_ITER_TRAITS_HPP */
