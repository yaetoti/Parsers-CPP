#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>

#include "AstNodes.hpp"
#include "DynamicBitset.hpp"
#include "TruthTable.hpp"

// Interpretation:
//
// Analyze expression, create parameter vector/map, sort parameters by name
// After that we can evaluate an expression or generate a truth table
// We can create a separate class for generation which will get parameters and evaluate for each
//
// How will we iterate over values? Create a vector of bools of size 0 - N. After each iteration perform manual binary increment. When we reached max value - end
// using size_t will give maximum 64 parameters. Enough? I don't think so >:O (Their evaluation will take forewer anyway)

struct Interpreter final {
  void SetExpression(std::shared_ptr<AstNode> root) {
    m_root = std::move(root);
    // Get names
    std::unordered_set<std::string> names;
    CollectParameterNames(m_root.get(), names);
    // Sort names
    m_parameterNames = std::vector<std::string>(names.begin(), names.end());
    std::ranges::sort(m_parameterNames);
    // Create mapping
    m_nameIndexTable.clear();
    for (size_t i = 0; i < m_parameterNames.size(); ++i) {
      m_nameIndexTable.emplace(m_parameterNames.at(i), i);
    }
  }

  TruthTable GenerateTruthTable() {
    assert(m_root);

    TruthTable table(m_parameterNames);
    std::vector<bool> values(m_parameterNames.size());

    while (true) {
      bool result = EvaluateNode(m_root.get(), values);
      table.EmplaceRecord(std::make_unique<TruthTableRecord>(values, result));

      if (VectorBitset::AllSet(values)) {
        return table;
      }

      VectorBitset::Increment(values);

      // if (m_bitset.AllSet()) {
      //   return table;
      // }
      //
      // ++m_bitset;
    }
  }

  bool Evaluate(const std::vector<bool>& parameters) {
    assert(parameters.size() == m_parameterNames.size());
    return EvaluateNode(m_root.get(), parameters);
  }

private:
  void CollectParameterNames(const AstNode* node, std::unordered_set<std::string>& names) {
    if (node->GetType() == AstNodeType::PARAMETER) {
      names.emplace(dynamic_cast<const AstNodeParameter*>(node)->GetName());
      return;
    }

    if (node->GetType() == AstNodeType::UNARY_OPERATOR_NOT) {
      CollectParameterNames(dynamic_cast<const AstNodeUnaryOperatorNot*>(node)->GetChild().get(), names);
      return;
    }

    if (node->GetType() == AstNodeType::BINARY_OPERATOR) {
      auto opNode = dynamic_cast<const AstNodeBinaryOperator*>(node);
      CollectParameterNames(opNode->GetLeft().get(), names);
      CollectParameterNames(opNode->GetRight().get(), names);
      return;
    }
  }

  bool EvaluateNode(const AstNode* node, const std::vector<bool>& parameters) {
    if (node->GetType() == AstNodeType::BOOL) {
      return node->As<AstNodeBool>()->GetValue();
    }

    if (node->GetType() == AstNodeType::PARAMETER) {
      return parameters.at(m_nameIndexTable.at(node->As<AstNodeParameter>()->GetName()));
    }

    if (node->GetType() == AstNodeType::UNARY_OPERATOR_NOT) {
      return !EvaluateNode(node->As<AstNodeUnaryOperatorNot>()->GetChild().get(), parameters);
    }

    if (node->GetType() == AstNodeType::BINARY_OPERATOR) {
      auto opNode = node->As<AstNodeBinaryOperator>();
      bool leftResult = EvaluateNode(opNode->GetLeft().get(), parameters);
      bool rightResult = EvaluateNode(opNode->GetRight().get(), parameters);

      switch (opNode->GetOperatorType()) {
        case BinaryOperatorType::AND: {
          return leftResult && rightResult;
        }
        case BinaryOperatorType::XOR: {
          return leftResult ^ rightResult;
        }
        case BinaryOperatorType::OR: {
          return leftResult || rightResult;
        }
        default: {
          // WTF
          return false;
        }
      }
    }

    // WTF
    return false;
  }

private:
  std::shared_ptr<AstNode> m_root;
  std::vector<std::string> m_parameterNames;
  std::unordered_map<std::string, size_t> m_nameIndexTable;
  // DynamicBitset m_bitset;
};
