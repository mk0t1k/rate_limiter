#pragma once

#include <cstddef>

#include <chrono>

namespace config {

  constexpr int kBurstCapacity = 100;

  constexpr double kTargetRatePerSec = 1000.0; 

  constexpr auto kKeyTTL = std::chrono::seconds(5);

  constexpr double kMinTestDurationSec = 1.0; // 10.0

  constexpr int kNumThreads = 8; 

  constexpr int kRepetitions = 1; // 5
  
  // For testing aligned and non-aligned sharded limiters
  constexpr std::size_t kShardCapacity = 3;

  constexpr std::size_t kCrossShardKeyLastInFirstShard = kShardCapacity - 1;
  constexpr std::size_t kCrossShardKeyFirstInSecondShard = kShardCapacity;
} // namespace config
