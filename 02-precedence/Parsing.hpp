#pragma once

#include "AstNodes.hpp"
#include "ParserContext.hpp"
#include "SpanUtils.hpp"

// Grammar
// Expression = (Expression Repeat)
// Expression = Number Repeat
//
// Repeat = Operator Expression
// Repeat = Operator Expression Repeat
// Repeat = $
//
// Number = Digit
// Number = Digit Number
//
// Operator = '+' | '-' | '*' | '/'
// Digit = 0..9
//
//
//
// FOLLOW(Expression) = Repeat = Operator | ')' | $
// FOLLOW(Repeat) = ')' | $
// FOLLOW(Number) = Operator | ')' | $
// FOLLOW(Operator) = Expression = '(' | Number
//
// FIRST(Expression) = '(' | Number
// FIRST(Repeat) = Operator | $
// FIRST(Number) = Digit

// Update
// I need to transform this grammar using precedence climbing technique to ensure simplisity and implicit priority handling
// https://en.wikipedia.org/wiki/Operator-precedence_parser
// It's enough for tokenizing, but it doesn't consider priority, so it delegates this work to parser's code, which introduces mess and extra context fields
//
// Expression = AddSub
// AddSub = MulDiv ( ('+' | '-') MulDiv )*
// MulDiv = Primary ( ('*' | '/') Primary )*
// Primary = Number | '(' Expression ')'
//
// Each level is iterative (left-associative), but every component recurses (right-associative) to another level in a cycle.
// And it works because we are assuming that before and after each +- there may be MulDiv

// Building AST

inline std::shared_ptr<AstNode> ParseNumber(ParserContext& ctx) {
  if (!ctx.MatchType(TokenType::NUMBER)) {
    return nullptr;
  }

  auto token = dynamic_cast<const TokenNumber*>(ctx.Advance());
  return std::make_shared<AstNodeNumber>(token->GetNumber());
}

// ParseNumber
// ParseOperator
// ParseBraceOpen
// ParseBraceClose

// ParseRepeat
// ParseExpression

inline bool ConsumeBraceOpen(ParserContext& ctx) {
  if (!ctx.MatchType(TokenType::BRACE_OPEN)) {
    return false;
  }

  ctx.Advance();
  return true;
}

inline bool ConsumeBraceClose(ParserContext& ctx) {
  if (!ctx.MatchType(TokenType::BRACE_CLOSE)) {
    return false;
  }

  ctx.Advance();
  return true;
}

inline std::shared_ptr<AstNode> ParseExpression(ParserContext& ctx);

inline std::shared_ptr<AstNode> ParsePrimary(ParserContext& ctx) {
  if (ctx.IsEnd()) {
    return nullptr;
  }

  if (ctx.Next()->GetType() == TokenType::NUMBER) {
    return ParseNumber(ctx);
  }

  if (ConsumeBraceOpen(ctx)) {
    auto result = ParseExpression(ctx);
    if (result == nullptr) {
      return nullptr;
    }

    if (!ConsumeBraceClose(ctx)) {
      return nullptr;
    }

    return result;
  }

  return nullptr;
}

inline std::shared_ptr<AstNode> ParseMulDiv(ParserContext& ctx) {
  auto left = ParsePrimary(ctx);
  if (!left) {
    return nullptr;
  }

  while (ctx.MatchType(TokenType::OPERATOR)) {
    auto opToken = dynamic_cast<const TokenOperator*>(ctx.Next());
    if (opToken->GetOperatorType() != OperatorType::MULTIPLY && opToken->GetOperatorType() != OperatorType::DIVIDE) {
      break;
    }
    ctx.Advance();

    auto right = ParsePrimary(ctx);
    if (!right) {
      return nullptr;
    }

    left = std::make_shared<AstNodeOperator>(
      opToken->GetOperatorType(),
      std::move(left),
      std::move(right)
    );
  }

  return left;
}

inline std::shared_ptr<AstNode> ParseAddSub(ParserContext& ctx) {
  auto left = ParseMulDiv(ctx);
  if (!left) {
    return nullptr;
  }

  while (ctx.MatchType(TokenType::OPERATOR)) {
    auto opToken = dynamic_cast<const TokenOperator*>(ctx.Next());
    if (opToken->GetOperatorType() != OperatorType::ADD && opToken->GetOperatorType() != OperatorType::SUBTRACT) {
      break;
    }
    ctx.Advance();

    auto right = ParseMulDiv(ctx);
    if (!right) {
      return nullptr;
    }

    left = std::make_shared<AstNodeOperator>(
      opToken->GetOperatorType(),
      std::move(left),
      std::move(right)
    );
  }

  return left;
}

inline std::shared_ptr<AstNode> ParseExpression(ParserContext& ctx) {
  return ParseAddSub(ctx);
}

// Evaluating AST

inline int64_t EvaluateAst(const AstNode* node) {
  if (node->GetType() == AstNodeType::NUMBER) {
    return dynamic_cast<const AstNodeNumber*>(node)->GetNumber();
  }

  auto opNode = dynamic_cast<const AstNodeOperator*>(node);
  switch (opNode->GetOperatorType()) {
    case OperatorType::ADD: {
      return EvaluateAst(opNode->GetLeft().get()) + EvaluateAst(opNode->GetRight().get());
    }
    case OperatorType::SUBTRACT: {
      return EvaluateAst(opNode->GetLeft().get()) - EvaluateAst(opNode->GetRight().get());
    }
    case OperatorType::MULTIPLY: {
      return EvaluateAst(opNode->GetLeft().get()) * EvaluateAst(opNode->GetRight().get());
    }
    case OperatorType::DIVIDE: {
      return EvaluateAst(opNode->GetLeft().get()) / EvaluateAst(opNode->GetRight().get());
    }
    default: {
      // WTF
      return 0;
    }
  }
}
