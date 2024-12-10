#pragma once

#include "Tokens.hpp"

#include <cassert>
#include <string>
#include <vector>
#include <memory>

struct TokenizerContext final {
  explicit TokenizerContext(std::string input)
  : m_input(std::move(input))
  , m_position(0) {
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_input.size();
  }

  [[nodiscard]] char Next() const {
    assert(!IsEnd());
    return m_input[m_position];
  }

  char Advance() {
    assert(!IsEnd());
    return m_input[m_position++];
  }

  [[nodiscard]] bool MatchLookahead(const std::string& str) const {
    for (size_t i = 0; i < str.size(); ++i) {
      size_t position = m_position + i;
      // Out of range
      if (position > m_input.size()) {
        return false;
      }

      // Doesn't match
      if (m_input.at(position) != str.at(i)) {
        return false;
      }
    }

    return true;
  }

  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Token>>& GetTokens() const {
    return m_tokens;
  }

  std::vector<std::unique_ptr<Token>> ReleaseTokens() {
    std::vector<std::unique_ptr<Token>> result;
    std::swap(result, m_tokens);
    m_position = 0;
    return result;
  }

private:
  std::string m_input;
  size_t m_position;
  std::vector<std::unique_ptr<Token>> m_tokens;
};
