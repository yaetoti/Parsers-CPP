#pragma once

#include "TokenizerContext.hpp"

// Grammar
// Expression = (Expression) Repeat
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

// Match functions (FIRST())

// Simple matchers (terminal + number, as it's equivalent to a digit matcher)

inline bool MatchNumber(const TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  return isdigit(ctx.Next());
}

inline bool MatchWhitespace(const TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  return isspace(ctx.Next());
}

inline bool MatchOperator(const TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  char c = ctx.Next();
  return c == '+' || c == '-' || c == '*' || c == '/';
}

inline bool MatchBraceOpen(const TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  return ctx.Next() == '(';
}

inline bool MatchBraceClose(const TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  return ctx.Next() == ')';
}

// Non-terminal matchers

// Allowed to not exist
inline bool MatchRepeat(const TokenizerContext& ctx) {
  return MatchOperator(ctx);
}

inline bool MatchExpression(const TokenizerContext& ctx) {
  return MatchBraceOpen(ctx) || MatchNumber(ctx);
}


// Resolvers


inline bool TokenizeNumber(TokenizerContext& ctx) {
  if (!MatchNumber(ctx)) {
    return false;
  }

  std::stringstream ss;
  ss << ctx.Advance();

  while (MatchNumber(ctx)) {
    ss << ctx.Advance();
  }

  try {
    std::string result = ss.str();
    int64_t number = std::stoll(result);
    ctx.PushToken(std::make_unique<TokenNumber>(result, number));
  } catch (const std::exception&) {
    return false;
  }

  return true;
}

inline bool TokenizeOperator(TokenizerContext& ctx) {
  if (!MatchOperator(ctx)) {
    return false;
  }

  char c = ctx.Advance();
  switch (c) {
    case '+': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::ADD));
      break;
    }
    case '-': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::SUBTRACT));
      break;
    }
    case '*': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::MULTIPLY));
      break;
    }
    case '/': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::DIVIDE));
      break;
    }
    default: {
      // WTF
      return false;
    }
  }

  return true;
}

inline bool TokenizeBraceOpen(TokenizerContext& ctx) {
  if (!MatchBraceOpen(ctx)) {
    return false;
  }

  ctx.Advance();
  ctx.PushToken(std::make_unique<TokenBraceOpen>(std::string { '(' }));
  return true;
}

inline bool TokenizeBraceClose(TokenizerContext& ctx) {
  if (!MatchBraceClose(ctx)) {
    return false;
  }

  ctx.Advance();
  ctx.PushToken(std::make_unique<TokenBraceClose>(std::string { ')' }));
  return true;
}

inline bool ResolveWhitespace(TokenizerContext& ctx) {
  if (!MatchWhitespace(ctx)) {
    return false;
  }

  do {
    ctx.Advance();
  }
  while (MatchWhitespace(ctx));

  return true;
}

inline bool ResolveExpression(TokenizerContext& ctx);

inline bool ResolveRepeat(TokenizerContext& ctx) {
  ResolveWhitespace(ctx);

  // Zero-match
  if (!MatchOperator(ctx)) {
    return true;
  }

  // Operator Expression
  // Operator Expression Repeat
  do {
    if (!TokenizeOperator(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);

    if (!ResolveExpression(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);
  } while (MatchOperator(ctx));

  return true;
}

inline bool ResolveExpression(TokenizerContext& ctx) {
  ResolveWhitespace(ctx);

  if (!MatchExpression(ctx)) {
    return false;
  }

  // Expression = Number Repeat
  if (TokenizeNumber(ctx)) {
    ResolveWhitespace(ctx);

    if (!ResolveRepeat(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);

    return true;
  }

  // Expression = (Expression) Repeat
  if (TokenizeBraceOpen(ctx)) {
    ResolveWhitespace(ctx);

    if (!ResolveExpression(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);

    if (!TokenizeBraceClose(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);

    if (!ResolveRepeat(ctx)) {
      return false;
    }

    ResolveWhitespace(ctx);

    return true;
  }

  return false;
}
