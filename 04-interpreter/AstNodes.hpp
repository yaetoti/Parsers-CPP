#pragma once
#include <memory>
#include <string>

enum struct AstNodeType {
  BOOL,
  PARAMETER,
  UNARY_OPERATOR_NOT,
  BINARY_OPERATOR
};

enum struct BinaryOperatorType {
  AND,
  XOR,
  OR,
  COUNT
};

struct AstNode {
  virtual ~AstNode() = default;

  [[nodiscard]] AstNodeType GetType() const {
    return m_type;
  }

  template <typename T>
  [[nodiscard]] const T* As() const {
    return dynamic_cast<const T*>(this);
  }

protected:
  explicit AstNode(AstNodeType type)
  : m_type(type) {
  }

private:
  AstNodeType m_type;
};

struct AstNodeBool : AstNode {
  explicit AstNodeBool(bool value)
  : AstNode(GetType())
  , m_value(value) {
  }

  static AstNodeType GetType() {
    return AstNodeType::BOOL;
  }

  [[nodiscard]] bool GetValue() const {
    return m_value;
  }

private:
  bool m_value;
};

struct AstNodeParameter : AstNode {
  explicit AstNodeParameter(std::string name)
  : AstNode(GetType())
  , m_name(std::move(name)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::PARAMETER;
  }

  [[nodiscard]] const std::string& GetName() const {
    return m_name;
  }

private:
  std::string m_name;
};

struct AstNodeUnaryOperatorNot : AstNode {
  explicit AstNodeUnaryOperatorNot(std::shared_ptr<AstNode> child)
  : AstNode(GetType())
  , m_child(std::move(child)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::UNARY_OPERATOR_NOT;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetChild() const {
    return m_child;
  }

private:
  std::shared_ptr<AstNode> m_child;
};

struct AstNodeBinaryOperator : AstNode {
  explicit AstNodeBinaryOperator(BinaryOperatorType opType, std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> right)
  : AstNode(GetType())
  , m_opType(opType)
  , m_left(std::move(left))
  , m_right(std::move(right)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::BINARY_OPERATOR;
  }

  [[nodiscard]] BinaryOperatorType GetOperatorType() const {
    return m_opType;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetLeft() const {
    return m_left;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetRight() const {
    return m_right;
  }

private:
  BinaryOperatorType m_opType;
  std::shared_ptr<AstNode> m_left;
  std::shared_ptr<AstNode> m_right;
};
