#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <cassert>

enum struct TokenType : uint32_t {
  NUMBER,
  OPERATOR,
  COUNT
};

enum struct OperatorType : uint32_t {
  ADD,
  SUBTRACT,
  COUNT
};

std::string GetInput() {
  std::stringstream stream;

  std::string line;
  while (std::getline(std::cin, line)) {
    stream << line << ' ';
  }

  return stream.str();
}


// Tokenization


struct Token {
  virtual ~Token() = default;

  [[nodiscard]] TokenType GetType() const {
    return m_type;
  }

  [[nodiscard]] const std::string& GetToken() const {
    return m_token;
  }

protected:
  explicit Token(TokenType type, std::string token)
  : m_type(type)
  , m_token(std::move(token))
  {
  }

private:
  TokenType m_type;
  std::string m_token;
};

struct TokenNumber final : Token {
  explicit TokenNumber(std::string token, int64_t number)
  : Token(TokenType::NUMBER, std::move(token))
  , m_number(number)
  {
  }

  [[nodiscard]] int64_t GetNumber() const {
    return m_number;
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::NUMBER;
  }

private:
  int64_t m_number;
};

struct TokenOperator : Token {
  explicit TokenOperator(std::string token, OperatorType opType)
  : Token(TokenType::OPERATOR, std::move(token))
  , m_opType(opType)
  {
  }

  [[nodiscard]] OperatorType GetOperatorType() const {
    return m_opType;
  }

  [[nodiscard]] static TokenType GetType() {
    return TokenType::OPERATOR;
  }

private:
  OperatorType m_opType;
};

struct TokenizerContext final {
  explicit TokenizerContext(std::string input)
  : m_input(std::move(input))
  , m_position(0) {
  }

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_input.size();
  }

  [[nodiscard]] char Next() const {
    assert(!IsEnd());
    return m_input[m_position];
  }

  void Advance() {
    assert(!IsEnd());
    ++m_position;
  }

  void PushToken(std::unique_ptr<Token>&& token) {
    m_tokens.emplace_back(std::move(token));
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Token>>& GetTokens() const {
    return m_tokens;
  }

  std::vector<std::unique_ptr<Token>> ReleaseTokens() {
    std::vector<std::unique_ptr<Token>> result;
    std::swap(result, m_tokens);
    m_position = 0;
    return result;
  }

private:
  std::string m_input;
  size_t m_position;
  std::vector<std::unique_ptr<Token>> m_tokens;
};

bool TokenizeNumber(TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  std::stringstream ss;
  char c = ctx.Next();
  if (!isdigit(c)) {
    return false;
  }

  ss << c;
  ctx.Advance();

  while (!ctx.IsEnd() && isdigit(c = ctx.Next())) {
    ss << c;
    ctx.Advance();
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

bool TokenizeOperator(TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  char c = ctx.Next();
  ctx.Advance();

  switch (c) {
    case '+': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::ADD));
      break;
    }
    case '-': {
      ctx.PushToken(std::make_unique<TokenOperator>(std::string { c }, OperatorType::SUBTRACT));
      break;
    }
    default: {
      return false;
    }
  }

  return true;
}

bool MatchWhitespace(TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  if (!isspace(ctx.Next())) {
    return false;
  }

  ctx.Advance();

  while (!ctx.IsEnd() && isspace(ctx.Next())) {
    ctx.Advance();
  }

  return true;
}

bool ResolveExpression(TokenizerContext& ctx) {
  if (ctx.IsEnd()) {
    return false;
  }

  MatchWhitespace(ctx);
  if (!TokenizeNumber(ctx)) {
    return false;
  }
  if (ctx.IsEnd()) {
    return true;
  }
  MatchWhitespace(ctx);

  while (!ctx.IsEnd()) {
    if (!TokenizeOperator(ctx)) {
      return false;
    }
    MatchWhitespace(ctx);
    if (!TokenizeNumber(ctx)) {
      return false;
    }
    MatchWhitespace(ctx);
  }

  return true;
}


// Building AST

enum struct AstNodeType {
  NUMBER,
  OPERATOR,
  COUNT
};

struct AstNode {
  virtual ~AstNode() = default;

  [[nodiscard]] AstNodeType GetType() const {
    return m_type;
  }

protected:
  explicit AstNode(AstNodeType type)
  : m_type(type) {
  }

private:
  AstNodeType m_type;
};

struct AstNodeNumber final : AstNode {
  explicit AstNodeNumber(int64_t number)
  : AstNode(GetType())
  , m_number(number) {
  }

  [[nodiscard]] static AstNodeType GetType() {
    return AstNodeType::NUMBER;
  }

  [[nodiscard]] int64_t GetNumber() const {
    return m_number;
  }

private:
  int64_t m_number;
};

struct AstNodeOperator final : AstNode {
  explicit AstNodeOperator(OperatorType opType, std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> right)
  : AstNode(GetType())
  , m_opType(opType)
  , m_left(std::move(left))
  , m_right(std::move(right)) {
  }

  [[nodiscard]] static AstNodeType GetType() {
    return AstNodeType::OPERATOR;
  }

  [[nodiscard]] OperatorType GetOperatorType() const {
    return m_opType;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetLeft() const {
    return m_left;
  }

  [[nodiscard]] std::shared_ptr<AstNode> GetRight() const {
    return m_right;
  }

private:
  OperatorType m_opType;
  std::shared_ptr<AstNode> m_left;
  std::shared_ptr<AstNode> m_right;
};

struct ParserContext final {
  explicit ParserContext(const std::vector<std::unique_ptr<Token>>& tokens, size_t position = 0)
  : m_tokens(tokens)
  , m_position(position) {
  }

  ParserContext(const ParserContext& other) = default;

  [[nodiscard]] bool IsEnd() const {
    return m_position >= m_tokens.size();
  }

  [[nodiscard]] const Token* Next() const {
    assert(!IsEnd());
    return m_tokens.at(m_position).get();
  }

  const Token* Advance() {
    assert(!IsEnd());
    return m_tokens.at(m_position++).get();
  }

private:
  const std::vector<std::unique_ptr<Token>>& m_tokens;
  size_t m_position;
};

std::shared_ptr<AstNode> BuildAst(ParserContext& ctx) {
  if (ctx.IsEnd() || ctx.Next()->GetType() != TokenType::NUMBER) {
    return nullptr;
  }

  auto leftToken = dynamic_cast<const TokenNumber*>(ctx.Advance());
  std::shared_ptr<AstNode> leftNode = std::make_shared<AstNodeNumber>(
    leftToken->GetNumber()
  );

  while (!ctx.IsEnd() && ctx.Next()->GetType() == TokenType::OPERATOR) {
    auto opToken = dynamic_cast<const TokenOperator*>(ctx.Advance());
    if (ctx.IsEnd() || ctx.Next()->GetType() != TokenType::NUMBER) {
      return nullptr;
    }

    auto rightToken = dynamic_cast<const TokenNumber*>(ctx.Advance());
    std::shared_ptr<AstNode> rightNode = std::make_shared<AstNodeNumber>(
      rightToken->GetNumber()
    );

    leftNode = std::make_shared<AstNodeOperator>(
      opToken->GetOperatorType(),
      std::move(leftNode),
      std::move(rightNode)
    );
  }

  return leftNode;
}


// Evaluating AST


int64_t EvaluateAst(const AstNode* node) {
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
    default: {
      // WTF
      return 0;
    }
  }
}

int main() {
  std::string input = GetInput();
  TokenizerContext ctx(input);
  if (!ResolveExpression(ctx)) {
    std::cout << "Fail\n";
    return 0;
  }

  ParserContext parserCtx(ctx.GetTokens());
  std::shared_ptr<AstNode> astTree = BuildAst(parserCtx);
  if (!astTree) {
    std::cout << "Fail\n";
    return 0;
  }

  int64_t result = EvaluateAst(astTree.get());
  std::cout << "The result is: " << result << '\n';

  return 0;
}
