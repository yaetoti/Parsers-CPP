#pragma once

#include <unordered_set>
#include <string>
#include <string_view>

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


