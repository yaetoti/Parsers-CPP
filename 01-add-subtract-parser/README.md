# Add-Subtract expression parser

## Goal

- Parse expressions of type

```
2 + 3 - 4 + 1
```

## Motivation

- Learn how lexers and parsers work
- Learn how to construct AST
- Apply knowledge from university courses
- Create possibilities for writing refactoring tools and intergating programming languages into bigger projects (DSL, developer console, programming in games)

## Solution

I decided to divide the task into 4 parts, so their result go to the next stage:

- Getting input
- Lexical analysis (Tokenization)
- Syntax analysis (Building an AST)
- Interpretation (Executing an AST)


TODO Input can retain newlines to tract line and column

Steps to design a parser:
- Preparation
  - Define grammar
  - Define tokens, important for building an AST
  - Define FIRST() functions for theese tokens (For the purpose of matching them)
- Programming a lexer
  - Create enumeration for token types, polymorphical hierarchy of classes for them (some tokens require storing numbers, strings, etc..)
  - Create TokenizerView (string + position)
  - Create Tokenizer class containing stack for result tokens and methods for tokenization
  - Define bool Match..(const TokenizerView& view) functions
  - Define bool Tokenize..(TokenizerView& view) functions for higher rules
- Programming a parser
  - Create enumeration for node types (Number, BinaryOperator)
  - Create AstNode polymorphical hierarchy. AstNodeBinaryOperator contains pointers to other AstNodes, regardless of their type
  - Define bool Parse..(ParserView& view) for parsing available tokens into corresponding AstNodes
- Programming an interpreter
  - Define a method int Evaluate(AstNode*) which defines type of the node and returns the result, recursing into subnodes when needed

TODO describe
- Getting lines process
- How lexer works
- How parser works
- How to execute
- What structures created, what methods were useful

Grammar:

```
Expression = Number ExpressionRepeat
ExpressionRepeat = Operator Number ExpressionRepeat
ExpressionRepeat = ε
Expression = Expression Operator Number

Number = Digit
Number = Digit Number
Digit = '0'..'9'
Operator = '+' | '-'
```

Screenshots:

Problem: Recursive approach generates children on the right
Solution: Use iterative approach

## Perspective

This project provided me with knowledge about general algorithms, useful data structures and holped to answer old questions about other APIs that I saw earlier. However, I still have questions about handling more complex scenarios like operator precedence. The way I imagine it is too complex, non-modular and error-prone.

- Create parser for more complex expressions with braces and operator precedence, like

```
2 + (2 - 3) * 8
```

- Implement error logging (e.g. "Unexpected token 'ABOBA' at line: 6, column: 9." or "Unexpected end of line.")
