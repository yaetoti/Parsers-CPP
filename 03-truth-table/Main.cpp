#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "Interpreter.hpp"
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

void TableToCSV(const std::string& filename, TruthTable& table) {
  std::ofstream out(filename);
  if (!out) {
    return;
  }

  const auto& names = table.GetParameterNames();
  const auto& records = table.GetRecords();

  // Print names
  for (size_t i = 0; i < names.size(); ++i) {
    out << names[i];
    out << ", ";
  }

  out << "Result\n";

  // Print data
  for (size_t recordId = 0; recordId < records.size(); ++recordId) {
    const auto& values = records.at(recordId)->values;
    bool result = records.at(recordId)->result;

    for (size_t i = 0; i < values.size(); ++i) {
      out << (values[i] ? "True" : "False");
      out << ", ";
    }

    out << (result ? "True" : "False");
    out << '\n';
  }
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

  Interpreter interpreter;
  interpreter.SetExpression(root);
  auto table = interpreter.GenerateTruthTable();

  TableToCSV("result.csv", table);

  // (x1_1 && x2) || (!x3 & x4)
  // TODO: if not does not match FOLLOW() - throw TokenizationError
  // Unexpected token "X" at line: 24, column: 8.
  return 0;
}