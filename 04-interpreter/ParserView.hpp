#pragma once

#include "Tokens.hpp"

#include <cassert>
#include <vector>
#include <memory>

struct ParserView final {
  explicit ParserView(const std::vector<std::unique_ptr<Token>>& input)
  : m_input(input)
  , m_position(0) {
  }

  void Reset() {
    m_position = 0;
  }

  [[nodiscard]] bool HasTokens() const {
    return m_position < m_input.size();
  }

  [[nodiscard]] const Token* Next() const {
    assert(HasTokens());
    return m_input.at(m_position).get();
  }

  template <typename T>
  [[nodiscard]] const T* NextAs() const {
    assert(HasTokens());
    return dynamic_cast<const T*>(m_input.at(m_position).get());
  }

  const Token* Advance() {
    assert(HasTokens());
    return m_input.at(m_position++).get();
  }

  template <typename T>
  const T* AdvanceAs() {
    assert(HasTokens());
    return dynamic_cast<const T*>(m_input.at(m_position++).get());
  }

  [[nodiscard]] bool MatchType(TokenType type) const {
    if (!HasTokens()) {
      return false;
    }

    return Next()->GetType() == type;
  }

private:
  const std::vector<std::unique_ptr<Token>>& m_input;
  size_t m_position;
};
