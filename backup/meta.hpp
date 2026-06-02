#pragma once

#include <cstddef>

namespace avito_meta {

template<std::size_t A, std::size_t B>
struct Gcd {
  static constexpr std::size_t value = Gcd<B, A % B>::value;
};

template<std::size_t A>
struct Gcd<A, 0U> {
  static constexpr std::size_t value = A;
};

template<std::size_t A, std::size_t B>
struct Lcm {
  static constexpr std::size_t value = A * B / Gcd<A, B>::value;
};

template<std::size_t A>
struct Lcm<A, 0U> {
  static constexpr std::size_t value = A;
};

template<std::size_t B>
struct Lcm<0U, B> {
  static constexpr std::size_t value = B;
};

template<>
struct Lcm<0U, 0U> {
  static constexpr std::size_t value = 0U;
};

template<std::size_t ... Sizes>
struct SizeSequence {};

template<typename ... Types>
struct AmorphicTuple {};

template<typename T, typename Atuple>
struct AmorphAppend {};

template<typename T, typename ... Types>
struct AmorphAppend<T, AmorphicTuple<Types...>> {
  using type = AmorphicTuple<Types..., T>;
};

template<typename T, typename Atuple>
struct AmorphPrepend {};

template<typename T, typename ... Types>
struct AmorphPrepend<T, AmorphicTuple<Types...>> {
  using type = AmorphicTuple<T, Types...>;
};

template<typename Ta, typename Tb>
struct ConcatTuples {};

template<typename ... Largs, typename ... Rargs>
struct ConcatTuples<AmorphicTuple<Largs...>, AmorphicTuple<Rargs...>> {
  using type = AmorphicTuple<Largs..., Rargs...>;
};

template<typename ... Largs>
struct ConcatTuples<AmorphicTuple<Largs...>, AmorphicTuple<>> {
  using type = AmorphicTuple<Largs...>;
};

template<typename ... Rargs>
struct ConcatTuples<AmorphicTuple<>, AmorphicTuple<Rargs...>> {
  using type = AmorphicTuple<Rargs...>;
};

template<typename N, typename Tuple>
using AmorphAppendT = typename AmorphAppend<N, Tuple>::type;

template<typename N, typename Tuple>
using AmorphPrependT = typename AmorphPrepend<N, Tuple>::type;

template<typename U, typename V>
using AmorphConcatT = typename ConcatTuples<U, V>::type;
} // namespace avito_meta
