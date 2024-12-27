#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <sstream>

#include "Lexer.hpp"
#include "Matcher.hpp"
#include "Parser.hpp"

std::string GetInput() {
  std::stringstream ss;
  std::string line;
  while (std::getline(std::cin, line)) {
    ss << line << '\n';
  }

  return ss.str();
}

void PrintToken(const Token* token) {
  TokenType type = token->GetType();
  switch (type) {
    case TokenType::NUMBER: {
      const auto* tokenNumber = dynamic_cast<const TokenNumber*>(token);
      std::cout << "NUMBER(" << tokenNumber->GetValue() << ")\n";
      break;
    }
    case TokenType::IDENTIFIER: {
      const auto* tokenIdentifier = dynamic_cast<const TokenIdentifier*>(token);
      std::cout << "IDENTIFIER(" << tokenIdentifier->GetName() << ")\n";
      break;
    }
    case TokenType::OP_DEREFERENCE:
      std::cout << "OP_DEREFERENCE\n";
      break;
    case TokenType::OP_OR:
      std::cout << "OP_OR\n";
      break;
    case TokenType::OP_AND:
      std::cout << "OP_AND\n";
      break;
    case TokenType::OP_EQUAL:
      std::cout << "OP_EQUAL\n";
      break;
    case TokenType::OP_NOT_EQUAL:
      std::cout << "OP_NOT_EQUAL\n";
      break;
    case TokenType::OP_ASSIGN:
      std::cout << "OP_ASSIGN\n";
      break;
    case TokenType::OP_CALL:
      std::cout << "OP_CALL\n";
      break;
    case TokenType::KEYWORD_PRINT:
      std::cout << "KEYWORD_PRINT\n";
      break;
    case TokenType::KEYWORD_ADD:
      std::cout << "KEYWORD_ADD\n";
      break;
    case TokenType::KEYWORD_SUB:
      std::cout << "KEYWORD_SUB\n";
      break;
    case TokenType::KEYWORD_MULT:
      std::cout << "KEYWORD_MULT\n";
      break;
    case TokenType::KEYWORD_DELETE:
      std::cout << "KEYWORD_DELETE\n";
      break;
    case TokenType::KEYWORD_IF:
      std::cout << "KEYWORD_IF\n";
      break;
    case TokenType::KEYWORD_THEN:
      std::cout << "KEYWORD_THEN\n";
      break;
    case TokenType::KEYWORD_FUNCTION:
      std::cout << "KEYWORD_FUNCTION\n";
      break;
    case TokenType::KEYWORD_LOOP:
      std::cout << "KEYWORD_LOOP\n";
      break;
    case TokenType::KEYWORD_DO:
      std::cout << "KEYWORD_DO\n";
      break;
    case TokenType::BLOCK_END:
      std::cout << "BLOCK_END\n";
      break;
    default: break;
  }
}

void PrintTokens(const std::vector<std::unique_ptr<Token>>& tokens) {
  for (const auto& token : tokens) {
    PrintToken(token.get());
  }
}

int main() {
  std::string input = GetInput();
  Lexer lexer(input);
  if (!lexer.Tokenize()) {
    std::cout << "Fail! FAIL!!1 YOU ARE A FAILURE !!1!!1!\n";
    return 1;
  }

  std::cout << "Success UwU\n";
  const auto& tokens = lexer.GetTokens();
  PrintTokens(tokens);
  std::cout.flush();

  Parser parser(tokens);
  auto program = parser.Parse();

  return 0;

  // LexerView view(input);
  // SampleContext ctx;
  //
  // if (ResolveStatementChain(view)) {
  //   std::cout << "Success UwU\n";
  //   return 0;
  // }
  //
  // std::cout << "Fail! FAIL!! YOU ARE A FAILURE!!11!\n";
  // return 1;
}