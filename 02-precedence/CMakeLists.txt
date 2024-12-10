cmake_minimum_required(VERSION 3.28)
project(Parsing)

set(CMAKE_CXX_STANDARD 20)

add_executable(Parsing main.cpp
  OperatorType.hpp
  Tokenization.hpp
  Tokens.hpp
  TokenizerContext.hpp
  AstNodes.hpp
  ParserContext.hpp
  Parsing.hpp
  SpanUtils.hpp
)
