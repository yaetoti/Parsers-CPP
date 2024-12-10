#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "AstNodes.hpp"
#include "ParserContext.hpp"
#include "Parsing.hpp"
#include "Tokenization.hpp"

std::string GetInput() {
  std::stringstream stream;

  std::string line;
  while (std::getline(std::cin, line)) {
    stream << line << ' ';
  }

  return stream.str();
}

int main() {
  std::string input = GetInput();
  TokenizerContext ctx(input);
  if (!ResolveExpression(ctx)) {
    std::cout << "Fail\n";
    return 0;
  }

  ParserContext parserCtx(ctx.GetTokens());
  std::shared_ptr<AstNode> astTree = ParseExpression(parserCtx);
  if (!astTree) {
    std::cout << "Fail\n";
    return 0;
  }

  int64_t result = EvaluateAst(astTree.get());
  std::cout << "The result is: " << result << '\n';

  return 0;
}
