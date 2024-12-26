cmake_minimum_required(VERSION 3.28)
project(Parsing)

set(CMAKE_CXX_STANDARD 20)

add_executable(Parsing
  Main.cpp
  Grammar.hpp
  #Tokenizer.hpp
  StringUtils.hpp
  Tokens.hpp
  TokenRecorder.hpp
  #AstNodes.hpp
  #ParserView.hpp
  #Parser.hpp
  #Interpreter.hpp
  #DynamicBitset.hpp
  #TruthTable.hpp
  LexerView.hpp
  Lexer.hpp
  Matcher.hpp
)
