#pragma once

#include "TokenizerView.hpp"
#include "TokenRecorder.hpp"
#include "StringUtils.hpp"
#include "Tokens.hpp"

#include <vector>
#include <memory>
#include <sstream>
#include <unordered_set>

struct TokenizationError final : std::exception {
  explicit TokenizationError(size_t line, size_t column, std::string token)
  : line(line)
  , column(column)
  , token(std::move(token)) {
  }

  size_t line;
  size_t column;
  std::string token;
};

struct Tokenizer final {
  explicit Tokenizer(const std::string& input)
  : m_view(input) {
  }

  void Reset() {
    m_view.Reset();
    m_tokens.clear();
  }

  bool Resolve() {
    return ResolveExpression(m_view);
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Token>>& GetTokens() const {
    return m_tokens;
  }

  std::vector<std::unique_ptr<Token>> ReleaseTokens() {
    m_view.Reset();
    std::vector<std::unique_ptr<Token>> result;
    std::swap(result, m_tokens);
    return result;
  }

private:
  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  // Matchers

  static bool MatchWhitespace(const TokenizerView& view) {
    if (!view.HasTokens()) {
      return false;
    }

    return isspace(view.Next());
  }

  static bool MatchIdentifierFirst(const TokenizerView& view) {
    if (!view.HasTokens()) {
      return false;
    }

    char c = view.Next();
    return IsLetter(c) || c == '_';
  }

  static bool MatchIdentifierRepeat(const TokenizerView& view) {
    if (!view.HasTokens()) {
      return false;
    }

    char c = view.Next();
    return IsLetter(c) || isdigit(c) || c == '_';
  }

  static bool MatchBool(const TokenizerView& view) {
    return view.MatchAny("01");
  }

  static bool MatchIdentifier(const TokenizerView& view) {
    return MatchIdentifierFirst(view);
  }

  static bool MatchBraceOpen(const TokenizerView& view) {
    return view.Match('(');
  }

  static bool MatchBraceClose(const TokenizerView& view) {
    return view.Match(')');
  }

  // Symbol operators

  static bool MatchSymbolOperatorNot(const TokenizerView& view) {
    return view.MatchAny("!~");
  }

  static bool MatchSymbolOperatorOr(const TokenizerView& view) {
    return view.Match('|');
  }

  static bool MatchSymbolOperatorXor(const TokenizerView& view) {
    return view.Match('^');
  }

  static bool MatchSymbolOperatorAnd(const TokenizerView& view) {
    return view.MatchAny("&*");
  }

  static bool MatchSymbolOperator(const TokenizerView& view) {
    return MatchSymbolOperatorNot(view) || MatchSymbolOperatorXor(view) || MatchSymbolOperatorOr(view) || MatchSymbolOperatorAnd(view);
  }

  // Word operators

  static bool MatchWordOperatorNot(const TokenizerView& view) {
    return view.MatchAnyCase("not") && FollowWordOperatorNot(view.AtOffset(3));
  }

  static bool MatchWordOperatorOr(const TokenizerView& view) {
    return view.MatchAnyCase("or") && FollowBinaryWordOperator(view.AtOffset(2));
  }

  static bool MatchWordOperatorXor(const TokenizerView& view) {
    return view.MatchAnyCase("xor") && FollowBinaryWordOperator(view.AtOffset(3));
  }

  static bool MatchWordOperatorAnd(const TokenizerView& view) {
    return view.MatchAnyCase("and") && FollowBinaryWordOperator(view.AtOffset(3));
  }

  static bool MatchWordOperator(const TokenizerView& view) {
    return MatchWordOperatorNot(view) || MatchWordOperatorAnd(view) || MatchWordOperatorXor(view) || MatchWordOperatorOr(view);
  }

  // Operators

  static bool MatchOperatorNot(const TokenizerView& view) {
    return MatchSymbolOperatorNot(view) || MatchWordOperatorNot(view);
  }

  static bool MatchOperatorOr(const TokenizerView& view) {
    return MatchSymbolOperatorOr(view) || MatchWordOperatorOr(view);
  }

  static bool MatchOperatorXor(const TokenizerView& view) {
    return MatchSymbolOperatorXor(view) || MatchWordOperatorXor(view);
  }

  static bool MatchOperatorAnd(const TokenizerView& view) {
    return MatchSymbolOperatorAnd(view) || MatchWordOperatorAnd(view);
  }

  static bool MatchOperator(const TokenizerView& view) {
    return MatchOperatorNot(view) || MatchOperatorAnd(view) || MatchOperatorXor(view) || MatchOperatorOr(view);
  }


  // Follow


  static bool FollowBool(const TokenizerView& view) {
    return !view.HasTokens() || MatchBraceClose(view) || MatchOperator(view) || MatchWhitespace(view);
  }

  static bool FollowIdentifyer(const TokenizerView& view) {
    return !view.HasTokens() || MatchBraceClose(view) || MatchOperator(view) || MatchWhitespace(view);
  }

  static bool FollowBinaryWordOperator(const TokenizerView& view) {
    return  MatchSymbolOperatorNot(view) || MatchIdentifier(view) || MatchBool(view) || MatchBraceOpen(view) || MatchWhitespace(view);
  }

  static bool FollowWordOperatorNot(const TokenizerView& view) {
    return MatchIdentifier(view) || MatchBool(view) || MatchBraceOpen(view) || MatchWhitespace(view);
  }

  static bool FollowBinarySymbolOperator(const TokenizerView& view) {
    return MatchOperatorNot(view) || MatchIdentifier(view) || MatchBool(view) || MatchBraceOpen(view) || MatchWhitespace(view);
  }

  static bool FollowSymbolOperatorNot(const TokenizerView& view) {
    return MatchIdentifier(view) || MatchBool(view) || MatchBraceOpen(view) || MatchWhitespace(view);
  }

  static bool FollowBraceOpen(const TokenizerView& view) {
    return MatchWhitespace(view) || MatchOperatorNot(view) || MatchIdentifier(view) || MatchBool(view) || MatchBraceOpen(view);
  }

  static bool FollowBraceClose(const TokenizerView& view) {
    return !view.HasTokens() || MatchOperator(view) || MatchBraceClose(view) || MatchWhitespace(view);
  }


  // Tokenizers


  bool TokenizeBool(TokenizerView& view) {
    if (!MatchBool(view)) {
      return false;
    }

    char c = view.Advance();
    PushToken(std::make_unique<TokenBool>(c == '1'));

    ResolveWhitespace(view);
    if (!FollowBool(view)) {
      throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
    }

    return true;
  }

  bool TokenizeIdentifier(TokenizerView& view) {
    if (!MatchIdentifierFirst(view)) {
      return false;
    }

    TokenRecorder recorder(view);

    std::stringstream ss;
    do {
      ss << view.Advance();
    }
    while (MatchIdentifierRepeat(view));
    std::string result = ss.str();

    if (kReservedKeywords.contains(result)) {
      throw TokenizationError(recorder.GetLine(), recorder.GetColumn(), recorder.GetToken());
    }

    PushToken(std::make_unique<TokenIdentifier>(ss.str()));

    ResolveWhitespace(view);
    if (!FollowIdentifyer(view)) {
      throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
    }

    return true;
  }

  // Braces

  bool TokenizeBraceOpen(TokenizerView& view) {
    if (!MatchBraceOpen(view)) {
      return false;
    }

    view.Advance();
    PushToken(std::make_unique<TokenBraceOpen>());

    ResolveWhitespace(view);
    if (!FollowBraceOpen(view)) {
      throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
    }

    return true;
  }

  bool TokenizeBraceClose(TokenizerView& view) {
    if (!MatchBraceClose(view)) {
      return false;
    }

    view.Advance();
    PushToken(std::make_unique<TokenBraceClose>());

    ResolveWhitespace(view);
    if (!FollowBraceClose(view)) {
      throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
    }

    return true;
  }

  // Operators

  bool TokenizeOperatorNot(TokenizerView& view) {
    if (MatchSymbolOperatorNot(view)) {
      view.Advance();
      PushToken(std::make_unique<TokenOperatorNot>());

      ResolveWhitespace(view);
      if (!FollowSymbolOperatorNot(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    if (MatchWordOperatorNot(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorNot>());

      ResolveWhitespace(view);
      if (!FollowWordOperatorNot(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    return false;
  }

  bool TokenizeOperatorOr(TokenizerView& view) {
    if (MatchSymbolOperatorOr(view)) {
      view.Advance();
      if (MatchSymbolOperatorOr(view)) {
        view.Advance();
      }

      PushToken(std::make_unique<TokenOperatorOr>());

      ResolveWhitespace(view);
      if (!FollowBinarySymbolOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    if (MatchWordOperatorOr(view)) {
      view.Advance(2);
      PushToken(std::make_unique<TokenOperatorOr>());

      ResolveWhitespace(view);
      if (!FollowBinaryWordOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    return false;
  }

  bool TokenizeOperatorXor(TokenizerView& view) {
    if (MatchSymbolOperatorXor(view)) {
      view.Advance();
      PushToken(std::make_unique<TokenOperatorXor>());

      ResolveWhitespace(view);
      if (!FollowBinarySymbolOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    if (MatchWordOperatorXor(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorXor>());

      ResolveWhitespace(view);
      if (!FollowBinaryWordOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    return false;
  }

  bool TokenizeOperatorAnd(TokenizerView& view) {
    if (MatchSymbolOperatorAnd(view)) {
      char c = view.Advance();
      if (c == '&' && view.Match('&')) {
        view.Advance();
      }

      PushToken(std::make_unique<TokenOperatorAnd>());

      ResolveWhitespace(view);
      if (!FollowBinarySymbolOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    if (MatchWordOperatorAnd(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorAnd>());

      ResolveWhitespace(view);
      if (!FollowBinaryWordOperator(view)) {
        throw TokenizationError(view.GetLine(), view.GetColumn(), view.ExtractNextToken());
      }

      return true;
    }

    return false;
  }

  // Resolvers
  static bool ResolveWhitespace(TokenizerView& view) {
    if (!MatchWhitespace(view)) {
      return false;
    }

    do {
      view.Advance();
    }
    while (MatchWhitespace(view));

    return true;
  }

  bool ResolvePrimary(TokenizerView& view) {
    ResolveWhitespace(view);

    while (TokenizeOperatorNot(view)) {
      ResolveWhitespace(view);
    }

    if (TokenizeBool(view) || TokenizeIdentifier(view)) {
      ResolveWhitespace(view);
      return true;
    }

    if (TokenizeBraceOpen(view)) {
      ResolveWhitespace(view);

      if (!ResolveExpression(view)) {
        return false;
      }

      ResolveWhitespace(view);

      if (!TokenizeBraceClose(view)) {
        return false;
      }

      ResolveWhitespace(view);
    }

    return true;
  }

  bool ResolveAndExpression(TokenizerView& view) {
    ResolveWhitespace(view);

    if (!ResolvePrimary(view)) {
      return false;
    }

    ResolveWhitespace(view);

    while (TokenizeOperatorAnd(view)) {
      ResolveWhitespace(view);

      if (!ResolvePrimary(view)) {
        return false;
      }

      ResolveWhitespace(view);
    }

    return true;
  }

  bool ResolveXorExpression(TokenizerView& view) {
    ResolveWhitespace(view);

    if (!ResolveAndExpression(view)) {
      return false;
    }

    ResolveWhitespace(view);

    while (TokenizeOperatorXor(view)) {
      ResolveWhitespace(view);

      if (!ResolveAndExpression(view)) {
        return false;
      }

      ResolveWhitespace(view);
    }

    return true;
  }

  bool ResolveOrExpression(TokenizerView& view) {
    ResolveWhitespace(view);

    if (!ResolveXorExpression(view)) {
      return false;
    }

    ResolveWhitespace(view);

    while (TokenizeOperatorOr(view)) {
      ResolveWhitespace(view);

      if (!ResolveXorExpression(view)) {
        return false;
      }

      ResolveWhitespace(view);
    }

    return true;
  }

  bool ResolveExpression(TokenizerView& view) {
    return ResolveOrExpression(view);
  }

private:
  TokenizerView m_view;
  std::vector<std::unique_ptr<Token>> m_tokens;
  inline static std::unordered_set<std::string> kReservedKeywords = { "and", "xor", "or", "not" };
};
