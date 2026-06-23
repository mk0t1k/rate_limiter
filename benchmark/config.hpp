#pragma once

#include <cstddef>

#include <chrono>

namespace config {

  constexpr std::size_t kBurstCapacity = 100;

  constexpr double kTargetRatePerSec = 1000.0; 

  constexpr auto kKeyTTL = std::chrono::seconds(5);

  constexpr double kMinTestDurationSec = 1.0; // 10.0

  constexpr int kNumThreads = 8; 

  constexpr int kRepetitions = 1; // 5
  
  // --- Burst ---
  constexpr int kBurstSize = 50;
  constexpr int kBurstInterReqUs = 100;
  constexpr double kBurstSleepSec = 1.0;

  // --- Sparse + Burst ---
  constexpr double kSparseBurstRatePerSec = 5.0;
  constexpr double kSparseBurstProbability = 0.02;

  // --- Zombie ---
  constexpr int kZombieReqsPerKey = 2;
  constexpr double kZombieTargetRatePerSec = 5000.0;

  // For testing aligned and non-aligned sharded limiters
  constexpr std::size_t kShardCapacity = 3;

  constexpr std::size_t kCrossShardKeyLastInFirstShard = kShardCapacity - 1;
  constexpr std::size_t kCrossShardKeyFirstInSecondShard = kShardCapacity;
} // namespace config
