#include "aligned_alloc.hpp"

#include <cstdlib>
#ifdef _WIN32
#include <malloc.h>
#endif

namespace avito_memory {

void* AlignedAllocator::Allocate(std::size_t align, 
  std::size_t size_bytes) noexcept {
#ifdef _WIN32
  return _aligned_malloc(size_bytes, align);
#else
  return std::aligned_alloc(align, size_bytes);
#endif
}

void AlignedAllocator::Deallocate(void* memptr) noexcept {
#ifdef _WIN32
  _aligned_free(memptr);
#else
  std::free(memptr);
#endif
}
} // namespace avito_memory
