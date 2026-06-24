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

template<typename T, bool C, T P, T N>
struct ValueConditional final {
  static constexpr T value = P;

  constexpr operator T() noexcept {
    return P;
  }
};

template<typename T, T P, T N>
struct ValueConditional<T, false, P, N> final {
  static constexpr T value = N;

  constexpr operator T() noexcept {
    return N;
  }
};
} // namespace avito_meta
