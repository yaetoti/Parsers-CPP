#pragma once

#include "Tokens.hpp"

#include <cassert>
#include <vector>
#include <memory>

struct ParserView final {
  struct State final {
    size_t position;
  };

  explicit ParserView(const std::vector<std::unique_ptr<Token>>& input)
  : m_input(input)
  , m_position(0) {
  }

  void Reset() {
    m_position = 0;
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] State GetState() const {
    return State(m_position);
  }

  void SetState(const State& state) {
    m_position = state.position;
  }

  [[nodiscard]] size_t RemainingSize() const {
    return m_input.size() - m_position;
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_input.size();
  }

  [[nodiscard]] bool HasTokens() const {
    return m_position < m_input.size();
  }

  [[nodiscard]] bool HasTokens(size_t amount) const {
    return m_position + amount <= m_input.size();
  }

  [[nodiscard]] const Token* Next() const {
    assert(HasTokens());
    return m_input.at(m_position).get();
  }

  [[nodiscard]] const Token* Next(size_t offset) const {
    assert(HasTokens(offset + 1));
    return m_input.at(m_position + offset).get();
  }

  template <ConceptTokenClass Type>
  [[nodiscard]] const Type* Next() const {
    assert(HasTokens());
    return dynamic_cast<const Type*>(m_input.at(m_position).get());
  }

  template <ConceptTokenClass Type>
  [[nodiscard]] const Type* Next(size_t offset) const {
    assert(HasTokens(offset + 1));
    return dynamic_cast<const Type*>(m_input.at(m_position + offset).get());
  }

  const Token* Advance() {
    assert(HasTokens());
    return m_input.at(m_position++).get();
  }

  template <typename T>
  const T* Advance() {
    assert(HasTokens());
    const T* result = dynamic_cast<const T*>(m_input.at(m_position++).get());
    assert(result);
    return result;
  }

  void Advance(size_t offset) {
    assert(HasTokens(offset));
    m_position += offset;
  }

  template <typename... TokenTypes>
  [[nodiscard]] bool Match(TokenTypes... types) const requires (... && std::same_as<TokenTypes, TokenType>) {
    if (!HasTokens()) {
      return false;
    }

    return ((Next()->GetType() == types) || ...);
  }

  template <typename... TokenTypes>
  [[nodiscard]] bool MatchAdvance(TokenTypes... types) requires (... && std::same_as<TokenTypes, TokenType>) {
    if (Match(types...)) {
      Advance();
      return true;
    }

    return false;
  }

private:
  const std::vector<std::unique_ptr<Token>>& m_input;
  size_t m_position;
};
