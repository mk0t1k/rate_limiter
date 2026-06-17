#pragma once

#include <cstddef>

namespace avito_memory {

class AlignedAllocator final {
public:
  static void* Allocate(std::size_t align, std::size_t size_bytes) noexcept;

  static void Deallocate(void* memptr) noexcept;
};
} // namespace avito_memory
