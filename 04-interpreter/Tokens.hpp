#pragma once

#include <string>

#include "Grammar.hpp"

struct Token;

template <typename T>
concept ConceptTokenClass = requires(T type) {
  std::derived_from<T, Token>;
  { T::GetType() } -> std::convertible_to<TokenType>;
};

struct Token {
  virtual ~Token() = default;

  [[nodiscard]] TokenType GetType() const {
    return m_type;
  }

  template <ConceptTokenClass T>
  T* As() {
    assert(T::GetType() == GetType());
    return dynamic_cast<T*>(this);
  }

  template <ConceptTokenClass T>
  const T* As() const {
    assert(T::GetType() == GetType());
    return dynamic_cast<const T*>(this);
  }

protected:
  explicit Token(TokenType type)
  : m_type(type) {
  }

private:
  TokenType m_type;
};

template <TokenType T>
struct SimpleToken final : Token {
  explicit SimpleToken()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return T;
  }
};

using TokenDereference = SimpleToken<TokenType::OP_DEREFERENCE>;
using TokenOr = SimpleToken<TokenType::OP_OR>;
using TokenAnd = SimpleToken<TokenType::OP_AND>;
using TokenEqual = SimpleToken<TokenType::OP_EQUAL>;
using TokenNotEqual = SimpleToken<TokenType::OP_NOT_EQUAL>;
using TokenAssign = SimpleToken<TokenType::OP_ASSIGN>;
using TokenCall = SimpleToken<TokenType::OP_CALL>;

using TokenPrint = SimpleToken<TokenType::KEYWORD_PRINT>;
using TokenAdd = SimpleToken<TokenType::KEYWORD_ADD>;
using TokenSub = SimpleToken<TokenType::KEYWORD_SUB>;
using TokenMult = SimpleToken<TokenType::KEYWORD_MULT>;
using TokenDelete = SimpleToken<TokenType::KEYWORD_DELETE>;
using TokenIf = SimpleToken<TokenType::KEYWORD_IF>;
using TokenThen = SimpleToken<TokenType::KEYWORD_THEN>;
using TokenFunction = SimpleToken<TokenType::KEYWORD_FUNCTION>;
using TokenLoop = SimpleToken<TokenType::KEYWORD_LOOP>;
using TokenDo = SimpleToken<TokenType::KEYWORD_DO>;

using TokenBlockEnd = SimpleToken<TokenType::BLOCK_END>;

struct TokenIdentifier final : Token {
  explicit TokenIdentifier(std::string name)
  : Token(GetType())
  , m_name(std::move(name)) {
  }

  static TokenType GetType() {
    return TokenType::IDENTIFIER;
  }

  [[nodiscard]] const std::string& GetName() const {
    return m_name;
  }

private:
  std::string m_name;
};

struct TokenNumber final : Token {
  explicit TokenNumber(int64_t value)
  : Token(GetType())
  , m_value(value) {
  }

  static TokenType GetType() {
    return TokenType::IDENTIFIER;
  }

  [[nodiscard]] int64_t GetValue() const {
    return m_value;
  }

private:
  int64_t m_value;
};
