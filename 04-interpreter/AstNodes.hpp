#pragma once
#include <memory>
#include <string>
#include <vector>
/*

Nodes:

AstNodeStatementChain: (vector<AstNode> Statements)

AstNodeStatementLoop: (AstNode Value, AstNode StatementChain)
AstNodeStatementCondition: (AstNode condition, AstNode StatementChain)

AstNodeBinaryOperator: (Type operation, AstNode Left, AstNode Right) -> bool (==, !=, or, and)

AstNodeStatementVariableModification: (string Identifier, Type operation, AstNode Value) (add sub mult =)
AstNodeStatementFunctionDeclaration: (string Identifier, AstNode StatementChain)
AstNodeStatementCall: (string Identifier)

AstNodeStatementPrint
AstNodeStatementDelete: (string Identifier)

AstNodeValue: (Type, int Number / string Identifier)



EvaluateValue() -> number
EvaluateExpression() -> bool
EvaluateStatement() -> Interpreter modification


// And remember, function and variable identifiers can be the same

*/


enum struct AstNodeType : uint32_t {
  VALUE_NUMBER,//
  VALUE_IDENTIFIER,//
  BINARY_OPERATOR,//

  STATEMENT_PRINT,//
  STATEMENT_DELETE,//
  STATEMENT_CALL,//
  STATEMENT_VAR_MODIFICATION,//
  STATEMENT_FUNC_DECL,//
  STATEMENT_CONDITION,//
  STATEMENT_LOOP,//
  STATEMENT_CHAIN,

  COUNT
};

enum struct BinaryOperatorType : uint32_t {
  EQUALS,
  NOT_EQUALS,
  OR,
  AND,
  COUNT
};

enum struct ModificationOperatorType : uint32_t {
  ADD,
  SUBTRACT,
  MULTIPLY,
  ASSIGN,
  COUNT
};

struct AstNode;

struct AstNode {
  virtual ~AstNode() = default;

  [[nodiscard]] AstNodeType GetType() const {
    return m_type;
  }

  template <typename T>
  [[nodiscard]] const T* As() const requires(std::derived_from<T, AstNode>) {
    return dynamic_cast<const T*>(this);
  }

protected:
  explicit AstNode(AstNodeType type)
  : m_type(type) {
  }

private:
  AstNodeType m_type;
};

struct AstNodeValueNumber final : AstNode {
  explicit AstNodeValueNumber(int64_t value)
  : AstNode(GetType())
  , m_value(value) {
  }

  static AstNodeType GetType() {
    return AstNodeType::VALUE_NUMBER;
  }

  [[nodiscard]] int64_t GetValue() const {
    return m_value;
  }

private:
  int64_t m_value;
};

struct AstNodeValueIdentifier final : AstNode {
  explicit AstNodeValueIdentifier(std::string name)
  : AstNode(GetType())
  , m_name(std::move(name)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::VALUE_IDENTIFIER;
  }

  [[nodiscard]] const std::string& GetName() const {
    return m_name;
  }

private:
  std::string m_name;
};

struct AstNodeBinaryOperator final : AstNode {
  explicit AstNodeBinaryOperator(BinaryOperatorType type, std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> right)
  : AstNode(GetType())
  , m_type(type)
  , m_left(std::move(left))
  , m_right(std::move(right)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::BINARY_OPERATOR;
  }

  [[nodiscard]] BinaryOperatorType GetOperatorType() const {
    return m_type;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetLeft() const {
    return m_left;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetRight() const {
    return m_right;
  }

private:
  BinaryOperatorType m_type;
  std::shared_ptr<AstNode> m_left;
  std::shared_ptr<AstNode> m_right;
};

struct AstNodeStatementPrint final : AstNode {
  explicit AstNodeStatementPrint()
  : AstNode(GetType()) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_PRINT;
  }
};

struct AstNodeStatementDelete final : AstNode {
  explicit AstNodeStatementDelete(std::string variableName)
  : AstNode(GetType())
  , m_variableName(std::move(variableName)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_DELETE;
  }

  [[nodiscard]] const std::string& GetVariableName() const {
    return m_variableName;
  }

private:
  std::string m_variableName;
};

struct AstNodeStatementCall final : AstNode {
  explicit AstNodeStatementCall(std::string functionName)
  : AstNode(GetType())
  , m_functionName(std::move(functionName)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CALL;
  }

  [[nodiscard]] const std::string& GetFunctionName() const {
    return m_functionName;
  }

private:
  std::string m_functionName;
};

struct AstNodeBinaryStatementVarModification final : AstNode {
  explicit AstNodeBinaryStatementVarModification(ModificationOperatorType type, std::string varName, std::shared_ptr<AstNode> value)
  : AstNode(GetType())
  , m_type(type)
  , m_varName(std::move(varName))
  , m_value(std::move(value)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_VAR_MODIFICATION;
  }

  [[nodiscard]] ModificationOperatorType GetOperatorType() const {
    return m_type;
  }

  [[nodiscard]] const std::string& GetVariableName() const {
    return m_varName;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetValue() const {
    return m_value;
  }

private:
  ModificationOperatorType m_type;
  std::string m_varName;
  std::shared_ptr<AstNode> m_value;
};

struct AstNodeStatementFunctionDeclaration final : AstNode {
  explicit AstNodeStatementFunctionDeclaration(std::string functionName, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_functionName(std::move(functionName))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_FUNC_DECL;
  }

  [[nodiscard]] const std::string& GetFunctionName() const {
    return m_functionName;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::string m_functionName;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementCondition final : AstNode {
  explicit AstNodeStatementCondition(std::shared_ptr<AstNode> condition, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_condition(std::move(condition))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CONDITION;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCondition() const {
    return m_condition;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::shared_ptr<AstNode> m_condition;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementLoop final : AstNode {
  explicit AstNodeStatementLoop(std::shared_ptr<AstNode> initValue, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_initValue(std::move(initValue))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_LOOP;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetInitValue() const {
    return m_initValue;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::shared_ptr<AstNode> m_initValue;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementChain final : AstNode {
  explicit AstNodeStatementChain(std::vector<std::shared_ptr<AstNode>> statements)
  : AstNode(GetType())
  , m_statements(std::move(statements)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CHAIN;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<AstNode>>& GetStatements() const {
    return m_statements;
  }

private:
  std::vector<std::shared_ptr<AstNode>> m_statements;
};
