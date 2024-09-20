#include "labeling/memory_pool.h"
#include "problem/debug.h"

#define BLOCK_SIZE (10 * 1024 * 1024)

MemoryPool::MemoryPool() :
    blocks_(),
    block_idx_(0),
    byte_idx_(0),
    object_size_(1)
{
    debug_assert(BLOCK_SIZE % 8 == 0);

    blocks_.reserve(50);
    blocks_.push_back(std::make_unique<Byte[]>(BLOCK_SIZE));
    debug_assert(blocks_.back());
}

void* MemoryPool::get_buffer()
{
    // Advance to the next block if there's no space in the current block.
    if (byte_idx_ + object_size_ >= BLOCK_SIZE)
    {
        block_idx_++;
        byte_idx_ = 0;
    }

    // Allocate new block if no space left.
    if (block_idx_ == blocks_.size())
    {
        blocks_.push_back(std::make_unique<Byte[]>(BLOCK_SIZE));
        debug_assert(blocks_.back());
    }

    // Find an address to store the object.
    debug_assert(block_idx_ < blocks_.size());
    debug_assert(byte_idx_ < BLOCK_SIZE);
    auto object = reinterpret_cast<void*>(&(blocks_[block_idx_][byte_idx_]));
    debug_assert(reinterpret_cast<uintptr_t>(object) % 8 == 0);

    // Done.
    return object;
}

void MemoryPool::commit_buffer()
{
    debug_assert(block_idx_ < blocks_.size());
    debug_assert(byte_idx_ < BLOCK_SIZE);
    debug_assert(object_size_ % 8 == 0);
    byte_idx_ += object_size_;
}

void MemoryPool::reset(const Size object_size)
{
    block_idx_ = 0;
    byte_idx_ = 0;
    object_size_ = ((object_size + 7) & (-8)); // Round up to next multiple of 8
}
