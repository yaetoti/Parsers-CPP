#pragma once
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "LexerView.hpp"
#include "TokenRecorder.hpp"
#include "Tokens.hpp"

// loop $x do x//comment <nl> add 2 - INVALID (expression is not finished)
// loop $x do//comment <nl> x add 2 - INVALID (no expression in a block)
// loop $x do loop//comment $x do <nl> x add 2 - INVALID (expression is not finished)
// loop $x//comment <nl>do x add 2 - OK (loop body haven't started yet)
// loop $x do x add 2//comment <nl> - OK (expression was finished, end of loop body)
// loop $x do print//comment <nl> - OK (expression was finished, end of loop body)

// Comments shouldn't consume NL

// What would I want?
// loop $x do print

// Matcher().Match("loop").Match(Value).Match("do").Match(Matcher.any(Statement | Condition | FunctionDeclaration))
// Print:
// ChainLexer(view).Consume("print").

// Token: TokenPrint, Match: "print"

// FunctionDeclaration = Identifier WS_NL* "function" WS* Statement NL
// Print = "print"
// NL = '\n'
// WS = '\t' | ' '
// WS_NL = WS | NL

/*
  Variables: nestingLevel, isInExpression

  nesting == 0
    inWS = Space, NewLine, Comment
    outWS = inWS

  nesting > 0
    inWS = Space
    outWS = Space, Comment
 */

/*
Task: Complex identifier with different rules for the first and the following letters

Identifier = FirstSymbol FollowSymbol*
FollowSymbol = FirstSymbol | '_' | '0'..'9'
FirstSymbol = 'a'..'z' | 'A'..'Z'

My answer:

Problem: First symbol is different from the others, so we can't fit everything into 1 matcher, however, the entire word should be saved
Solution: Replace Match() and ConsumeAdvance() with separate calls to Consume() and Apply(). Every Consume() extends length of token, stored in Matcher. Matcher becomes a state machine, essentially. When calling Apply(), we supply predicate with the entire consumed token
Create operator bool() for matcher that will check it's state.

IsFirstSymbol = [](...) { ... }
IsFollowSymbol = [](...) { ... }
AddIdentifierToken = [](..., Context ctx, string_view str) { ...; ctx->PushToken(TokenIdentifier(str))}

return Matcher().Consume(IsFirstSymbol).Consume(IsFollowSymbol).Apply(AddIdentifierToken);
*/

/*
Expression = '(' NestedExpression ')'
NestedExpression = Identifier
NestedExpression = Identifier NestedExpression // Here is the problem - ambiguity for Identifier
NestedExpression = Expression
Identifier = 'a'..'z' | 'A'..'Z'

bool ResolveIdentifier(LexerView& view) {
  return Matcher(view).Consume(isalpha).Apply([](){ ctx->PushToken(Identifier(str)) });
}

// Another problem was that we already wrote a ResolveIdentifier.
// Reusing it in a C++ was would introduce the same complexities, with ifs and view.GetState() view.RollbackState(state)
// Solution:
// Need to store state in the matcher automatically and restore it if anything went wrong.
// Also, we can introduce Matcher.Any(bool: condition...) and pass another matcher into it
// The execution flow would be like that
// - Create Matcher1, it saves state
// - Consume Identifier, LexerView changed
// - call Matcher1.Any(Matcher2().Match(NestedExpression))
//   Matcher2 saves the state after we already consumed identifier and if anything goes wrong - restores it to that moment and Matcher1 may also set its state to false
//
// To resolve ambiguity we may add Matcher.Optional(bool). It's like any, but doesn't reset the state.

bool ResolveIdentifierBased(LexerView& view) {
  return Matcher(view).Any(ResolveIdentifier(view)).Optional(Matcher(view).Any(ResolveNestedExpression));
}

bool ResolveNestedExpression(LexerView& view) {
  return Matcher(view).Any(ResolveExpression, ResolveIdentifierBased);
}

bool ResolveExpression(LexerView& view) {
  return Matcher(view).Match('(').Apply(PushBraceOpen).Any(ResolveNestedExpression).Match(')').Apply(PushBraceClose);
}
*/

/*
Task 2: Floating point numbers

FpNumber = PartNumberBased
FpNumber = '.' Number Exponent?

PartNumberBased = Number Exponent?
PartNumberBased = Number '.' Number? Exponent?

Exponent = ExponentSymbol Sign? Number
Number = Digit+

ExponentSymbol = 'e' | 'E'
Sign = '+' | '-'
Digit = '0'..'9'

Problem: Despite having a lots of rules, all of that should go to stof(), so we should collectively accumulate input somehow
Solution: If stof() already works like a parser, we should just match a number and Collect() or Extract() that token
So, Any() can specifically accept Matcher& and update cursor with the one that returned true
Or it can fetch LexerView& again. Not decided yet
Or it can do nothing and then we call Update(view). But shouldn't that reset the state entirely? I will think about that.

MatchNumber:
return Matcher(view).While(isnumber);

MatchExponent:
return Matcher(view).Any({'e', 'E'}).Optional(Matcher(view).Any({'+', '-'})).Any(MatchNumber);

MatchPartNumberBased:
return Matcher(view)
.Any(MatchNumber)
.Any(
  Matcher(view)
  .Any('.')
  .Optional(MatchNumber)
  .Optional(MatchExponent),
  Matcher(view)
  .Optional(MatchExponent)
)

MatchFpNumber:
return Matcher(view).Any(
  MatchPartNumberBased,
  Matcher(view)
  .Any('.')
  .Any(MatchNumber)
  .Optional(MatchExponent);
);

TokenizeFpNumber:
try {
  float result = stof(...);
  PushFpToken(result);
  return true;
  // Hmm.. I need to extract the token, so I can't incapsulate that into bool Match..()
  // Also I will need to extract the start and end position
  // How to do that with just Matcher()?
} catch (...) {
  return false;
}

*/

/*
StatementIdentifierBased = Identifier (PartVarModify | PartFuncDecl | PartVarDecl | PartCall)
PartVarModify = Skip+ ("add" | "sub" | "mult") Skip* Value
PartFuncDecl = Skip+ "function" Skip+ StatementChain NL
PartVarDecl = Skip* '=' Skip* Value
PartCall = Skip* '(' Skip* ')'

using Matcher = ChainMatcher<Lexer>

bool ResolveIdentifierBased(LexerView& view) {
  return Matcher().Consume(IsIdenti)
}
*/

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

template <typename Context, typename State>
concept ContextWithState = requires(Context ctx, const State& state) {
  { ctx->GetState() } -> std::same_as<State>;
  ctx->SetState(state);
  std::copyable<State>;
};

template <typename Context, typename State>
concept NoContext = std::same_as<Context, void*> && std::same_as<State, void*>;

template <typename LexerContext, typename LexerState>
requires NoContext<LexerContext, LexerState> || ContextWithState<LexerContext, LexerState>
struct ChainMatcher;

template <typename LexerContext, typename LexerState, typename Predicate>
concept ChainMatcherPredicate = requires(ChainMatcher<LexerContext, LexerState>& matcher, Predicate pred) {
  { pred(matcher) } -> std::convertible_to<bool>;
};

template <typename LexerContext, typename LexerState, typename Predicate>
concept ChainMatcherConsumer = requires(ChainMatcher<LexerContext, LexerState>& matcher, Predicate pred) {
  pred(matcher);
};

// Automatically saves state and restores it on fail
template <typename LexerContext = void*, typename LexerState = void*>
requires NoContext<LexerContext, LexerState> || ContextWithState<LexerContext, LexerState>
struct ChainMatcher final {
  explicit ChainMatcher(LexerView& view, LexerContext ctx) requires ContextWithState<LexerContext, LexerState>
  : m_view(view)
  , m_ctx(ctx)
  , m_viewState(view.GetState())
  , m_state(ctx->GetState())
  , m_isOk(true) {
  }

  explicit ChainMatcher(LexerView& view) requires NoContext<LexerContext, LexerState>
  : m_view(view)
  , m_ctx(nullptr)
  , m_state(nullptr)
  , m_viewState(view.GetState())
  , m_isOk(true) {
  }

  LexerView& GetView() {
    return m_view;
  }

  LexerContext GetContext() {
    return m_ctx;
  }

  operator bool() const {
    return m_isOk;
  }

  // Record: saves states of the context and the view
  ChainMatcher& Record() requires ContextWithState<LexerContext, LexerState> {
    if (!m_isOk) {
      return *this;
    }
    // TODO should update TokenRecorder
    m_viewState = m_view.GetState();
    m_state = m_ctx->GetState();
    return *this;
  }

  ChainMatcher& Record() requires NoContext<LexerContext, LexerState> {
    if (!m_isOk) {
      return *this;
    }

    m_viewState = m_view.GetState();
    return *this;
  }

  ChainMatcher& Assert(bool status) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = status;
    RestoreIfFail();
    return *this;
  }

  // Perform (void (*)(Matcher&)...)
  template <typename Consumer>
  requires ChainMatcherConsumer<LexerContext, LexerState, Consumer>
  ChainMatcher& Perform(Consumer consumer) {
    if (!m_isOk) {
      return *this;
    }

    consumer(*this);
    return *this;
  }

  // Any (char...)
  template <typename... Chars>
  ChainMatcher& Any(Chars... chars) requires(std::same_as<Chars, char> && ...) {
    if (!m_isOk) {
      return *this;
    }

    char next = m_view.Next();
    m_isOk = m_view.HasSymbols() && ((next == chars) || ...);
    if (m_isOk) {
      m_view.Advance();
    }

    RestoreIfFail();
    return *this;
  }

  // Any (string_view...)
  template <typename... Strings>
  ChainMatcher& Any(Strings... strings) requires(std::same_as<Strings, std::string_view> && ...) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = (m_view.MatchAdvance(strings) || ...);
    RestoreIfFail();
    return *this;
  }

  // Any (bool (*)(char)...)
  template <CharPredicate... Predicate>
  ChainMatcher& Any(Predicate... preds) {
    if (!m_isOk) {
      return *this;
    }

    char next = m_view.Next();
    m_isOk = m_view.HasSymbols() && (preds(next) || ...);
    if (m_isOk) {
      m_view.Advance();
    }

    RestoreIfFail();
    return *this;
  }

  // Any (bool (*)(Matcher&)...)
  template <typename... Predicate>
  requires ChainMatcherPredicate<LexerContext, LexerState, Predicate>
  ChainMatcher& Any(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = (predicate(*this) || ...);
    if (m_isOk) {
      m_view.Advance();
    }

    RestoreIfFail();
    return *this;
  }

  // Many
  // ZeroMany
  // Optional

private:
  // Restore: restores states. Does NOT remove FAIL.
  void Restore() requires ContextWithState<LexerContext, LexerState> {
    m_view.SetState(m_viewState);
    m_ctx->SetState(m_state);
  }

  void Restore() requires NoContext<LexerContext, LexerState> {
    m_view.SetState(m_viewState);
  }

  void RestoreIfFail() {
    if (!m_isOk) {
      Restore();
    }
  }

private:
  LexerView& m_view;
  LexerContext m_ctx;
  LexerView::State m_viewState;
  LexerState m_state;
  bool m_isOk;

  // consumed stringview
};

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
    return ResolveStatement(m_view);
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
  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  void PopToken() {
    m_tokens.pop_back();
  }

  static constexpr auto FirstSign = [](char c) { return c == '+' || c == '-'; };
  static constexpr auto FirstWhitespace = [](char c) { return isspace(c) && c != '\n'; };

  static constexpr auto FirstNumber = [](char c) { return isdigit(c) || FirstSign(c); };
  static constexpr auto FirstIdentifier = [](char c) { return isalpha(c); };

  bool MatchSkip(LexerView& view, bool inExpression) const {
    // Same for IN and OUT
    if (m_nestingLevel == 0) {
      return view.Match(isspace) || view.Match("//");
    }

    if (inExpression) {
      return view.Match(FirstWhitespace);
    }

    // This function is designed specifically for skipping spaces.
    // If you want to match \n after the last word in a statement, then you stould do it separately.
    return view.Match(FirstWhitespace) || view.Match("//");
  }

  // If a tokenizer function fails, the position shouldn't change
  // Only top-level functions should handle whitespaces and new lines

  bool TokenizeNumber(LexerView& view) {
    if (!view.Match(FirstNumber)) {
      return false;
    }

    auto start = m_view.GetState();
    TokenRecorder recorder(view);

    if (view.Match(FirstSign)) {
      view.Advance();

      if (view.Match(isdigit)) {
        m_view.SetState(start);
        return false;
      }
    }

    while (view.MatchAdvance(isdigit)) {
    }

    try {
      int64_t result = std::stoll(recorder.GetToken());
      PushToken(std::make_unique<TokenNumber>(result));
      return true;
    } catch(std::exception& e) {
      m_view.SetState(start);
      return false;
    }
  }

  bool TokenizeIdentifier(LexerView& view) {
    if (!view.Match(FirstIdentifier)) {
      return false;
    }

    auto start = m_view.GetState();
    TokenRecorder recorder(view);

    while (view.MatchAdvance(FirstIdentifier)) {
    }

    std::string token = recorder.GetToken();
    if (Grammar::IsKeyword(token)) {
      view.SetState(start);
      return false;
    }

    PushToken(std::make_unique<TokenIdentifier>(std::move(token)));
    return true;
  }

  // Resolvers

  // Advance word and match space
  // InExpression = is the following space located inside of an expression.
  // It makes difference for block-based expressions.
  bool ResolveWord(LexerView& view, std::string_view word, bool inExpression) {
    auto viewState = view.GetState();
    if (!view.MatchAdvance(word)) {
      return false;
    }

    if (!view.Match('\n') && !MatchSkip(view, inExpression)) {
      view.SetState(viewState);
      return false;
    }

    return true;
  }

  bool ResolveComment(LexerView& view) {
    if (!view.MatchAdvance("//")) {
      return false;
    }

    while (view.HasSymbols() && view.Next() != '\n') {
      view.Advance();
    }

    return true;
  }

  bool ResolveSkip(LexerView& view, bool inExpression) {
    // S, NL, Comment: same for IN and OUT
    if (m_nestingLevel == 0) {
      bool matched = false;
      while (view.MatchAdvance(isspace) || ResolveComment(view)) {
        matched = true;
      }

      return matched;
    }

    if (inExpression) {
      // S
      if (view.MatchAdvance(FirstWhitespace)) {
        return false;
      }

      while (view.MatchAdvance(FirstWhitespace)) {
      }

      return true;
    }

    // S, Comment
    bool matched = false;
    while (view.MatchAdvance(FirstWhitespace) || ResolveComment(view)) {
      matched = true;
    }

    return matched;
  }

  bool ResolveValue(LexerView& view) {
    if (TokenizeNumber(view)) {
      return true;
    }

    auto state = view.GetState();

    if (view.MatchAdvance('&')) {
      PushToken(std::make_unique<TokenDereference>());
      if (!TokenizeIdentifier(view)) {
        view.SetState(state);
        PopToken();
        return false;
      }

      return true;
    }

    return false;
  }

  bool TokenizePrint(LexerView& view) {
    if (!ResolveWord(view, "print", false)) {
      return false;
    }

    PushToken(std::make_unique<TokenPrint>());
    return true;
  }

  bool ResolveVariableDeletion(LexerView& view) {
    auto state = view.GetState();

    if (!ResolveWord(view, "delete", true)) {
      return false;
    }

    PushToken(std::make_unique<TokenDelete>());
    if (!TokenizeIdentifier(view)) {
      PopToken();
      view.SetState(state);
      return false;
    }

    return true;
  }

  bool ResolveComparison(LexerView& view) {
    if (view.MatchAdvance("==")) {
      PushToken(std::make_unique<TokenEqual>());
      return true;
    }

    if (view.MatchAdvance("!=")) {
      PushToken(std::make_unique<TokenNotEqual>());
      return true;
    }

    return false;
  }

  bool ResolvePrimaryExpression(LexerView& view) {
    // How to reset? Number = 1 token, Value = 2 tokens. LexerState?
    auto state = GetState();
    auto viewState = view.GetState();

    if (!ResolveValue(view)) {
      return false;
    }

    if (!ResolveComparison(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    if (!ResolveValue(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    return true;
  }

  bool ResolveAndExpression(LexerView& view) {
    auto state = GetState();
    auto viewState = view.GetState();

    if (!ResolvePrimaryExpression(view)) {
      return false;
    }

    while (ResolveWord(view, "and", true)) {
      PushToken(std::make_unique<TokenAnd>());

      if (!ResolvePrimaryExpression(view)) {
        SetState(state);
        view.SetState(viewState);
        return false;
      }
    }

    return true;
  }

  bool ResolveOrExpression(LexerView& view) {
    auto state = GetState();
    auto viewState = view.GetState();

    if (!ResolveAndExpression(view)) {
      return false;
    }

    while (ResolveWord(view, "or", true)) {
      PushToken(std::make_unique<TokenOr>());

      if (!ResolveAndExpression(view)) {
        SetState(state);
        view.SetState(viewState);
        return false;
      }
    }

    return true;
  }

  bool ResolveExpression(LexerView& view) {
    return ResolveOrExpression(view);
  }

  // identifier function expression \n, identifier = value, identifier (add/sub/mult) value, identifier()
  bool ResolveIdentifierBased(LexerView& view) {
    auto state = GetState();
    auto viewState = view.GetState();

    if (!TokenizeIdentifier(view)) {
      return false;
    }

    // Function call
    if (view.MatchAdvance("()")) {
      PushToken(std::make_unique<TokenCall>());
      return true;
    }

    // Assignment / Add / Mul / Sub
    {
      bool matched = true;
      if (view.MatchAdvance('=')) {
        PushToken(std::make_unique<TokenAssign>());
      } else if (view.MatchAdvance("add")) {
        PushToken(std::make_unique<TokenAdd>());
      } else if (view.MatchAdvance("sub")) {
        PushToken(std::make_unique<TokenSub>());
      } else if (view.MatchAdvance("mult")) {
        PushToken(std::make_unique<TokenMult>());
      } else {
        matched = false;
      }

      if (matched) {
        if (!ResolveValue(view)) {
          SetState(state);
          view.SetState(viewState);
          return false;
        }

        return true;
      }
    }

    // Function declaration
    if (ResolveWord(view, "function", true)) {
      PushToken(std::make_unique<TokenFunction>());
      ++m_nestingLevel;

      if (!ResolveStatement(view)) {
        SetState(state);
        view.SetState(viewState);
        return false;
      }

      if (!view.Match('\n')) {
        SetState(state);
        view.SetState(viewState);
        return false;
      }

      PushToken(std::make_unique<TokenBlockEnd>());
      --m_nestingLevel;

      if (m_nestingLevel == 0) {
        view.Advance();
      }
    }

    SetState(state);
    view.SetState(viewState);
    return false;
  }

  bool ResolveCondition(LexerView& view) {
    if (!view.MatchAdvance("if")) {
      return false;
    }

    PushToken(std::make_unique<TokenIf>());

    auto state = GetState();
    auto viewState = view.GetState();

    if (!ResolveExpression(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    if (!ResolveWord(view, "then", true)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    PushToken(std::make_unique<TokenThen>());

    ++m_nestingLevel;

    if (!ResolveStatement(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    if (!view.Match('\n')) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    PushToken(std::make_unique<TokenBlockEnd>());
    --m_nestingLevel;

    if (m_nestingLevel == 0) {
      view.Advance();
    }

    return true;
  }

  bool ResolveLoop(LexerView& view) {
    if (!view.MatchAdvance("loop")) {
      return false;
    }

    PushToken(std::make_unique<TokenLoop>());

    auto state = GetState();
    auto viewState = view.GetState();

    if (!ResolveValue(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    if (!ResolveWord(view, "do", true)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    PushToken(std::make_unique<TokenDo>());

    ++m_nestingLevel;

    if (!ResolveStatement(view)) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    if (!view.Match('\n')) {
      SetState(state);
      view.SetState(viewState);
      return false;
    }

    PushToken(std::make_unique<TokenBlockEnd>());
    --m_nestingLevel;

    if (m_nestingLevel == 0) {
      view.Advance();
    }

    return true;
  }

  // StatementTerm :=
  // StatementModify
  // | Print+
  // | VariableDeclaration+
  // | VariableDeletion+
  // | VariableModification+
  // | VariableAssignment+
  // | FunctionCall+
  // | FunctionDeclaration+
  // | Condition+
  // | Loop+
  bool ResolveStatementTerm(LexerView& view) {
    return TokenizePrint(view)
      | ResolveVariableDeletion(view)
      | ResolveCondition(view)
      | ResolveLoop(view)
      | ResolveIdentifierBased(view);
  }

  bool ResolveStatement(LexerView& view) {
    if (!ResolveStatementTerm(view)) {
      return false;
    }

    while (ResolveStatementTerm(view)) {
    }

    return true;
  }

private:
  LexerView m_view;
  std::vector<std::unique_ptr<Token>> m_tokens;
  size_t m_nestingLevel;
};
