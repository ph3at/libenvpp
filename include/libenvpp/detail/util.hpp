#pragma once

#include <type_traits>

namespace env::detail::util {

//////////////////////////////////////////////////////////////////////////
// Always false helper for static assertions.

template <typename...>
struct always_false : std::false_type {
};
template <typename... Ts>
inline constexpr auto always_false_v = always_false<Ts...>::value;

} // namespace env::detail::util
