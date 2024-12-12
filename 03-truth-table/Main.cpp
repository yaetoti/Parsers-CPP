#include <iostream>
#include <string>
#include <sstream>

#include "Parser.hpp"
#include "Tokenizer.hpp"

std::string GetInput() {
  std::stringstream ss;
  std::string line;
  while (std::getline(std::cin, line)) {
    ss << line << '\n';
  }

  return ss.str();
}

int main() {
  std::string input = GetInput();
  Tokenizer tokenizer(input);
  try {
    if (!tokenizer.Resolve()) {
      std::cout << "Unknown tokenization error\n";
      return 0;
    }
  } catch (const TokenizationError& e) {
    std::stringstream ss;
    ss << "[TE001] Unexpected symbol: \"";
    ss << e.token;
    ss << "\" at line ";
    ss << e.line;
    ss << ", column ";
    ss << e.column;
    ss << ".\n";
    std::cout << ss.str();
    return 0;
  }

  const auto& tokens = tokenizer.GetTokens();
  Parser parser(tokens);
  auto root = parser.Parse();
  if (!root) {
    std::cout << "Unknown semantic analysis error\n";
    return 0;
  }

  // (x1_1 && x2) || (!x3 & x4)
  // TODO: if not does not match FOLLOW() - throw TokenizationError
  // Unexpected token "X" at line: 24, column: 8.
  return 0;
}