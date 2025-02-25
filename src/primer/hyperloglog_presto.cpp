//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog_presto.cpp
//
// Identification: src/primer/hyperloglog_presto.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/hyperloglog_presto.h"
#include <bitset>
#include <cmath>
#include <cstdint>
#include <mutex>
#include "primer/hyperloglog.h"

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLogPresto<KeyType>::HyperLogLogPresto(int16_t n_leading_bits) : cardinality_(0), n_leading_bits_(n_leading_bits) {
  if (n_leading_bits_ >= 0) {
    dense_bucket_size_ = std::pow(2, n_leading_bits_);
    dense_bucket_.resize(dense_bucket_size_, {});
  }
}

/** @brief Element is added for HLL calculation. */
template <typename KeyType>
auto HyperLogLogPresto<KeyType>::AddElem(KeyType val) -> void {
  if (n_leading_bits_ < 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  auto hash = CalculateHash(val);
  auto binary = std::bitset<BITSET_CAPACITY>(hash);
  uint16_t index = (binary >> (BITSET_CAPACITY - n_leading_bits_)).to_ulong();

  uint8_t num = 0;
  for (int i = 0; i < BITSET_CAPACITY - n_leading_bits_; ++i) {
    if (binary.test(i)) {
      break;
    }
    ++num;
  }

  if (num <= MergeBucketNumOfIndex(index)) {
    return;
  }

  dense_bucket_[index] = std::bitset<DENSE_BUCKET_SIZE>(num & 0x0F);
  if (num > 15) {
    overflow_bucket_[index] = std::bitset<OVERFLOW_BUCKET_SIZE>((num >> DENSE_BUCKET_SIZE) & 0b111);
  }
}

/** @brief Function to compute cardinality. */
template <typename T>
auto HyperLogLogPresto<T>::ComputeCardinality() -> void {
  if (n_leading_bits_ < 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  double sum = 0;
  for (size_t i = 0; i < dense_bucket_size_; ++i) {
    sum += std::pow(2, -(static_cast<int16_t>(MergeBucketNumOfIndex(i))));
  }

  cardinality_ = CONSTANT * dense_bucket_size_ * dense_bucket_size_ / sum;
}

template class HyperLogLogPresto<int64_t>;
template class HyperLogLogPresto<std::string>;
}  // namespace bustub
