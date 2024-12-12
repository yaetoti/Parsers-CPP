#pragma once

#include <cassert>
#include <string>

struct TokenizerView final {
  explicit TokenizerView(const std::string& input)
  : m_input(input)
  , m_position(0)
  , m_line(1)
  , m_column(1) {
  }

  TokenizerView(const TokenizerView& other) = default;

  void Reset() {
    m_position = 0;
    m_line = 1;
    m_column = 1;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_line;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_column;
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] std::string ExtractToken(size_t offset, size_t size) const {
    assert(offset + size <= m_input.size());
    return m_input.substr(offset, size);
  }

  [[nodiscard]] std::string ExtractNextToken() const {
    int offset = 0;
    while (HasTokens(offset) && !isspace(Next(offset))) {
      ++offset;
    }

    return m_input.substr(m_position, m_position + offset);
  }

  [[nodiscard]] TokenizerView AtOffset(size_t offset) const {
    TokenizerView view(*this);
    view.Advance(offset);
    return view;
  }

  [[nodiscard]] bool HasTokens() const {
    return m_position < m_input.size();
  }

  [[nodiscard]] bool HasTokens(size_t count) const {
    return m_position + count <= m_input.size();
  }

  [[nodiscard]] char Next() const {
    assert(HasTokens());
    return m_input.at(m_position);
  }

  [[nodiscard]] char Next(size_t offset) const {
    assert(HasTokens(offset + 1));
    return m_input.at(m_position + offset);
  }

  [[nodiscard]] bool Match(char c) const {
    if (!HasTokens()) {
      return false;
    }

    return Next() == c;
  }

  [[nodiscard]] bool Match(std::string tokens) const {
    if (!HasTokens(tokens.size())) {
      return false;
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
      if (Next(i) != tokens.at(i)) {
        return false;
      }
    }

    return true;
  }

  [[nodiscard]] bool MatchAnyCase(std::string tokens) const {
    if (!HasTokens(tokens.size())) {
      return false;
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
      if (tolower(Next(i)) != tolower(tokens.at(i))) {
        return false;
      }
    }

    return true;
  }

  [[nodiscard]] bool MatchAny(const std::string& tokens) const {
    if (!HasTokens()) {
      return false;
    }

    return tokens.find(Next()) != std::string::npos;
  }

  char Advance() {
    assert(HasTokens());
    if (Next() == '\n') {
      ++m_line;
      m_column = 1;
    }
    return m_input.at(m_position++);
  }

  void Advance(size_t size) {
    assert(HasTokens(size));
    for (size_t i = 0; i < size; ++i) {
      Advance();
    }
  }

private:
  const std::string& m_input;
  size_t m_position;
  size_t m_line;
  size_t m_column;
};
