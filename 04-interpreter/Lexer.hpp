#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "LexerView.hpp"
#include "TokenRecorder.hpp"
#include "Matcher.hpp"
#include "Tokens.hpp"

// TODO will I use it?
// If I will, I will have to rollback 2 states (Tokens + nesting)
// However, I can create a single function for that
// But a single state class won't be enough. Or will it be?
struct TokenStorage final {
  struct State final {
    size_t tokensCount;
  };

  [[nodiscard]] State GetState() const {
    return State(m_tokens.size());
  }

  void SetState(const State& state) {
    // Rollback-only
    assert(state.tokensCount <= m_tokens.size());
    m_tokens.resize(state.tokensCount);
  }

  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Token>>& GetTokens() const {
    return m_tokens;
  }

  std::vector<std::unique_ptr<Token>> ReleaseTokens() {
    std::vector<std::unique_ptr<Token>> result;
    std::swap(m_tokens, result);
    return result;
  }

private:
  std::vector<std::unique_ptr<Token>> m_tokens;
};


// LLLLLLEEEEEEEXX


struct LexerState final {
  size_t tokenCount;
  size_t nestingLevel;
};

struct Lexer final {
  explicit Lexer(std::string_view input)
  : m_view(input)
  , m_nestingLevel(0) {
  }

  bool Tokenize() {
    return ResolveStatementChain(m_view);
  }

  void Reset() {
    m_view.Reset();
    m_tokens.clear();
    m_nestingLevel = 0;
  }

  [[nodiscard]] LexerState GetState() const {
    return LexerState(m_tokens.size(), m_nestingLevel);
  }

  void SetState(const LexerState& state) {
    // Rollback only
    assert(state.tokenCount <= m_tokens.size());
    m_tokens.resize(state.tokenCount);
    m_nestingLevel = state.nestingLevel;
  }

  void Reset(std::string_view input) {
    m_view = LexerView(input);
    m_tokens.clear();
    m_nestingLevel = 0;
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Token>>& GetTokens() const {
    return m_tokens;
  }

  std::vector<std::unique_ptr<Token>> ReleaseTokens() {
    m_view.Reset();
    m_nestingLevel = 0;
    std::vector<std::unique_ptr<Token>> result;
    std::swap(m_tokens, result);
    return result;
  }

private:
  template <typename Type, typename... Args>
  void PushToken(Args... args) requires(std::constructible_from<Type, Args...>) {
    m_tokens.emplace_back(std::make_unique<Type>(args...));
  }

  // If a tokenizer function fails, the position shouldn't change
  // Only top-level functions should handle whitespaces and new lines

  // Вот они все, слева направо
  static constexpr auto IsWS = [](char c) { return c == ' ' || c == '\t' || c == '\v' || c  == '\f' || c == '\r'; };
  static constexpr auto IsNL = [](char c) { return c == '\n'; };
  static constexpr auto IsNotNL = [](char c) { return c != '\n'; };
  static constexpr auto IsSign = [](char c) { return c == '+' || c == '-'; };

  bool ResolveComment(LexerView& view) {
    return Matcher(view).Any("//").ZeroMany(IsNotNL);
  }

  bool ResolveSkip(LexerView& view) {
    return (m_nestingLevel == 0)
    ? (Matcher(view).Any(IsWS, IsNL) || ResolveComment(view))
    : (view.MatchAdvance(IsWS) || ResolveComment(view));
  }

  bool ResolveNumber(LexerView& view) {
    bool isPositive = true;
    return Matcher(view)
    .Perform([this, &isPositive](LexerView& view) {
      if (view.Match(IsSign)) {
        isPositive = view.Next() == '+';
        view.Advance();
      }

      ResolveSkip(view);
    })
    .Record()
    .Many(isdigit)
    .CollectTokenRecorder([this, isPositive](const auto& recorder) {
      try {
        auto view = recorder.GetTokenView();
        int64_t number = std::stoll(std::string(view));
        // TODO possible overflow
        number = isPositive ? number : -number;
        PushToken<TokenNumber>(number);
        return true;
      } catch (const std::exception&) {
        return false;
      }
    });
  }

  bool ResolveIdentifier(LexerView& view) {
    return Matcher(view)
    .Many(isalpha)
    .CollectTokenRecorder([this](const auto& recorder) {
      auto view = recorder.GetTokenView();
      if (Grammar::IsKeyword(view)) {
       return false;
      }

      PushToken<TokenIdentifier>(std::string(view));
      return true;
    });
  }

  template <typename SimpleTokenType>
  bool ResolveSimpleKeyword(std::string_view token, LexerView& view) {
    return Matcher(view).Any(token).Perform([this](LexerView& view) {
      PushToken<SimpleTokenType>();
    });
  }

  bool ResolveValue(LexerView& view) {
    auto state = GetState();
    return ResolveNumber(view)
    || Matcher(view)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenDereference>, this, "$")
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveIdentifier, this)
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveStatementPrint(LexerView& view) {
    return ResolveSimpleKeyword<TokenPrint>("print", view);
  }

  bool ResolveStatementDelete(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenDelete>, this, "delete")
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveIdentifier, this)
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveFragmentCall(LexerView& view) {
    return Matcher(view)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Any('(')
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Any(')')
    .Perform([this](LexerView& view) {
      PushToken<TokenCall>();
    });
  }

  bool ResolveFragmentVariableDeclaration(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenAssign>, this, "=")
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveValue, this)
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveFragmentFunctionDeclaration(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenFunction>, this, "function")
    .Perform([this](LexerView& view) {
      ++m_nestingLevel;
    })
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveStatementChain, this)
    .Perform([this](LexerView& view) {
      --m_nestingLevel;
      PushToken<TokenBlockEnd>();
      // TODO no need for that, if nesting == 0, NL will be consumed by the next statement chain
      // if (m_nestingLevel == 0) {
      //   view.Advance();
      // }
    })
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveFragmentVariableModification(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert([this](LexerView& view) {
      return ResolveSimpleKeyword<TokenAdd>("add", view)
      || ResolveSimpleKeyword<TokenSub>("sub", view)
      || ResolveSimpleKeyword<TokenMult>("mult", view);
    })
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveValue, this)
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveStatementIdentifierBased(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .Assert(&Lexer::ResolveIdentifier, this)
    .Assert([this](LexerView& view) {
      return ResolveFragmentCall(view)
      || ResolveFragmentFunctionDeclaration(view)
      || ResolveFragmentVariableDeclaration(view)
      || ResolveFragmentVariableModification(view);
    })
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveComparison(LexerView& view) {
    return ResolveSimpleKeyword<TokenEqual>("==", view)
    || ResolveSimpleKeyword<TokenNotEqual>("!=", view);
  }

  bool ResolveExpressionPrimary(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .Assert(&Lexer::ResolveValue, this)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveComparison, this)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveValue, this)
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveExpressionAnd(LexerView& view) {
    return Matcher(view)
    .Assert(&Lexer::ResolveExpressionPrimary, this)
    .AssertZeroMany([this](LexerView& view) {
      auto state = GetState();
      return Matcher(view)
      .AssertZeroMany(&Lexer::ResolveSkip, this)
      .Assert(&Lexer::ResolveSimpleKeyword<TokenAnd>, this, "and")
      .AssertZeroMany(&Lexer::ResolveSkip, this)
      .Assert(&Lexer::ResolveExpressionPrimary, this)
      .PerformIfFailed([this, &state](LexerView& view) {
        SetState(state);
      });
    });
  }

  bool ResolveExpressionOr(LexerView& view) {
    return Matcher(view)
    .Assert(&Lexer::ResolveExpressionAnd, this)
    .AssertZeroMany([this](LexerView& view) {
      auto state = GetState();
      return Matcher(view)
      .AssertZeroMany(&Lexer::ResolveSkip, this)
      .Assert(&Lexer::ResolveSimpleKeyword<TokenOr>, this, "or")
      .AssertZeroMany(&Lexer::ResolveSkip, this)
      .Assert(&Lexer::ResolveExpressionAnd, this)
      .PerformIfFailed([this, &state](LexerView& view) {
        SetState(state);
      });
    });
  }

  bool ResolveExpression(LexerView& view) {
    return ResolveExpressionOr(view);
  }

  bool ResolveStatementCondition(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenIf>, this, "if")
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveExpression, this)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenThen>, this, "then")
    .Perform([this](LexerView& view) {
      ++m_nestingLevel;
    })
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveStatementChain, this)
    .Perform([this](LexerView& view) {
      --m_nestingLevel;
      PushToken<TokenBlockEnd>();
    })
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveStatementLoop(LexerView& view) {
    auto state = GetState();
    return Matcher(view)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenLoop>, this, "loop")
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveValue, this)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveSimpleKeyword<TokenDo>, this, "do")
    .Perform([this](LexerView& view) {
      ++m_nestingLevel;
    })
    .AssertMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveStatementChain, this)
    .Perform([this](LexerView& view) {
      --m_nestingLevel;
      PushToken<TokenBlockEnd>();
    })
    .PerformIfFailed([this, &state](LexerView& view) {
      SetState(state);
    });
  }

  bool ResolveStatement(LexerView& view) {
    return ResolveStatementPrint(view)
    || ResolveStatementDelete(view)
    || ResolveStatementIdentifierBased(view)
    || ResolveStatementCondition(view)
    || ResolveStatementLoop(view);
  }

  bool ResolveStatementChain(LexerView& view) {
    return Matcher(view)
    .AssertZeroMany(&Lexer::ResolveSkip, this)
    .Assert(&Lexer::ResolveStatement, this)
    .AssertZeroMany(&Lexer::ResolveStatementChain, this);
  }

private:
  LexerView m_view;
  std::vector<std::unique_ptr<Token>> m_tokens;
  size_t m_nestingLevel;
};
