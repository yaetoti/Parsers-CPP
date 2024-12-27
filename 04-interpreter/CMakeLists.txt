cmake_minimum_required(VERSION 3.28)
project(Parsing)

set(CMAKE_CXX_STANDARD 20)

add_executable(Parsing
  Main.cpp
  Grammar.hpp
  StringUtils.hpp
  Tokens.hpp
  TokenRecorder.hpp
  LexerView.hpp
  Lexer.hpp
  Matcher.hpp
  AstNodes.hpp
  ParserView.hpp
  Parser.hpp
)
