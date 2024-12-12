cmake_minimum_required(VERSION 3.28)
project(Parsing)

set(CMAKE_CXX_STANDARD 20)

add_executable(Parsing
  Main.cpp
  TokenType.hpp
  TokenizerView.hpp
  Tokenizer.hpp
  StringUtils.hpp
  StringUtils.hpp
  StringUtils.hpp
  StringUtils.hpp
  Tokens.hpp
  TokenRecorder.hpp
  AstNodes.hpp
  ParserView.hpp
  Parser.hpp
)
