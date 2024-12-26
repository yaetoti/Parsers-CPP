#pragma once

#include "StringUtils.hpp"

#include <cassert>
#include <cctype>
#include <string_view>

template <typename Type>
concept StringViewIsh = std::convertible_to<Type, std::string_view>;

template <typename Func>
concept CharPredicate = requires(Func f, char c) {
  { f(c) } -> std::convertible_to<bool>;
};

struct LexerView final {
  struct State final {
    size_t position;
    size_t line;
    size_t column;
  };

  explicit LexerView(std::string_view string)
  : m_string(string)
  , m_position(0)
  , m_line(1)
  , m_column(1) {
  }

  LexerView(const LexerView& other) = default;

  [[nodiscard]] LexerView AtOffset(size_t offset) const {
    assert(HasSymbols(offset));
    LexerView view(*this);
    view.Advance(offset);
    return view;
  }

  void Reset() {
    m_position = 0;
    m_line = 1;
    m_column = 1;
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_line;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_column;
  }

  [[nodiscard]] State GetState() const {
    return State(m_position, m_line, m_column);
  }

  void SetState(const State& state) {
    m_position = state.position;
    m_line = state.line;
    m_column = state.column;
  }

  [[nodiscard]] size_t RemainingSize() const {
    return m_string.size() - m_position;
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_string.size();
  }

  [[nodiscard]] bool HasSymbols() const {
    return m_position < m_string.size();
  }

  [[nodiscard]] bool HasSymbols(size_t number) const {
    return m_position + number <= m_string.size();
  }

  [[nodiscard]] char Next() const {
    assert(HasSymbols());
    return m_string.at(m_position);
  }

  [[nodiscard]] char Next(size_t offset) const {
    assert(HasSymbols(offset + 1));
    return m_string.at(m_position + offset);
  }

  [[nodiscard]] char NextAsLowerCase() const {
    assert(HasSymbols());
    return static_cast<char>(std::tolower(m_string.at(m_position)));
  }

  [[nodiscard]] char NextAsLowerCase(size_t offset) const {
    assert(HasSymbols(offset + 1));
    return static_cast<char>(std::tolower(m_string.at(m_position + offset)));
  }

  char Advance() {
    assert(HasSymbols());
    if (Next() == '\n') {
      ++m_line;
      m_column = 1;
    } else {
      ++m_column;
    }

    return m_string.at(m_position++);
  }

  char AdvanceAsLowerCase() {
    assert(HasSymbols());
    if (Next() == '\n') {
      ++m_line;
      m_column = 1;
    } else {
      ++m_column;
    }

    return static_cast<char>(std::tolower(m_string.at(m_position++)));
  }

  void Advance(size_t offset) {
    assert(HasSymbols(offset));
    while (offset-- > 0) {
      Advance();
    }
  }

  [[nodiscard]] std::string_view GetTokenView(size_t start, size_t length) const {
    assert(start + length <= m_string.size());
    return m_string.substr(start, length);
  }

  [[nodiscard]] std::string_view GetTokenView(size_t length) const {
    assert(HasSymbols(length));
    return m_string.substr(m_position, length);
  }

  [[nodiscard]] std::string GetToken(size_t start, size_t length) const {
    return std::string(GetTokenView(start, length));
  }

  [[nodiscard]] std::string GetToken(size_t length) const {
    return std::string(GetTokenView(length));
  }


  // Match


  template <typename... Chars>
  [[nodiscard]] bool Match(Chars... chars) const requires(... && std::same_as<Chars, char>) {
    if (!HasSymbols()) {
      return false;
    }

    char c = Next();
    return ((chars == c) || ...);
  }

  // TODO refactor 2 stringviewish

  template <StringViewIsh... StringViews>
  [[nodiscard]] bool Match(const StringViews&... views) const {
    return ((m_string.find(std::string_view { views }, m_position) == m_position) || ...);
  }

  template <CharPredicate... Predicates>
  [[nodiscard]] bool Match(Predicates... predicates) const {
    if (!HasSymbols()) {
      return false;
    }

    char c = Next();
    return (predicates(c) || ...);
  }

  template <typename... Chars>
  [[nodiscard]] bool MatchIgnoreCase(Chars... chars) const requires(... && std::same_as<Chars, char>) {
    if (!HasSymbols()) {
      return false;
    }

    char c = NextAsLowerCase();
    return ((c == tolower(chars)) || ...);
  }

  template <StringViewIsh... StringViews>
  [[nodiscard]] bool MatchIgnoreCase(const StringViews&... views) const {
    return (([this](const std::string_view view) {
      return EqualsIgnoreCase(m_string.substr(m_position, view.size()), view);
    }(views)) || ...);
  }


  // MatchAdvance


  template <typename... Chars>
  bool MatchAdvance(Chars... chars) requires(... && std::same_as<Chars, char>) {
    if (Match(chars...)) {
      Advance();
      return true;
    }

    return false;
  }

  template <StringViewIsh... StringViews>
  bool MatchAdvance(const StringViews&... views) {
    return (([this](std::string_view view) {
      if (Match(view)) {
        Advance(view.size());
        return true;
      }

      return false;
    }(views)) || ...);
  }

  template <CharPredicate... Predicate>
  bool MatchAdvance(Predicate... predicates) {
    if (!HasSymbols()) {
      return false;
    }

    if (Match(predicates...)) {
      Advance();
      return true;
    }

    return false;
  }

  template <typename... Chars>
  bool MatchAdvanceIgnoreCase(Chars... chars) requires(... && std::same_as<Chars, char>) {
    if (MatchIgnoreCase(chars)) {
      Advance();
      return true;
    }

    return false;
  }

  template <StringViewIsh... StrViews>
  bool MatchAdvanceIgnoreCase(const StrViews&... views) {
    return (([this](const std::string_view view) {
      if (MatchIgnoreCase(view)) {
        Advance(view.size());
        return true;
      }

      return false;
    }(views)) || ...);
  }

private:
  std::string_view m_string;
  size_t m_position;
  size_t m_line;
  size_t m_column;
};
