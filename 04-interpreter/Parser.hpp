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
    return ParseStatementChain(m_view);
  }

private:
  static std::shared_ptr<AstNode> ParseValue(ParserView& view) {
    if (view.Match(TokenType::NUMBER)) {
      const auto* token = view.Advance<TokenNumber>();
      return std::make_shared<AstNodeValueNumber>(token->GetValue());
    }

    auto state = view.GetState();
    if (view.MatchAdvance(TokenType::OP_DEREFERENCE)) {
      if (!view.Match(TokenType::IDENTIFIER)) {
        view.SetState(state);
        return nullptr;
      }

      const auto* token = view.Advance<TokenIdentifier>();
      return std::make_shared<AstNodeValueIdentifier>(token->GetName());
    }

    return nullptr;
  }

  static std::shared_ptr<AstNode> ParseStatementDelete(ParserView& view) {
    auto state = view.GetState();
    if (!view.MatchAdvance(TokenType::KEYWORD_DELETE)) {
      return nullptr;
    }

    if (!view.Match(TokenType::IDENTIFIER)) {
      return nullptr;
    }

    const auto* token = view.Advance<TokenIdentifier>();
    return std::make_shared<AstNodeStatementDelete>(token->GetName());
  }

  static std::shared_ptr<AstNode> ParseStatementPrint(ParserView& view) {
    if (!view.MatchAdvance(TokenType::KEYWORD_PRINT)) {
      return nullptr;
    }

    return std::make_shared<AstNodeStatementPrint>();
  }

  static std::shared_ptr<AstNode> ParseFragmentCall(ParserView& view, const TokenIdentifier* identidier) {
    if (!view.MatchAdvance(TokenType::OP_CALL)) {
      return nullptr;
    }

    return std::make_shared<AstNodeStatementCall>(identidier->GetName());
  }

  static std::shared_ptr<AstNode> ParseFragmentVariableModification(ParserView& view, const TokenIdentifier* identidier) {
    if (!view.Match(TokenType::OP_ASSIGN, TokenType::KEYWORD_ADD, TokenType::KEYWORD_SUB, TokenType::KEYWORD_MULT)) {
      return nullptr;
    }

    auto state = view.GetState();
    ModificationOperatorType opType;
    switch (view.Advance()->GetType()) {
      case TokenType::OP_ASSIGN:
        opType = ModificationOperatorType::ASSIGN;
        break;
      case TokenType::KEYWORD_ADD:
        opType = ModificationOperatorType::ADD;
        break;
      case TokenType::KEYWORD_SUB:
        opType = ModificationOperatorType::SUBTRACT;
        break;
      case TokenType::KEYWORD_MULT:
        opType = ModificationOperatorType::MULTIPLY;
        break;
      default: {
        // WTF
        view.SetState(state);
        return nullptr;
      }
    }

    auto nodeValue = ParseValue(view);
    if (!nodeValue) {
      view.SetState(state);
      return nullptr;
    }

    return std::make_shared<AstNodeBinaryStatementVarModification>(
      opType,
      identidier->GetName(),
      std::move(nodeValue)
    );
  }

  static std::shared_ptr<AstNode> ParseFragmentFunctionDeclaration(ParserView& view, const TokenIdentifier* identidier) {
    auto state = view.GetState();
    if (!view.MatchAdvance(TokenType::KEYWORD_FUNCTION)) {
      return nullptr;
    }

    auto nodeStatementChain = ParseStatementChain(view);
    if (!nodeStatementChain) {
      view.SetState(state);
      return nullptr;
    }

    if (!view.MatchAdvance(TokenType::BLOCK_END)) {
      view.SetState(state);
      return nullptr;
    }

    return std::make_shared<AstNodeStatementFunctionDeclaration>(identidier->GetName(), std::move(nodeStatementChain));
  }

  static std::shared_ptr<AstNode> ParseStatementIdentifierBased(ParserView& view) {
    auto state = view.GetState();
    if (!view.Match(TokenType::IDENTIFIER)) {
      return nullptr;
    }

    const auto* identifier = view.Advance<TokenIdentifier>();
    if (auto node = ParseFragmentCall(view, identifier)) {
      return node;
    }

    if (auto node = ParseFragmentVariableModification(view, identifier)) {
      return node;
    }

    if (auto node = ParseFragmentFunctionDeclaration(view, identifier)) {
      return node;
    }

    view.SetState(state);
    return nullptr;
  }

  static std::shared_ptr<AstNode> ParseExpressionPrimary(ParserView& view) {
    auto state = view.GetState();
    auto leftNode = ParseValue(view);
    if (!leftNode) {
      return nullptr;
    }

    if (!view.Match(TokenType::OP_EQUAL, TokenType::OP_NOT_EQUAL)) {
      view.SetState(state);
      return nullptr;
    }
    const auto* opToken = view.Advance();

    auto rightNode = ParseValue(view);
    if (!rightNode) {
      view.SetState(state);
      return nullptr;
    }

    if (opToken->GetType() == TokenType::OP_EQUAL) {
      return std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::EQUALS, std::move(leftNode), std::move(rightNode));
    }

    return std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::NOT_EQUALS, std::move(leftNode), std::move(rightNode));
  }

  static std::shared_ptr<AstNode> ParseExpressionAnd(ParserView& view) {
    auto leftNode = ParseExpressionPrimary(view);
    if (!leftNode) {
      return nullptr;
    }

    while (view.MatchAdvance(TokenType::OP_AND)) {
      // We don't need state, actually... Because we can disambiguate grammar by introducing fragments
      auto rightNode = ParseExpressionPrimary(view);
      if (!rightNode) {
        return nullptr;
      }

      leftNode = std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::AND, std::move(leftNode), std::move(rightNode));
    }

    return leftNode;
  }

  static std::shared_ptr<AstNode> ParseExpressionOr(ParserView& view) {
    auto leftNode = ParseExpressionAnd(view);
    if (!leftNode) {
      return nullptr;
    }

    while (view.MatchAdvance(TokenType::OP_OR)) {
      auto rightNode = ParseExpressionAnd(view);
      if (!rightNode) {
        return nullptr;
      }

      leftNode = std::make_shared<AstNodeBinaryOperator>(BinaryOperatorType::OR, std::move(leftNode), std::move(rightNode));
    }

    return leftNode;
  }

  static std::shared_ptr<AstNode> ParseStatementCondition(ParserView& view) {
    auto state = view.GetState();
    if (!view.MatchAdvance(TokenType::KEYWORD_IF)) {
      return nullptr;
    }

    auto condition = ParseExpressionOr(view);
    if (!condition) {
      view.SetState(state);
      return nullptr;
    }

    if (!view.MatchAdvance(TokenType::KEYWORD_THEN)) {
      view.SetState(state);
      return nullptr;
    }

    auto code = ParseStatementChain(view);
    if (!code) {
      view.SetState(state);
      return nullptr;
    }

    if (!view.MatchAdvance(TokenType::BLOCK_END)) {
      view.SetState(state);
      return nullptr;
    }

    return std::make_shared<AstNodeStatementCondition>(std::move(condition), std::move(code));
  }

  static std::shared_ptr<AstNode> ParseStatementLoop(ParserView& view) {
    auto state = view.GetState();
    if (!view.MatchAdvance(TokenType::KEYWORD_LOOP)) {
      return nullptr;
    }

    auto value = ParseValue(view);
    if (!value) {
      view.SetState(state);
      return nullptr;
    }

    if (!view.MatchAdvance(TokenType::KEYWORD_DO)) {
      view.SetState(state);
      return nullptr;
    }

    auto code = ParseStatementChain(view);
    if (!code) {
      view.SetState(state);
      return nullptr;
    }

    if (!view.MatchAdvance(TokenType::BLOCK_END)) {
      view.SetState(state);
      return nullptr;
    }

    return std::make_shared<AstNodeStatementLoop>(std::move(value), std::move(code));
  }

/*
  AstNodeStatementChain: (vector<AstNode> Statements)

  StatementChain = Skip* Statement StatementChain*
  Statement = StatementPrint
  | StatementVarDelete
  | StatementIdentifierBased
  | StatementCondition
  | StatementLoop
*/

  static std::shared_ptr<AstNode> ParseStatementChain(ParserView& view) {
    std::vector<std::shared_ptr<AstNode>> statements;

    while (true) {
      if (auto statement = ParseStatementPrint(view)) {
        statements.emplace_back(std::move(statement));
        continue;
      }

      if (auto statement = ParseStatementDelete(view)) {
        statements.emplace_back(std::move(statement));
        continue;
      }

      if (auto statement = ParseStatementIdentifierBased(view)) {
        statements.emplace_back(std::move(statement));
        continue;
      }

      if (auto statement = ParseStatementCondition(view)) {
        statements.emplace_back(std::move(statement));
        continue;
      }

      if (auto statement = ParseStatementLoop(view)) {
        statements.emplace_back(std::move(statement));
        continue;
      }

      break;
    }

    return std::make_shared<AstNodeStatementChain>(std::move(statements));
  }

private:
  ParserView m_view;
};
