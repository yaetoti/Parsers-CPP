#pragma once

#include "AstNodes.hpp"
#include "ParserView.hpp"

struct Parser final {
  explicit Parser(const std::vector<std::unique_ptr<Token>>& tokens)
  : m_view(tokens) {
  }

  void Reset() {
    m_view.Reset();
  }

  std::shared_ptr<AstNode> Parse() {
    return ParseExpression(m_view);
  }

private:
  std::shared_ptr<AstNode> ParsePrimary(ParserView& view) {
    // Not Primary
    if (view.MatchType(TokenType::OPERATOR_NOT)) {
      view.Advance();

      auto innerNode = ParsePrimary(view);
      if (!innerNode) {
        return nullptr;
      }

      return std::make_shared<AstNodeUnaryOperatorNot>(std::move(innerNode));
    }

    // Identifier | Bool | (Expression)
    if (view.MatchType(TokenType::IDENTIFIER)) {
      return std::make_shared<AstNodeParameter>(view.AdvanceAs<TokenIdentifier>()->GetName());
    }

    if (view.MatchType(TokenType::BOOL)) {
      return std::make_shared<AstNodeBool>(view.AdvanceAs<TokenBool>()->GetValue());
    }

    if (view.MatchType(TokenType::BRACE_OPEN)) {
      view.Advance();

      auto innerNode = ParseExpression(view);
      if (!innerNode) {
        return nullptr;
      }

      if (!view.MatchType(TokenType::BRACE_CLOSE)) {
        return nullptr;
      }
      view.Advance();

      return innerNode;
    }

    return nullptr;
  }

  std::shared_ptr<AstNode> ParseAnd(ParserView& view) {
    auto leftNode = ParsePrimary(view);
    if (!leftNode) {
      return nullptr;
    }

    while (view.MatchType(TokenType::OPERATOR_AND)) {
      view.Advance();
      auto rightNode = ParsePrimary(view);
      if (!rightNode) {
        return nullptr;
      }

      leftNode = std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::AND, std::move(leftNode), std::move(rightNode));
    }

    return leftNode;
  }

  std::shared_ptr<AstNode> ParseXor(ParserView& view) {
    auto leftNode = ParseAnd(view);
    if (!leftNode) {
      return nullptr;
    }

    while (view.MatchType(TokenType::OPERATOR_XOR)) {
      view.Advance();
      auto rightNode = ParseAnd(view);
      if (!rightNode) {
        return nullptr;
      }

      leftNode = std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::XOR, std::move(leftNode), std::move(rightNode));
    }

    return leftNode;
  }

  std::shared_ptr<AstNode> ParseOr(ParserView& view) {
    auto leftNode = ParseXor(view);
    if (!leftNode) {
      return nullptr;
    }

    while (view.MatchType(TokenType::OPERATOR_OR)) {
      view.Advance();
      auto rightNode = ParseXor(view);
      if (!rightNode) {
        return nullptr;
      }

      leftNode = std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::OR, std::move(leftNode), std::move(rightNode));
    }

    return leftNode;
  }

  std::shared_ptr<AstNode> ParseExpression(ParserView& view) {
    return ParseOr(view);
  }

private:
  ParserView m_view;
};
