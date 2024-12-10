#pragma once

#include "OperatorType.hpp"

#include <string>

enum struct TokenType : uint32_t {
  NUMBER,
  OPERATOR,
  BRACE_OPEN,
  BRACE_CLOSE,
  COUNT
};

struct Token {
  virtual ~Token() = default;

  [[nodiscard]] TokenType GetType() const {
    return m_type;
  }

  [[nodiscard]] const std::string& GetToken() const {
    return m_token;
  }

protected:
  explicit Token(TokenType type, std::string token)
  : m_type(type)
  , m_token(std::move(token))
  {
  }

private:
  TokenType m_type;
  std::string m_token;
};

struct TokenNumber final : Token {
  explicit TokenNumber(std::string token, int64_t number)
  : Token(TokenNumber::GetType(), std::move(token))
  , m_number(number)
  {
  }

  [[nodiscard]] int64_t GetNumber() const {
    return m_number;
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::NUMBER;
  }

private:
  int64_t m_number;
};

struct TokenOperator : Token {
  explicit TokenOperator(std::string token, OperatorType opType)
  : Token(TokenOperator::GetType(), std::move(token))
  , m_opType(opType)
  {
  }

  [[nodiscard]] OperatorType GetOperatorType() const {
    return m_opType;
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::OPERATOR;
  }

private:
  OperatorType m_opType;
};

struct TokenBraceOpen final : Token {
  explicit TokenBraceOpen(std::string token)
  : Token(TokenBraceOpen::GetType(), std::move(token)) {
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::BRACE_OPEN;
  }
};

struct TokenBraceClose final : Token {
  explicit TokenBraceClose(std::string token)
  : Token(TokenBraceClose::GetType(), std::move(token)) {
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::BRACE_CLOSE;
  }
};
