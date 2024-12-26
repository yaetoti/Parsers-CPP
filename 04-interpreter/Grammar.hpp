#pragma once

#include <unordered_set>
#include <string>
#include <string_view>

/*

Interpreter's formal grammar:

Statement = StatementTerm Statement*
StatementTerm = Add | Sub | Mult | Print | VariableDeclaration | VariableDeletion | FunctionCall | FunctionDeclaration | Condition

Expression = OrExpr
OrExpr = AndExpr Or AndExpr
AndExpr = PrimaryExpr And PrimaryExpr
PrimaryExpr = Value Comparison Value

Condition = if Expression then Statement NewLine

FunctionDeclaration = Identifier function Statement NewLine
Loop = loop Value do Statement NewLine

Add = Identifier add Value
Sub = Identifier sub Value
Mult = Identifier mult Value
Print = print

VariableDeclaration = Identifier = Value
VariableDeletion = delete Identifier
FunctionCall = Identifier()

Value = Number | $Identifier
Comparison = Equal | NotEqual

Or = or
And = and
Equal = '=='
NotEqual = '!='
Number = Digit+
Identifier = IdentifierSymbol+

Digit = 0..9
IdentifierSymbol = a..z | A..Z

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


