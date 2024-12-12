#include <iostream>
#include <string>
#include <sstream>

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
  if (!tokenizer.Resolve()) {
    std::cout << "Fail\n";
    return 0;
  }

  auto& tokens = tokenizer.GetTokens();
  // (x1_1 &&& x2) || (!x3 & x4)
  // TODO: if not does not match FOLLOW() - throw TokenizationError
  // Unexpected token "X" at line: 24, column: 8.
  return 0;
}