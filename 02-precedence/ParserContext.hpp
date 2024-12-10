#pragma once

#include <cassert>
#include <memory>
#include <span>
#include <vector>

#include "Tokens.hpp"

struct ParserContext final {
  explicit ParserContext(const std::vector<std::unique_ptr<Token>>& tokens, size_t position = 0)
  : m_tokens(tokens)
  , m_position(position) {
  }

  ParserContext(const ParserContext& other) = default;

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_tokens.size();
  }

  [[nodiscard]] bool HasTokens(size_t number) const {
    return m_position + number < m_tokens.size();
  }

  [[nodiscard]] const Token* Next() const {
    assert(!IsEnd());
    return m_tokens.at(m_position).get();
  }

  [[nodiscard]] const Token* LookAhead(size_t offset) const {
    assert(HasTokens(offset));
    return m_tokens.at(m_position + offset).get();
  }

  const Token* Advance() {
    assert(!IsEnd());
    return m_tokens.at(m_position++).get();
  }

  [[nodiscard]] bool MatchType(TokenType type) const {
    if (IsEnd()) {
      return false;
    }

    return Next()->GetType() == type;
  }

  [[nodiscard]] bool MatchTypes(const std::span<TokenType>& types) const {
    if (!HasTokens(types.size())) {
      return false;
    }

    for (size_t i = 0; i < types.size(); ++i) {
      if (LookAhead(i)->GetType() != types[i]) {
        return false;
      }
    }

    return true;
  }

private:
  const std::vector<std::unique_ptr<Token>>& m_tokens;
  size_t m_position;
};
