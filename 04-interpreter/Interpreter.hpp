#pragma once

#include <bitset>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "AstNodes.hpp"

/*

Nodes:

AstNodeStatementLoop: (AstNode Value, AstNode StatementChain)
AstNodeStatementCondition: (AstNode condition, AstNode StatementChain)

EvaluateValue() -> number
EvaluateExpression() -> bool
EvaluateStatement() -> Interpreter modification


// And remember, function and variable identifiers can be the same

*/

struct ExecutionException final : std::exception {
  explicit ExecutionException(const char* message)
  : std::exception(message) {
  }
};

struct Variable final {
  std::string name;
  int64_t value;
};

struct Interpreter final {
  void Reset() {
    m_shouldTerminate = false;
  }

  void Evaluate(const AstNode* root) noexcept(false) {
    if (m_shouldTerminate) {
      return;
    }

    EvaluateStatement(root);
  }

private:
  int64_t EvaluateValue(const AstNode* node) {
    if (node->GetType() == AstNodeType::VALUE_NUMBER) {
      return node->As<AstNodeValueNumber>()->GetValue();
    }

    if (node->GetType() == AstNodeType::VALUE_IDENTIFIER) {
      const auto& varName = node->As<AstNodeValueIdentifier>()->GetName();
      if (!m_variables.contains(varName)) {
        throw ExecutionException("Undefined variable.");
      }

      return m_variables.at(varName)->value;
    }

    throw ExecutionException("Unexpected node type.");
  }

  bool EvaluateExpression(const AstNode* node) {
    if (node->GetType() != AstNodeType::BINARY_OPERATOR) {
      throw ExecutionException("Unexpected node type.");
    }

    const auto* opNode = node->As<AstNodeBinaryOperator>();
    if (opNode->GetOperatorType() == BinaryOperatorType::EQUALS) {
      return EvaluateValue(opNode->GetLeft().get()) == EvaluateValue(opNode->GetRight().get());
    }

    if (opNode->GetOperatorType() == BinaryOperatorType::NOT_EQUALS) {
      return EvaluateValue(opNode->GetLeft().get()) != EvaluateValue(opNode->GetRight().get());
    }

    if (opNode->GetOperatorType() == BinaryOperatorType::OR) {
      return EvaluateExpression(opNode->GetLeft().get()) || EvaluateExpression(opNode->GetRight().get());
    }

    if (opNode->GetOperatorType() == BinaryOperatorType::AND) {
      return EvaluateExpression(opNode->GetLeft().get()) && EvaluateExpression(opNode->GetRight().get());
    }

    throw ExecutionException("Unexpected node type.");
  }

  void EvaluateStatementChain(const AstNodeStatementChain* node) {
    if (m_shouldTerminate) {
      return;
    }

    const auto& statements = node->GetStatements();
    for (auto iter = statements.begin(), end = statements.end(); !m_shouldTerminate && iter != end; ++iter) {
      EvaluateStatement(iter->get());
    }
  }

  void EvaluateStatementPrint(const AstNodeStatementPrint* node) {
    if (m_shouldTerminate) {
      return;
    }

    m_shouldTerminate = true;
    for (const auto& variable : m_values) {
      std::cout << variable.name << " = " << variable.value << '\n';
    }
  }

  void EvaluateStatementDelete(const AstNodeStatementDelete* node) {
    if (m_shouldTerminate) {
      return;
    }

    const auto& name = node->GetVariableName();
    if (!m_variables.contains(name)) {
      throw ExecutionException("Undefined variable.");
    }

    m_values.erase(m_variables.at(name));
    m_variables.erase(name);
  }

  void EvaluateStatementCall(const AstNodeStatementCall* node) {
    if (m_shouldTerminate) {
      return;
    }

    const auto& name = node->GetFunctionName();
    if (!m_functions.contains(name)) {
      throw ExecutionException("Undefined function.");
    }

    EvaluateStatement(m_functions.at(name).get());
  }

  void EvaluateStatementVariableModification(const AstNodeBinaryStatementVarModification* node) {
    if (m_shouldTerminate) {
      return;
    }

    const auto& varName = node->GetVariableName();
    if (node->GetOperatorType() == ModificationOperatorType::ASSIGN) {
      // Reassignment
      if (m_variables.contains(varName)) {
        m_variables.at(varName)->value = EvaluateValue(node->GetValue().get());
        return;
      }

      // Declaration
      m_variables.emplace(varName, m_values.emplace(m_values.end(), varName, EvaluateValue(node->GetValue().get())));
      return;
    }

    if (!m_variables.contains(varName)) {
      throw ExecutionException("Undefined variable.");
    }

    switch (node->GetOperatorType()) {
      case ModificationOperatorType::ADD: {
        m_variables.at(varName)->value += EvaluateValue(node->GetValue().get());
        return;
      }
      case ModificationOperatorType::SUBTRACT: {
        m_variables.at(varName)->value -= EvaluateValue(node->GetValue().get());
        return;
      }
      case ModificationOperatorType::MULTIPLY: {
        m_variables.at(varName)->value *= EvaluateValue(node->GetValue().get());
        return;
      }
      default: throw ExecutionException("Unexpected node.");
    }
  }

  void EvaluateStatementFunctionDeclaration(const AstNodeStatementFunctionDeclaration* node) {
    if (m_shouldTerminate) {
      return;
    }

    const auto& name = node->GetFunctionName();
    if (m_functions.contains(name)) {
      throw ExecutionException("Function is already defined.");
    }

    m_functions.emplace(name, node->GetCode());
  }

  void EvaluateStatementCondition(const AstNodeStatementCondition* node) {
    if (m_shouldTerminate) {
      return;
    }

    if (EvaluateExpression(node->GetCondition().get())) {
      EvaluateStatement(node->GetCode().get());
    }
  }

  void EvaluateStatementLoop(const AstNodeStatementLoop* node) {
    if (m_shouldTerminate) {
      return;
    }

    // Evaluate a wolf
    int64_t iterator = EvaluateValue(node->GetInitValue().get());
    while (iterator > 0) {
      EvaluateStatement(node->GetCode().get());
      --iterator;
    }
  }

  void EvaluateStatement(const AstNode* node) {
    if (m_shouldTerminate) {
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_CHAIN) {
      EvaluateStatementChain(node->As<AstNodeStatementChain>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_PRINT) {
      EvaluateStatementPrint(node->As<AstNodeStatementPrint>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_DELETE) {
      EvaluateStatementDelete(node->As<AstNodeStatementDelete>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_CALL) {
      EvaluateStatementCall(node->As<AstNodeStatementCall>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_VAR_MODIFICATION) {
      EvaluateStatementVariableModification(node->As<AstNodeBinaryStatementVarModification>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_FUNC_DECL) {
      EvaluateStatementFunctionDeclaration(node->As<AstNodeStatementFunctionDeclaration>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_CONDITION) {
      EvaluateStatementCondition(node->As<AstNodeStatementCondition>());
      return;
    }

    if (node->GetType() == AstNodeType::STATEMENT_LOOP) {
      EvaluateStatementLoop(node->As<AstNodeStatementLoop>());
      return;
    }

    throw ExecutionException("Unexpected node.");
  }

private:
  bool m_shouldTerminate = false;
  // Print should print variables in order
  std::list<Variable> m_values;
  std::unordered_map<std::string, std::list<Variable>::iterator> m_variables;
  std::unordered_map<std::string, std::shared_ptr<AstNode>> m_functions;
};
