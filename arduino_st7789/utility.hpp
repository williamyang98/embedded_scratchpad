#pragma once

template <int N>
inline void nop() {
  static_assert(N > 0);
  asm volatile("nop\n");
  nop<N-1>();
}

template <>
inline void nop<0>() {}