#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

struct DynamicBitset final {
  explicit DynamicBitset()
  : m_bits(0) {
  }

  explicit DynamicBitset(size_t bits)
  : m_bits(bits) {
    size_t chunks = (bits + kChunkSize - 1) / kChunkSize;
    m_chunks.resize(chunks);
  }

  [[nodiscard]] size_t GetSize() const {
    return m_bits;
  }

  void Reset() {
    std::ranges::fill(m_chunks, 0);
  }

  void Resize(size_t bits) {
    m_bits = bits;
    size_t chunks = (bits + kChunkSize - 1) / kChunkSize;
    m_chunks.resize(chunks);
    Reset();
  }

  bool Get(size_t index) const {
    assert(index < m_bits);
    size_t chunkId = index / kChunkSize;
    size_t bitId = index % kChunkSize;
    return (m_chunks.at(chunkId) >> bitId) & 1;
  }

  [[nodiscard]] bool AllSet() const {
    size_t fullChunks = m_bits / kChunkSize;
    size_t remainingBits = m_bits % kChunkSize;

    for (size_t chunkId = 0; chunkId < fullChunks; ++chunkId) {
      size_t chunkValue = m_chunks[chunkId];
      for (size_t bitId = 0; bitId < kChunkSize; ++bitId) {
        if ((chunkValue >> bitId & 1) == 0) {
          return false;
        }
      }
    }

    size_t chunkValue = m_chunks[fullChunks];
    for (size_t bitId = 0; bitId < remainingBits; ++bitId) {
      if ((chunkValue >> bitId & 1) == 0) {
        return false;
      }
    }

    return true;
  }

  DynamicBitset& operator++() {
    bool overflow = true;
    size_t fullChunks = m_bits / kChunkSize;

    for (size_t chunkId = 0; overflow && chunkId < fullChunks; ++chunkId) {
      ++m_chunks.at(chunkId);
      if (m_chunks.at(chunkId) != 0) {
        overflow = false;
      }
    }

    if (!overflow) {
      return *this;
    }

    ++m_chunks.at(fullChunks);
    if (m_chunks.at(fullChunks) == (1 << m_bits % kChunkSize)) {
      // Set to 0 on overflow
      Reset();
    }

    return *this;
  }

  [[nodiscard]] std::vector<bool> AsBoolVector() const {
    std::vector<bool> result(m_bits);
    for (size_t bitId = 0; bitId < m_bits; ++bitId) {
      // TODO Get is not effecient for copying
      result.at(bitId) = Get(bitId);
    }

    return result;
  }

private:
  std::vector<size_t> m_chunks;
  size_t m_bits;
  static constexpr size_t kChunkSize = 8 * sizeof(size_t);
};
