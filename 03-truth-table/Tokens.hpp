#pragma once

#include <string>

#include "TokenType.hpp"

struct Token {
  virtual ~Token() = default;

  [[nodiscard]] TokenType GetType() const {
    return m_type;
  }

protected:
  explicit Token(TokenType type)
  : m_type(type) {
  }

private:
  TokenType m_type;
};

struct TokenBool final : Token {
  explicit TokenBool(bool value)
  : Token(GetType())
  , m_value(value) {
  }

  static TokenType GetType() {
    return TokenType::BOOL;
  }

  [[nodiscard]] bool GetValue() const {
    return m_value;
  }

private:
  bool m_value;
};

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

struct TokenOperatorNot final : Token {
  explicit TokenOperatorNot()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::OPERATOR_NOT;
  }
};

struct TokenOperatorOr final : Token {
  explicit TokenOperatorOr()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::OPERATOR_OR;
  }
};

struct TokenOperatorXor final : Token {
  explicit TokenOperatorXor()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::OPERATOR_XOR;
  }
};

struct TokenOperatorAnd final : Token {
  explicit TokenOperatorAnd()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::OPERATOR_AND;
  }
};

struct TokenBraceOpen final : Token {
  explicit TokenBraceOpen()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::BRACE_OPEN;
  }
};

struct TokenBraceClose final : Token {
  explicit TokenBraceClose()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return TokenType::BRACE_CLOSE;
  }
};
