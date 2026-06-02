#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <concepts>
#include <mutex>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <iostream>
#include <stdexcept>

#include "aligned_alloc.hpp"
#include "meta.hpp"
#include "interface.hpp"

namespace avito_limiter {

namespace impl {

constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;
} // namespace impl

template<requirements::RateLimiterLogic Alg, typename ShardCapacity,
  typename KeyType>
class ShardedWrapper final {
  struct DataNode {
    DataNode* next = nullptr;
    mutable std::mutex mtx;
    std::size_t cnt_active = 0U;
    alignas(avito_meta::Lcm<impl::kCacheLineSize, alignof(Alg)>::value) 
      std::byte data[ShardCapacity::value * sizeof(Alg)];
  };
  
  DataNode* data_start_ = nullptr;
  DataNode* data_end_ = nullptr;

  using alg_pos_type = std::pair<DataNode*, Alg*>;
  using index_type = std::unordered_map<KeyType, alg_pos_type>;
  index_type data_index_;

  void AddNode() {
    void* mem = avito_memory::AlignedAllocator::Allocate(
      impl::kCacheLineSize, sizeof(DataNode));
    if(mem == nullptr) {
      throw std::bad_alloc{};
    }
    DataNode* node = new (mem) DataNode{};
    assert(reinterpret_cast<std::uint64_t>(node) % alignof(DataNode) == 0);
    if(data_end_ != nullptr) {
      data_end_->next = node;
    }
    data_end_ = node;
    if(data_start_ == nullptr) {
      data_start_ = node;
    }
  }

  void DestroyAllNodes() {
    DataNode* curr = data_start_;
    while (curr != nullptr) {
      DataNode* next = curr->next;
      Alg* curr_alg = reinterpret_cast<Alg*>(&curr->data);
      for(std::size_t i = 0U; i < curr->cnt_active; ++i) {
        curr_alg->~Alg();
        ++curr_alg;
      }
      curr->~DataNode();
      avito_memory::AlignedAllocator::Deallocate(curr);
      curr = next;
    }
  }

  template<typename ... Args>
  alg_pos_type AllocateLimiter(DataNode* node_ptr, Args&&... args) {
    assert(node_ptr != nullptr && node_ptr->cnt_active < ShardCapacity::value);
    std::byte* mem_ptr = node_ptr->data + sizeof(Alg) * node_ptr->cnt_active;
    Alg* alg = new (mem_ptr) Alg{std::forward<Args>(args)...};
    (node_ptr->cnt_active)++;
    assert(alg != nullptr);
    return {node_ptr, alg};
  }

  template<typename ... Args>
  alg_pos_type EmplaceBack(Args&&... args) {
    DataNode* tgt = data_end_;
    if(tgt == nullptr || tgt->cnt_active == ShardCapacity::value) {
      AddNode();
      tgt = data_end_;
      assert(tgt != nullptr);
    }
    return AllocateLimiter(tgt, std::forward<Args>(args)...);
  } 

public:
  ShardedWrapper() {}

  template<std::input_iterator Iter, typename Sentinel>
  ShardedWrapper(Iter begin, Sentinel end) {
    while(begin != end) {
      data_index_.emplace(*begin, EmplaceBack());
      ++begin;
    }
  }

  template<std::input_iterator Iter, typename Sentinel>
  ShardedWrapper(Iter begin, Sentinel end, const Alg& init_alg) {
    while(begin != end) {
      data_index_.emplace(*begin, EmplaceBack(init_alg));
      ++begin;
    }
  }

  ShardedWrapper(const ShardedWrapper&) = delete;

  ShardedWrapper(ShardedWrapper&&) = delete;

  ShardedWrapper& operator=(const ShardedWrapper&) = delete;

  ShardedWrapper& operator=(ShardedWrapper&&) = delete;

  template<typename Func, typename ... Args>
  auto Visit(const KeyType& key, Func&& func, 
    Args&&... args) -> std::variant<std::false_type, 
    std::invoke_result_t<Func, Alg&, Args...>> {
    auto it = data_index_.find(key);
    if(it != data_index_.end()) {
      alg_pos_type pos = it->second;
      std::lock_guard lock(pos.first->mtx);
      return func(*(pos.second), args...);
    }
    return std::false_type{};
  }

  template<typename Func, typename ... Args>
  auto Visit(const KeyType& key, Func&& func, 
    Args&&... args) const -> std::variant<std::false_type, 
    std::invoke_result_t<Func, Alg&, Args...>> {
    auto it = data_index_.find(key);
    if(it != data_index_.end()) {
      alg_pos_type pos = it->second;
      std::lock_guard lock(pos.first->mtx);
      return func(*(pos.second), args...);
    }
    return std::false_type{};
  }

  ~ShardedWrapper() {
    DestroyAllNodes();
  }
};
} // namespace avito_limiter