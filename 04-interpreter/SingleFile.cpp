#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <list>
#include <cassert>
#include <cctype>
#include <unordered_set>
#include <functional>
#include <stdexcept>

/*

Formal grammar

StatementChain = Skip* Statement StatementChain*
Statement = StatementPrint | StatementVarDelete | StatementIdentifierBased | StatementCondition | StatementLoop

StatementLoop = "loop" Skip* Vaue Skip* "do" Skip+ StatementChain NL
StatementCondition = "if" Skip* Expression Skip* "then" Skip+ StatementChain NL

Expression = ExpressionOr
ExpressionOr = ExpressionAnd (Skip* "or" Skip* ExpressionAnd)*
ExpressionAnd = ExpressionPrimary (Skip* "and" Skip* ExpressionPrimary)*
ExpressionPrimary = Value Skip* Comparison Skip* Value

Comparison = NotEquals | Equals
NotEquals = "!="
Equals = "=="

StatementIdentifierBased = Identifier (PartVarModify | PartFuncDecl | PartVarDecl | PartCall)
PartVarModify = Skip+ ("add" | "sub" | "mult") Skip* Value
PartFuncDecl = Skip+ "function" Skip+ StatementChain NL
PartVarDecl = Skip* '=' Skip* Value
PartCall = Skip* '(' Skip* ')'

StatementPrint = "print"
StatementVarDelete = "delete" Skip+ Identifier

Value = OpDereference Skip* Identifier
Value = Number

Identifier = IdentifierSymbol+
Number = (Sign Skip*)? Digit+

Skip = WS | NL | Comment
*if (nestingLevel > 0) Skip = WS  | Comment

Comment = "//" (~'\n')*
NL = '\n'
WS = ' ' | '\t'

OpDereference = $
Sign = + | -
Digit = 0..9
IdentifierSymbol = a..z | A..Z

Not influenced by spaces, BUT:
- Don't match NL inside a block

*/

enum struct TokenType : uint32_t {
  NUMBER,
  IDENTIFIER,

  OP_DEREFERENCE,
  OP_OR,
  OP_AND,
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_ASSIGN,
  OP_CALL,

  KEYWORD_PRINT,
  KEYWORD_ADD,
  KEYWORD_SUB,
  KEYWORD_MULT,
  KEYWORD_DELETE,
  KEYWORD_IF,
  KEYWORD_THEN,
  KEYWORD_FUNCTION,
  KEYWORD_LOOP,
  KEYWORD_DO,

  BLOCK_END,

  COUNT
};

struct Grammar final {
  static bool IsKeyword(std::string_view view) {
    return kKeywords.contains(view);
  }

  static bool IsKeyword(const std::string& str) {
    return kKeywords.contains(std::string_view(str));
  }

  inline static const std::unordered_set<std::string_view> kKeywords = {
    "or",
    "and",
    "delete",
    "print",
    "if",
    "then",
    "loop",
    "do",
    "function",
    "add",
    "sub",
    "mult"
  };
};

struct Token;

template <typename T>
concept ConceptTokenClass = requires(T type) {
  std::derived_from<T, Token>;
  { T::GetType() } -> std::convertible_to<TokenType>;
};

struct Token {
  virtual ~Token() = default;

  [[nodiscard]] TokenType GetType() const {
    return m_type;
  }

  template <ConceptTokenClass T>
  T* As() {
    assert(T::GetType() == GetType());
    return dynamic_cast<T*>(this);
  }

  template <ConceptTokenClass T>
  const T* As() const {
    assert(T::GetType() == GetType());
    return dynamic_cast<const T*>(this);
  }

protected:
  explicit Token(TokenType type)
  : m_type(type) {
  }

private:
  TokenType m_type;
};

template <TokenType T>
struct SimpleToken final : Token {
  explicit SimpleToken()
  : Token(GetType()) {
  }

  static TokenType GetType() {
    return T;
  }
};

using TokenDereference = SimpleToken<TokenType::OP_DEREFERENCE>;
using TokenOr = SimpleToken<TokenType::OP_OR>;
using TokenAnd = SimpleToken<TokenType::OP_AND>;
using TokenEqual = SimpleToken<TokenType::OP_EQUAL>;
using TokenNotEqual = SimpleToken<TokenType::OP_NOT_EQUAL>;
using TokenAssign = SimpleToken<TokenType::OP_ASSIGN>;
using TokenCall = SimpleToken<TokenType::OP_CALL>;

using TokenPrint = SimpleToken<TokenType::KEYWORD_PRINT>;
using TokenAdd = SimpleToken<TokenType::KEYWORD_ADD>;
using TokenSub = SimpleToken<TokenType::KEYWORD_SUB>;
using TokenMult = SimpleToken<TokenType::KEYWORD_MULT>;
using TokenDelete = SimpleToken<TokenType::KEYWORD_DELETE>;
using TokenIf = SimpleToken<TokenType::KEYWORD_IF>;
using TokenThen = SimpleToken<TokenType::KEYWORD_THEN>;
using TokenFunction = SimpleToken<TokenType::KEYWORD_FUNCTION>;
using TokenLoop = SimpleToken<TokenType::KEYWORD_LOOP>;
using TokenDo = SimpleToken<TokenType::KEYWORD_DO>;

using TokenBlockEnd = SimpleToken<TokenType::BLOCK_END>;

struct TokenIdentifier final : Token {
  explicit TokenIdentifier(std::string name)
  : Token(GetType())
  , m_name(std::move(name)) {
  }

  static TokenType GetType() {
    return TokenType::IDENTIFIER;
  }

  [[nodiscard]] const std::string& GetName() const {
    return m_name;
  }

private:
  std::string m_name;
};

struct TokenNumber final : Token {
  explicit TokenNumber(int64_t value)
  : Token(GetType())
  , m_value(value) {
  }

  static TokenType GetType() {
    return TokenType::NUMBER;
  }

  [[nodiscard]] int64_t GetValue() const {
    return m_value;
  }

private:
  int64_t m_value;
};

inline bool EqualsIgnoreCase(char c1, char c2) {
  return tolower(c1) == tolower(c2);
}

inline bool EqualsIgnoreCase(std::string_view s1, std::string_view s2) {
  if (s1.size() != s2.size()) {
    return false;
  }

  auto c1 = s1.begin(), c2 = s2.begin();
  while (c1 != s1.end() && tolower(*c1++) == tolower(*c2++))
    ;

  return c1 == s1.end();
}

template <typename Type>
concept StringViewIsh = std::convertible_to<Type, std::string_view>;

template <typename Func>
concept CharPredicate = requires(Func f, char c) {
  { f(c) } -> std::convertible_to<bool>;
};

struct LexerView final {
  struct State final {
    size_t position;
    size_t line;
    size_t column;
  };

  explicit LexerView(std::string_view string)
  : m_string(string)
  , m_position(0)
  , m_line(1)
  , m_column(1) {
  }

  LexerView(const LexerView& other) = default;

  [[nodiscard]] LexerView AtOffset(size_t offset) const {
    assert(HasSymbols(offset));
    LexerView view(*this);
    view.Advance(offset);
    return view;
  }

  void Reset() {
    m_position = 0;
    m_line = 1;
    m_column = 1;
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_line;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_column;
  }

  [[nodiscard]] State GetState() const {
    return State { m_position, m_line, m_column };
  }

  void SetState(const State& state) {
    m_position = state.position;
    m_line = state.line;
    m_column = state.column;
  }

  [[nodiscard]] size_t RemainingSize() const {
    return m_string.size() - m_position;
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_string.size();
  }

  [[nodiscard]] bool HasSymbols() const {
    return m_position < m_string.size();
  }

  [[nodiscard]] bool HasSymbols(size_t number) const {
    return m_position + number <= m_string.size();
  }

  [[nodiscard]] char Next() const {
    assert(HasSymbols());
    return m_string.at(m_position);
  }

  [[nodiscard]] char Next(size_t offset) const {
    assert(HasSymbols(offset + 1));
    return m_string.at(m_position + offset);
  }

  [[nodiscard]] char NextAsLowerCase() const {
    assert(HasSymbols());
    return static_cast<char>(std::tolower(m_string.at(m_position)));
  }

  [[nodiscard]] char NextAsLowerCase(size_t offset) const {
    assert(HasSymbols(offset + 1));
    return static_cast<char>(std::tolower(m_string.at(m_position + offset)));
  }

  char Advance() {
    assert(HasSymbols());
    if (Next() == '\n') {
      ++m_line;
      m_column = 1;
    } else {
      ++m_column;
    }

    return m_string.at(m_position++);
  }

  char AdvanceAsLowerCase() {
    assert(HasSymbols());
    if (Next() == '\n') {
      ++m_line;
      m_column = 1;
    } else {
      ++m_column;
    }

    return static_cast<char>(std::tolower(m_string.at(m_position++)));
  }

  void Advance(size_t offset) {
    assert(HasSymbols(offset));
    while (offset-- > 0) {
      Advance();
    }
  }

  [[nodiscard]] std::string_view GetTokenView(size_t start, size_t length) const {
    assert(start + length <= m_string.size());
    return m_string.substr(start, length);
  }

  [[nodiscard]] std::string_view GetTokenView(size_t length) const {
    assert(HasSymbols(length));
    return m_string.substr(m_position, length);
  }

  [[nodiscard]] std::string GetToken(size_t start, size_t length) const {
    return std::string(GetTokenView(start, length));
  }

  [[nodiscard]] std::string GetToken(size_t length) const {
    return std::string(GetTokenView(length));
  }

  // Match

  template <typename... Chars>
  [[nodiscard]] bool Match(Chars... chars) const requires(... && std::same_as<Chars, char>) {
    if (!HasSymbols()) {
      return false;
    }

    char c = Next();
    return ((chars == c) || ...);
  }

  template <StringViewIsh... StringViews>
  [[nodiscard]] bool Match(const StringViews&... views) const {
    return ((m_string.find(std::string_view { views }, m_position) == m_position) || ...);
  }

  template <CharPredicate... Predicates>
  [[nodiscard]] bool Match(Predicates... predicates) const {
    if (!HasSymbols()) {
      return false;
    }

    char c = Next();
    return (predicates(c) || ...);
  }

  template <typename... Chars>
  [[nodiscard]] bool MatchIgnoreCase(Chars... chars) const requires(... && std::same_as<Chars, char>) {
    if (!HasSymbols()) {
      return false;
    }

    char c = NextAsLowerCase();
    return ((c == tolower(chars)) || ...);
  }

  template <StringViewIsh... StringViews>
  [[nodiscard]] bool MatchIgnoreCase(const StringViews&... views) const {
    return (([this](const std::string_view view) {
      return EqualsIgnoreCase(m_string.substr(m_position, view.size()), view);
    }(views)) || ...);
  }


  // MatchAdvance


  template <typename... Chars>
  bool MatchAdvance(Chars... chars) requires(... && std::same_as<Chars, char>) {
    if (Match(chars...)) {
      Advance();
      return true;
    }

    return false;
  }

  template <StringViewIsh... StringViews>
  bool MatchAdvance(const StringViews&... views) {
    return (([this](std::string_view view) {
      if (Match(view)) {
        Advance(view.size());
        return true;
      }

      return false;
    }(views)) || ...);
  }

  template <CharPredicate... Predicate>
  bool MatchAdvance(Predicate... predicates) {
    if (!HasSymbols()) {
      return false;
    }

    if (Match(predicates...)) {
      Advance();
      return true;
    }

    return false;
  }

  template <typename... Chars>
  bool MatchAdvanceIgnoreCase(Chars... chars) requires(... && std::same_as<Chars, char>) {
    if (MatchIgnoreCase(chars...)) {
      Advance();
      return true;
    }

    return false;
  }

  template <StringViewIsh... StrViews>
  bool MatchAdvanceIgnoreCase(const StrViews&... views) {
    return (([this](const std::string_view view) {
      if (MatchIgnoreCase(view)) {
        Advance(view.size());
        return true;
      }

      return false;
    }(views)) || ...);
  }

  // Extract. Non-advancing

  template <CharPredicate... Predicates>
  [[nodiscard]] std::string_view MatchExtractView(Predicates... predicates) const {
    size_t offset = 0;
    while (HasSymbols(offset + 1) && (predicates(Next(offset)) || ...)) {
      ++offset;
    }

    if (offset == 0) {
      return std::string_view {};
    }

    return m_string.substr(m_position, offset);
  }

  template <CharPredicate... Predicates>
  [[nodiscard]] std::string MatchExtract(Predicates... predicates) const {
    return std::string(MatchExtractView(predicates...));
  }

private:
  std::string_view m_string;
  size_t m_position;
  size_t m_line;
  size_t m_column;
};

struct TokenRecorder final {
  explicit TokenRecorder(const LexerView& view)
  : m_view(view)
  , m_savedState(view.GetState()) {
  }

  void Reset() {
    m_savedState = m_view.GetState();
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_savedState.position;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_savedState.line;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_savedState.column;
  }

  [[nodiscard]] const LexerView::State& GetSavedState() const {
    return m_savedState;
  }

  [[nodiscard]] std::string_view GetTokenView() const {
    return m_view.GetTokenView(m_savedState.position, m_view.GetPosition() - m_savedState.position);
  }

  [[nodiscard]] std::string GetToken() const {
    return m_view.GetToken(m_savedState.position, m_view.GetPosition() - m_savedState.position);
  }

private:
  const LexerView& m_view;
  LexerView::State m_savedState;
};

template <typename Func>
concept LexerViewPredicate = requires(Func f, LexerView& view) {
  { f(view) } -> std::convertible_to<bool>;
};

template <typename Func, typename... Args>
concept LexerViewPredicateV = requires(Func f, Args... args, LexerView& view) {
  { std::invoke(f, args..., view) } -> std::convertible_to<bool>;
};

template <typename Func>
concept LexerViewConsumer = requires(Func f, LexerView& view) {
  f(view);
};

// template <typename Func>
// concept StringViewPredicate = requires(Func f, std::string_view view) {
//   { f(view) } -> std::convertible_to<bool>;
// };

template <typename Func>
concept TokenRecorderPredicate = requires(Func f, const TokenRecorder& recorder) {
  { f(recorder) } -> std::convertible_to<bool>;
};

struct Matcher final {
  explicit Matcher(LexerView& view)
  : m_view(view)
  , m_savedState(view.GetState())
  , m_recorder(view)
  , m_isOk(true) {
  }

  operator bool() const {
    return m_isOk;
  }

  // Matcher& Store() {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_savedState = m_view.GetState();
  //   return *this;
  // }
  //
  // Matcher& Restore() {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_view.SetState(m_savedState);
  //   return *this;
  // }

  Matcher& Record() {
    if (!m_isOk) {
      return *this;
    }

    m_recorder.Reset();
    return *this;
  }

  std::string GetToken() {
    return m_recorder.GetToken();
  }

  std::string_view GetTokenView() {
    return m_recorder.GetTokenView();
  }

  // ??? WTF are you doing here? You want to repeat what we did yesterday?
  // template <StringViewPredicate Predicate>
  // Matcher& CollectTokenView(Predicate predicate) {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_isOk = predicate(m_recorder.GetTokenView());
  //   RestoreIfFailed();
  //   return *this;
  // }

  // Shitty function. What if I want line? column? tokenView? token?
  template <TokenRecorderPredicate Predicate>
  Matcher& CollectTokenRecorder(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = predicate(m_recorder);
    RestoreIfFailed();
    return *this;
  }

  // .CollectTokenView()???

  // Actions: Reset (OK) - no application, RestoreIfOK - no application (LexerView::State), RestoreIfFailed - utility function, called automatically (LexerView::State)
  // Philosophy: IF state is failed, view is momentarely reset and you cannot perform further actions
  // Reset???
  // RestoreIfOK???
  // ResetAndRestore???

  // .Store()???
  // .Restore()???


  // Non-advancing, state-changing
  // .Match(char...)
  // .Match(string_view...)
  // .Match(CharPredicate...)

  template <typename... Chars>
  Matcher& Match(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.Match(chars...);
    RestoreIfFailed();
    return *this;
}

  template <StringViewIsh... StringViews>
  Matcher& Match(StringViews... views) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.Match(views...);
    RestoreIfFailed();
    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Match(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.Match(predicate...);
    RestoreIfFailed();
    return *this;
  }



  // Advancing, non-state-changing
  // .Optional(char...)
  // .Optional(string_view...)
  // .Optional(CharPredicate...)

  template <typename... Chars>
  Matcher& Optional(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(chars...);
    return *this;
  }

  template <StringViewIsh... StringViews>
  Matcher& Optional(StringViews... views) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(views...);
    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Optional(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(predicate...);
    return *this;
  }

  // Advancing, state-changing

  // +.Any(char...)
  // +.Any(string_view...)
  // +.Any(CharPredicate...)

  template <typename... Chars>
  Matcher& Any(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(chars...);
    RestoreIfFailed();
    return *this;
  }

  template <StringViewIsh... StringViews>
  Matcher& Any(StringViews... views) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(views...);
    RestoreIfFailed();
    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Any(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(predicate...);
    RestoreIfFailed();
    return *this;
  }

  // Advancing, non-state-changing
  // +.ZeroMany(char...)
  // +.ZeroMany(string_view...)
  // +.ZeroMany(CharPredicate...)
  template <typename... Chars>
  Matcher& ZeroMany(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(chars...)) {
    }

    return *this;
  }

  template <StringViewIsh... Views>
  Matcher& ZeroMany(Views... views) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(views...)) {
    }

    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& ZeroMany(Predicate... predicates) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(predicates...)) {
    }

    return *this;
  }

  // +.Many(char...)
  // +.Many(string_view...)
  // +.Many(CharPredicate...)

  template <typename... Chars>
  Matcher& Many(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!Any(chars...)) {
      return *this;
    }

    while (m_view.MatchAdvance(chars...)) {
    }

    return *this;
  }

  template <StringViewIsh... Views>
  Matcher& Many(Views... views) {
    if (!Any(views...)) {
      return *this;
    }

    while (m_view.MatchAdvance(views...)) {
    }

    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Many(Predicate... predicates) {
    if (!m_isOk) {
      return *this;
    }

    if (!Any(predicates...)) {
      m_isOk = false;
      RestoreIfFailed();
      return *this;
    }

    while (m_view.MatchAdvance(predicates...)) {
    }

    return *this;
  }

  // State-changing, non-advancing
  // .Assert(bool)

  Matcher& Assert(bool value) {
    m_isOk &= value;
    RestoreIfFailed();
    return *this;
  }

  // State-changing, non-advancing
  // .Assert(ViewPredicate)???
  // .AssertMany(ViewPredicate)???
  // .AssertZeroMany(ViewPredicate)???

  template <LexerViewPredicate Predicate>
  Matcher& Assert(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = predicate(m_view);
    RestoreIfFailed();
    return *this;
  }

  template <typename Predicate, typename... Args>
  Matcher& Assert(Predicate predicate, Args... args) requires(LexerViewPredicateV<Predicate, Args...>) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = std::invoke(predicate, args..., m_view);
    RestoreIfFailed();
    return *this;
  }

  template <LexerViewPredicate Predicate>
  Matcher& AssertZeroMany(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    while (predicate(m_view)) {
    }

    return *this;
  }

  template <typename Predicate, typename... Args>
  Matcher& AssertZeroMany(Predicate predicate, Args... args) requires(LexerViewPredicateV<Predicate, Args...>) {
    if (!m_isOk) {
      return *this;
    }

    while (std::invoke(predicate, args..., m_view)) {
    }

    return *this;
  }

  template <LexerViewPredicate Predicate>
  Matcher& AssertMany(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    if (!predicate(m_view)) {
      m_isOk = false;
      RestoreIfFailed();
    }

    while (predicate(m_view)) {
    }

    return *this;
  }

  template <typename Predicate, typename... Args>
  Matcher& AssertMany(Predicate predicate, Args... args) requires(LexerViewPredicateV<Predicate, Args...>) {
    if (!m_isOk) {
      return *this;
    }

    if (!std::invoke(predicate, args..., m_view)) {
      m_isOk = false;
      RestoreIfFailed();
    }

    while (std::invoke(predicate, args..., m_view)) {
    }

    return *this;
  }

  // Why not MatcherPredicate???
  // What can we do with a matcher?
  // We can capture a matcher. I don't want to write matcher->getView every fucking time

  // non-state-changing, non-advancing
  // .Perform(ViewConsumer)???

  template <LexerViewConsumer Consumer>
  Matcher& Perform(Consumer consumer) {
    if (m_isOk) {
      consumer(m_view);
    }

    return *this;
  }

  template <LexerViewConsumer Consumer>
  Matcher& PerformIfFailed(Consumer consumer) {
    if (!m_isOk) {
      consumer(m_view);
    }

    return *this;
  }

private:
  void RestoreIfFailed() {
    if (!m_isOk) {
      m_view.SetState(m_savedState);
    }
  }

private:
  LexerView& m_view;
  LexerView::State m_savedState;
  TokenRecorder m_recorder;
  bool m_isOk;
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
    bool result = ResolveStatementChain(m_view);
    m_view.MatchAdvance(isspace);
    if (!m_view.IsEnd()) {
      return false;
    }

    return result;
  }

  void Reset() {
    m_view.Reset();
    m_tokens.clear();
    m_nestingLevel = 0;
  }

  [[nodiscard]] LexerState GetState() const {
    return LexerState { m_tokens.size(), m_nestingLevel };
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

struct ParserView final {
  struct State final {
    size_t position;
  };

  explicit ParserView(const std::vector<std::unique_ptr<Token>>& input)
  : m_input(input)
  , m_position(0) {
  }

  void Reset() {
    m_position = 0;
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] State GetState() const {
    return State { m_position };
  }

  void SetState(const State& state) {
    m_position = state.position;
  }

  [[nodiscard]] size_t RemainingSize() const {
    return m_input.size() - m_position;
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_input.size();
  }

  [[nodiscard]] bool HasTokens() const {
    return m_position < m_input.size();
  }

  [[nodiscard]] bool HasTokens(size_t amount) const {
    return m_position + amount <= m_input.size();
  }

  [[nodiscard]] const Token* Next() const {
    assert(HasTokens());
    return m_input.at(m_position).get();
  }

  [[nodiscard]] const Token* Next(size_t offset) const {
    assert(HasTokens(offset + 1));
    return m_input.at(m_position + offset).get();
  }

  template <ConceptTokenClass Type>
  [[nodiscard]] const Type* Next() const {
    assert(HasTokens());
    return dynamic_cast<const Type*>(m_input.at(m_position).get());
  }

  template <ConceptTokenClass Type>
  [[nodiscard]] const Type* Next(size_t offset) const {
    assert(HasTokens(offset + 1));
    return dynamic_cast<const Type*>(m_input.at(m_position + offset).get());
  }

  const Token* Advance() {
    assert(HasTokens());
    return m_input.at(m_position++).get();
  }

  template <typename T>
  const T* Advance() {
    assert(HasTokens());
    const T* result = dynamic_cast<const T*>(m_input.at(m_position++).get());
    assert(result);
    return result;
  }

  void Advance(size_t offset) {
    assert(HasTokens(offset));
    m_position += offset;
  }

  template <typename... TokenTypes>
  [[nodiscard]] bool Match(TokenTypes... types) const requires (... && std::same_as<TokenTypes, TokenType>) {
    if (!HasTokens()) {
      return false;
    }

    return ((Next()->GetType() == types) || ...);
  }

  template <typename... TokenTypes>
  [[nodiscard]] bool MatchAdvance(TokenTypes... types) requires (... && std::same_as<TokenTypes, TokenType>) {
    if (Match(types...)) {
      Advance();
      return true;
    }

    return false;
  }

private:
  const std::vector<std::unique_ptr<Token>>& m_input;
  size_t m_position;
};

enum struct AstNodeType : uint32_t {
  VALUE_NUMBER,//
  VALUE_IDENTIFIER,//
  BINARY_OPERATOR,//

  STATEMENT_PRINT,//
  STATEMENT_DELETE,//
  STATEMENT_CALL,//
  STATEMENT_VAR_MODIFICATION,//
  STATEMENT_FUNC_DECL,//
  STATEMENT_CONDITION,//
  STATEMENT_LOOP,//
  STATEMENT_CHAIN,

  COUNT
};

enum struct BinaryOperatorType : uint32_t {
  EQUALS,
  NOT_EQUALS,
  OR,
  AND,
  COUNT
};

enum struct ModificationOperatorType : uint32_t {
  ADD,
  SUBTRACT,
  MULTIPLY,
  ASSIGN,
  COUNT
};

struct AstNode;

struct AstNode {
  virtual ~AstNode() = default;

  [[nodiscard]] AstNodeType GetType() const {
    return m_type;
  }

  template <typename T>
  [[nodiscard]] const T* As() const requires(std::derived_from<T, AstNode>) {
    return dynamic_cast<const T*>(this);
  }

protected:
  explicit AstNode(AstNodeType type)
  : m_type(type) {
  }

private:
  AstNodeType m_type;
};

struct AstNodeValueNumber final : AstNode {
  explicit AstNodeValueNumber(int64_t value)
  : AstNode(GetType())
  , m_value(value) {
  }

  static AstNodeType GetType() {
    return AstNodeType::VALUE_NUMBER;
  }

  [[nodiscard]] int64_t GetValue() const {
    return m_value;
  }

private:
  int64_t m_value;
};

struct AstNodeValueIdentifier final : AstNode {
  explicit AstNodeValueIdentifier(std::string name)
  : AstNode(GetType())
  , m_name(std::move(name)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::VALUE_IDENTIFIER;
  }

  [[nodiscard]] const std::string& GetName() const {
    return m_name;
  }

private:
  std::string m_name;
};

struct AstNodeBinaryOperator final : AstNode {
  explicit AstNodeBinaryOperator(BinaryOperatorType type, std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> right)
  : AstNode(GetType())
  , m_type(type)
  , m_left(std::move(left))
  , m_right(std::move(right)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::BINARY_OPERATOR;
  }

  [[nodiscard]] BinaryOperatorType GetOperatorType() const {
    return m_type;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetLeft() const {
    return m_left;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetRight() const {
    return m_right;
  }

private:
  BinaryOperatorType m_type;
  std::shared_ptr<AstNode> m_left;
  std::shared_ptr<AstNode> m_right;
};

struct AstNodeStatementPrint final : AstNode {
  explicit AstNodeStatementPrint()
  : AstNode(GetType()) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_PRINT;
  }
};

struct AstNodeStatementDelete final : AstNode {
  explicit AstNodeStatementDelete(std::string variableName)
  : AstNode(GetType())
  , m_variableName(std::move(variableName)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_DELETE;
  }

  [[nodiscard]] const std::string& GetVariableName() const {
    return m_variableName;
  }

private:
  std::string m_variableName;
};

struct AstNodeStatementCall final : AstNode {
  explicit AstNodeStatementCall(std::string functionName)
  : AstNode(GetType())
  , m_functionName(std::move(functionName)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CALL;
  }

  [[nodiscard]] const std::string& GetFunctionName() const {
    return m_functionName;
  }

private:
  std::string m_functionName;
};

struct AstNodeBinaryStatementVarModification final : AstNode {
  explicit AstNodeBinaryStatementVarModification(ModificationOperatorType type, std::string varName, std::shared_ptr<AstNode> value)
  : AstNode(GetType())
  , m_type(type)
  , m_varName(std::move(varName))
  , m_value(std::move(value)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_VAR_MODIFICATION;
  }

  [[nodiscard]] ModificationOperatorType GetOperatorType() const {
    return m_type;
  }

  [[nodiscard]] const std::string& GetVariableName() const {
    return m_varName;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetValue() const {
    return m_value;
  }

private:
  ModificationOperatorType m_type;
  std::string m_varName;
  std::shared_ptr<AstNode> m_value;
};

struct AstNodeStatementFunctionDeclaration final : AstNode {
  explicit AstNodeStatementFunctionDeclaration(std::string functionName, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_functionName(std::move(functionName))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_FUNC_DECL;
  }

  [[nodiscard]] const std::string& GetFunctionName() const {
    return m_functionName;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::string m_functionName;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementCondition final : AstNode {
  explicit AstNodeStatementCondition(std::shared_ptr<AstNode> condition, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_condition(std::move(condition))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CONDITION;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCondition() const {
    return m_condition;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::shared_ptr<AstNode> m_condition;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementLoop final : AstNode {
  explicit AstNodeStatementLoop(std::shared_ptr<AstNode> initValue, std::shared_ptr<AstNode> code)
  : AstNode(GetType())
  , m_initValue(std::move(initValue))
  , m_code(std::move(code)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_LOOP;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetInitValue() const {
    return m_initValue;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetCode() const {
    return m_code;
  }

private:
  std::shared_ptr<AstNode> m_initValue;
  std::shared_ptr<AstNode> m_code;
};

struct AstNodeStatementChain final : AstNode {
  explicit AstNodeStatementChain(std::vector<std::shared_ptr<AstNode>> statements)
  : AstNode(GetType())
  , m_statements(std::move(statements)) {
  }

  static AstNodeType GetType() {
    return AstNodeType::STATEMENT_CHAIN;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<AstNode>>& GetStatements() const {
    return m_statements;
  }

private:
  std::vector<std::shared_ptr<AstNode>> m_statements;
};

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

struct ExecutionException final : std::runtime_error {
  explicit ExecutionException(const char* message)
  : std::runtime_error(message) {
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
    for (auto iter = m_values.cbegin(), end = m_values.cend(); iter != end;) {
      std::cout << iter->value;
      ++iter;
      if (iter == end) {
        std::cout << '\n';
        continue;
      }

      std::cout << ' ';
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
    // if (m_functions.contains(name)) {
    //   throw ExecutionException("Function is already defined.");
    // }

    if (m_functions.contains(name)) {
      m_functions.at(name) = node->GetCode();
    } else {
      m_functions.emplace(name, node->GetCode());
    }

    // m_functions.emplace(name, node->GetCode());
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

std::string GetInput() {
  std::stringstream ss;
  std::string line;
  std::getline(std::cin, line);
  uint64_t lineNumber = 0;
  try {
    lineNumber = std::stoull(line);
  } catch (const std::exception&) {
    lineNumber = 0;
  }

  while (lineNumber > 0 && std::getline(std::cin, line)) {
    ss << line << '\n';
    --lineNumber;
  }

  return ss.str();
}

int main() {
  std::string input = GetInput();
  Lexer lexer(input);
  if (!lexer.Tokenize()) {
    std::cout << "ERROR\n";
    return 0;
  }

  const auto& tokens = lexer.GetTokens();
  Parser parser(tokens);
  auto program = parser.Parse();
  if (!program) {
    std::cout << "ERROR\n";
    return 0;
  }

  Interpreter interpreter;
  try {
    interpreter.Evaluate(program.get());
  } catch (ExecutionException& e) {
    std::cout << "ERROR\n";
    return 0;
  }

  return 0;
}