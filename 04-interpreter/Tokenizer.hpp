#pragma once

#include "LexerView.hpp"
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
  explicit Tokenizer(std::string_view input)
  : m_view(input) {
  }

  void Reset() {
    m_view.Reset();
    m_tokens.clear();
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

  bool Resolve() {
    return ResolveExpression(m_view);
  }

private:
  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  // First

  static constexpr auto IsIdentifierFirst = [](char c) { return isalpha(c) || c == '_'; };
  static constexpr auto IsIdentifierRepeat = [](char c) { return isalnum(c) || c == '_'; };
  static constexpr auto IsIdentifier = IsIdentifierFirst;
  static constexpr auto IsBool = [](char c) { return MatchAny(c, "01"); };

  static constexpr auto IsBraceOpen = [](char c) { return c == '('; };
  static constexpr auto IsBraceClose = [](char c) { return c == ')'; };

  // Symbol operators
  static constexpr auto IsSymbolOperatorNot = [](char c) { return MatchAny(c, "!~"); };
  static constexpr auto IsSymbolOperatorOr = [](char c) { return c == '|'; };
  static constexpr auto IsSymbolOperatorXor = [](char c) { return c == '^'; };
  static constexpr auto IsSymbolOperatorAnd = [](char c) { return MatchAny(c, "&*"); };

  // Word operators
  static constexpr auto IsFollowWordOperator = [](char c) { return isspace(c) || IsBraceOpen(c) || IsSymbolOperatorNot(c); };

  static bool IsWordOperatorNot(const LexerView& view) {
    return view.MatchIgnoreCase("not") && (view.HasTokens(4) || view.AtOffset(3).Match(IsFollowWordOperator));
  }

  static bool IsWordOperatorOr(const LexerView& view) {
    return view.MatchIgnoreCase("or") && (view.HasTokens(3) || view.AtOffset(3).Match(IsFollowWordOperator));
  }

  static bool IsWordOperatorXor(const LexerView& view) {
    return view.MatchIgnoreCase("xor") && (view.HasTokens(4) || view.AtOffset(3).Match(IsFollowWordOperator));
  }

  static bool IsWordOperatorAnd(const LexerView& view) {
    return view.MatchIgnoreCase("and") && (view.HasTokens(4) || view.AtOffset(3).Match(IsFollowWordOperator));
  }

  // Tokenizers

  bool TokenizeBool(LexerView& view) {
    if (!view.Match(IsBool)) {
      return false;
    }

    char c = view.Advance();
    PushToken(std::make_unique<TokenBool>(c == '1'));
    return true;
  }

  bool TokenizeIdentifier(LexerView& view) {
    if (!view.Match(IsIdentifierFirst)) {
      return false;
    }

    size_t start = view.GetPosition();
    view.AdvanceWhile(IsIdentifierRepeat);
    std::string result = view.ExtractString(start, view.GetPosition() - start);

    if (kReservedKeywords.contains(result)) {
      return false;
    }

    PushToken(std::make_unique<TokenIdentifier>(result));
    return true;
  }

  // Braces

  bool TokenizeBraceOpen(LexerView& view) {
    if (!view.Match(IsBraceOpen)) {
      return false;
    }

    view.Advance();
    PushToken(std::make_unique<TokenBraceOpen>());
    return true;
  }

  bool TokenizeBraceClose(LexerView& view) {
    if (!view.Match(IsBraceClose)) {
      return false;
    }

    view.Advance();
    PushToken(std::make_unique<TokenBraceClose>());
    return true;
  }

  // Operators

  bool TokenizeOperatorNot(LexerView& view) {
    if (view.Match(IsSymbolOperatorNot)) {
      view.Advance();
      PushToken(std::make_unique<TokenOperatorNot>());
      return true;
    }

    if (IsWordOperatorNot(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorNot>());
      return true;
    }

    return false;
  }

  bool TokenizeOperatorOr(LexerView& view) {
    if (view.Match(IsSymbolOperatorOr)) {
      view.Advance();
      if (view.Match(IsSymbolOperatorOr)) {
        view.Advance();
      }

      PushToken(std::make_unique<TokenOperatorOr>());
      return true;
    }

    if (IsWordOperatorOr(view)) {
      view.Advance(2);
      PushToken(std::make_unique<TokenOperatorOr>());
      return true;
    }

    return false;
  }

  bool TokenizeOperatorXor(LexerView& view) {
    if (view.Match(IsSymbolOperatorXor)) {
      view.Advance();
      PushToken(std::make_unique<TokenOperatorXor>());
      return true;
    }

    if (IsWordOperatorXor(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorXor>());
      return true;
    }

    return false;
  }

  bool TokenizeOperatorAnd(LexerView& view) {
    if (view.Match(IsSymbolOperatorAnd)) {
      char c = view.Advance();
      if (c == '&' && view.Match('&')) {
        view.Advance();
      }

      PushToken(std::make_unique<TokenOperatorAnd>());
      return true;
    }

    if (IsWordOperatorAnd(view)) {
      view.Advance(3);
      PushToken(std::make_unique<TokenOperatorAnd>());
      return true;
    }

    return false;
  }

  // Resolvers

  static bool ResolveWhitespace(LexerView& view) {
    return !view.AdvanceExtract(isspace).empty();
  }

  bool ResolvePrimary(LexerView& view) {
    ResolveWhitespace(view);
    if (TokenizeBool(view) || TokenizeIdentifier(view)) {
      ResolveWhitespace(view);
      return true;
    }
    // andnot0
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

  bool ResolveAndExpression(LexerView& view) {
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

  bool ResolveXorExpression(LexerView& view) {
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

  bool ResolveOrExpression(LexerView& view) {
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

  bool ResolveExpression(LexerView& view) {
    return ResolveOrExpression(view);
  }

private:
  LexerView m_view;
  std::vector<std::unique_ptr<Token>> m_tokens;
  inline static std::unordered_set<std::string> kReservedKeywords = { "and", "xor", "or", "not" };
};
