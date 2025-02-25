//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog.cpp
//
// Identification: src/primer/hyperloglog.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/hyperloglog.h"
#include <cmath>
#include <cstdint>
#include <mutex>

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLog<KeyType>::HyperLogLog(int16_t n_bits) : cardinality_(0), b_(n_bits) {
  if (b_ >= 0) {
    register_size_ = static_cast<size_t>(std::pow(2, n_bits));
    register_.resize(register_size_, 0);
  }
}

/**
 * @brief Function that computes binary.
 *
 * @param[in] hash
 * @returns binary of a given hash
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeBinary(const hash_t &hash) const -> std::bitset<BITSET_CAPACITY> {
  return std::bitset<BITSET_CAPACITY>(hash);
}

/**
 * @brief Function that computes leading zeros.
 *
 * @param[in] bset - binary values of a given bitset
 * @returns leading zeros of given binary set
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::PositionOfLeftmostOne(const std::bitset<BITSET_CAPACITY> &bset) const -> uint64_t {
  uint64_t p = 0;
  for (int i = bset.size() - 1 - b_; i >= 0; --i) {
    ++p;
    if (bset.test(i)) {
      return p;
    }
  }
  return p;
}

/**
 * @brief Adds a value into the HyperLogLog.
 *
 * @param[in] val - value that's added into hyperloglog
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::AddElem(KeyType val) -> void {
  if (b_ < 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  hash_t hash = CalculateHash(val);
  auto binary = ComputeBinary(hash);
  uint64_t index = (binary >> (BITSET_CAPACITY - b_)).to_ulong();
  register_[index] = std::max(register_[index], static_cast<int64_t>(PositionOfLeftmostOne(binary)));
}

/**
 * @brief Function that computes cardinality.
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeCardinality() -> void {
  if (b_ < 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  double sum = 0;
  for (size_t i = 0; i < register_size_; ++i) {
    sum += std::pow(2, -(register_[i]));
  }

  cardinality_ = CONSTANT * register_size_ * register_size_ / sum;
}

template class HyperLogLog<int64_t>;
template class HyperLogLog<std::string>;

}  // namespace bustub
