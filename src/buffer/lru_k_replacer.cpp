//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <mutex>
#include <optional>
#include <utility>
#include "common/config.h"
#include "common/exception.h"

namespace bustub {

auto GetCurrentMilliseconds() -> size_t {

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

auto GetCurrentMicroseconds() -> size_t {

    return static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}

void LRUKNode::RecordAccessTime(size_t timestamp) {
    if (history_.size() >= k_) {
        history_.pop_front();
    }
    history_.push_back(timestamp);
}

auto LRUKNode::GetOldestAccessTime() -> size_t {
    if (history_.size() >= k_) {
        return history_.front();
    }
    return INF;
}

/**
 *
 * TODO(P1): Add implementation
 *
 * @brief a new LRUKReplacer.
 * @param num_frames the maximum number of frames the LRUReplacer will be required to store
 */
LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

/**
 * TODO(P1): Add implementation
 *
 * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
 * that are marked as 'evictable' are candidates for eviction.
 *
 * A frame with less than k historical references is given +inf as its backward k-distance.
 * If multiple frames have inf backward k-distance, then evict frame whose oldest timestamp
 * is furthest in the past.
 *
 * Successful eviction of a frame should decrement the size of replacer and remove the frame's
 * access history.
 *
 * @return true if a frame is evicted successfully, false if no frames can be evicted.
 */
auto LRUKReplacer::Evict() -> std::optional<frame_id_t> { 
    std::lock_guard<std::mutex> lock(latch_);
    if (curr_size_ == 0) {
        return {};
    }

    size_t current_time = GetCurrentMicroseconds();
    auto candidate_it = node_store_.end();
    size_t candidate_k_distance = INF;
    
    for (auto it = node_store_.begin(); it != node_store_.end(); ++it) {
        auto & node = it->second;
        if (!node.IsEvictable()) {
            continue;
        }

        if (candidate_it != node_store_.end()) {
            size_t access_time = node.GetOldestAccessTime();
            size_t k_distance = access_time == INF ? INF : current_time - access_time; 

            if (
                // (candidate_k_distance != INF && k_distance == INF) || 
                (candidate_k_distance != INF && k_distance > candidate_k_distance) || 
                (k_distance == candidate_k_distance && node.GetLastAccessTime() < candidate_it->second.GetLastAccessTime())) {
                candidate_it = it;
                candidate_k_distance = k_distance;
            }
        } else {
            candidate_it = it;
            size_t access_time = node.GetOldestAccessTime();
            if (access_time != INF) {
                candidate_k_distance = current_time - access_time; 
            }
        }
    }
    if (candidate_it != node_store_.end()) {
        frame_id_t frame_id = candidate_it->first;
        RemoveUnLock(frame_id);
        return frame_id;
    }

    return std::nullopt;
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Record the event that the given frame id is accessed at current timestamp.
 * Create a new entry for access history if frame id has not been seen before.
 *
 * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
 * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
 *
 * @param frame_id id of frame that received a new access.
 * @param access_type type of access that was received. This parameter is only needed for
 * leaderboard tests.
 */
void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
    std::lock_guard<std::mutex> lock(latch_);

    size_t current_time = GetCurrentMicroseconds();
    auto it = node_store_.find(frame_id);
    if (it == node_store_.end()) {
        auto iter = node_store_.emplace(frame_id, LRUKNode(k_, frame_id));
        it = iter.first;
    }
    it->second.RecordAccessTime(current_time);
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Toggle whether a frame is evictable or non-evictable. This function also
 * controls replacer's size. Note that size is equal to number of evictable entries.
 *
 * If a frame was previously evictable and is to be set to non-evictable, then size should
 * decrement. If a frame was previously non-evictable and is to be set to evictable,
 * then size should increment.
 *
 * If frame id is invalid, throw an exception or abort the process.
 *
 * For other scenarios, this function should terminate without modifying anything.
 *
 * @param frame_id id of frame whose 'evictable' status will be modified
 * @param set_evictable whether the given frame is evictable or not
 */
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    std::lock_guard<std::mutex> lock(latch_);
    /// TODO frame id is invalid

    if (auto it = node_store_.find(frame_id); it != node_store_.end()) {
        LRUKNode & node = it->second;
        if (node.IsEvictable() && !set_evictable) {
            --curr_size_;
            node.SetEvictable(set_evictable);
        } else if (!node.IsEvictable() && set_evictable) {
            ++curr_size_;
            node.SetEvictable(set_evictable);
        }
    }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Remove an evictable frame from replacer, along with its access history.
 * This function should also decrement replacer's size if removal is successful.
 *
 * Note that this is different from evicting a frame, which always remove the frame
 * with largest backward k-distance. This function removes specified frame id,
 * no matter what its backward k-distance is.
 *
 * If Remove is called on a non-evictable frame, throw an exception or abort the
 * process.
 *
 * If specified frame is not found, directly return from this function.
 *
 * @param frame_id id of frame to be removed
 */
void LRUKReplacer::Remove(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(latch_);
    RemoveUnLock(frame_id);
}

void LRUKReplacer::RemoveUnLock(frame_id_t frame_id) {
    if (auto it = node_store_.find(frame_id); it != node_store_.end()) {
        if (it->second.IsEvictable()) {
            node_store_.erase(it);
            --curr_size_;
            return;
        }

        throw new Exception("Logical error: Remove a non-evictable frame");
    }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Return replacer's size, which tracks the number of evictable frames.
 *
 * @return size_t
 */
auto LRUKReplacer::Size() -> size_t { 
    std::lock_guard<std::mutex> lock(latch_);
    return curr_size_; 
}

}  // namespace bustub
