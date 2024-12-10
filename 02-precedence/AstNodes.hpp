#pragma once

#include "OperatorType.hpp"

#include <cstdint>
#include <memory>

enum struct AstNodeType {
  NUMBER,
  OPERATOR,
  COUNT
};

struct AstNode {
  virtual ~AstNode() = default;

  [[nodiscard]] AstNodeType GetType() const {
    return m_type;
  }

protected:
  explicit AstNode(AstNodeType type)
  : m_type(type) {
  }

private:
  AstNodeType m_type;
};

struct AstNodeNumber final : AstNode {
  explicit AstNodeNumber(int64_t number)
  : AstNode(GetType())
  , m_number(number) {
  }

  [[nodiscard]] static AstNodeType GetType() {
    return AstNodeType::NUMBER;
  }

  [[nodiscard]] int64_t GetNumber() const {
    return m_number;
  }

private:
  int64_t m_number;
};

struct AstNodeOperator final : AstNode {
  explicit AstNodeOperator(OperatorType opType, std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> right)
  : AstNode(GetType())
  , m_opType(opType)
  , m_left(std::move(left))
  , m_right(std::move(right)) {
  }

  [[nodiscard]] static AstNodeType GetType() {
    return AstNodeType::OPERATOR;
  }

  [[nodiscard]] OperatorType GetOperatorType() const {
    return m_opType;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetLeft() const {
    return m_left;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetRight() const {
    return m_right;
  }

private:
  OperatorType m_opType;
  std::shared_ptr<AstNode> m_left;
  std::shared_ptr<AstNode> m_right;
};
