#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <sstream>

//#include "Interpreter.hpp"
#include "Lexer.hpp"
#include "Matcher.hpp"
//#include "Parser.hpp"
//#include "Tokenizer.hpp"

std::string GetInput() {
  std::stringstream ss;
  std::string line;
  while (std::getline(std::cin, line)) {
    ss << line << '\n';
  }

  return ss.str();
}

struct SampleState final {
  size_t nestingLevel;
};

struct SampleContext final {
  [[nodiscard]] SampleState GetState() const {
    return SampleState(m_nestingLevel);
  }

  void SetState(const SampleState& state) {
    m_nestingLevel = state.nestingLevel;
  }

public:
  size_t m_nestingLevel = 0;
};

size_t m_nestingLevel = 0;

static constexpr auto IsWS = [](char c) { return c == ' ' || c == '\t' || c == '\f' || c == '\v' || c == '\r'; };
static constexpr auto IsNL = [](char c) { return c == '\n'; };
static constexpr auto IsNotNL = [](char c) { return c != '\n'; };
static constexpr auto IsSign = [](char c) { return c == '+' || c == '-'; };

bool ResolveComment(LexerView& view) {
  return Matcher(view).Any("//").ZeroMany(IsNotNL);
}

bool ResolveSkip(LexerView& view) {
  return (m_nestingLevel == 0)
  ? (Matcher(view).Any(IsWS, IsNL) || ResolveComment(view))
  : (Matcher(view).Any(IsWS) || ResolveComment(view));
}

bool ResolveIdentifier(LexerView& view) {
  return Matcher(view)
  .Many(isalpha)
  .CollectTokenRecorder([](const auto& recorder) {
    auto tokenView = recorder.GetTokenView();
    if (Grammar::IsKeyword(tokenView)) {
      return false;
    }

    std::cout << "IDENTIFIER (" << tokenView << ")\n";
    return true;
  });
}

bool ResolveNumber(LexerView& view) {
  bool isNegative = false;
  return Matcher(view)
  .Perform([&isNegative](auto& view) {
    if (view.Match(IsSign)) {
      isNegative = view.Next() == '-';
      view.Advance();
    }
  })
  .AssertZeroMany(ResolveSkip)
  .Record()
  .Many(isdigit)
  .CollectTokenRecorder([isNegative](const auto& recorder) {
    try {
      int64_t number = std::stoll(recorder.GetToken());
      number = isNegative ? -number : number;
      std::cout << "NUMBER (" << number << ")\n";
    } catch (std::exception&) {
      return false;
    }
  });
}

bool ResolveDereference(LexerView& view) {
  return Matcher(view).Any('$').Perform([](auto& view) {
    std::cout << "OPERATOR_DEREFERENCE\n";
  });
}

bool ResolveValue(LexerView& view) {
  return ResolveNumber(view)
  || Matcher(view)
  .Assert(ResolveDereference)
  .AssertZeroMany(ResolveSkip)
  .Assert(ResolveIdentifier);
}

bool ResolveOperatorAssignment(LexerView& view) {
  return Matcher(view).Any('=').Perform([](auto& view) {
    std::cout << "OPERATOR_ASSIGNMENT\n";
  });
}

bool ResolveAssignment(LexerView& view) {
  return Matcher(view)
  .Assert(ResolveIdentifier)
  .AssertZeroMany(ResolveSkip)
  .Assert(ResolveOperatorAssignment)
  .AssertZeroMany(ResolveSkip)
  .Assert(ResolveValue);
}

bool ResolveStatementChain(LexerView& view) {
  return Matcher(view)
  .AssertZeroMany(ResolveSkip)
  .Assert(ResolveAssignment)
  .AssertZeroMany(ResolveStatementChain);
}

int main() {
  std::string input = GetInput();
  LexerView view(input);
  SampleContext ctx;

  if (ResolveStatementChain(view)) {
    std::cout << "Success UwU\n";
    return 0;
  }

  std::cout << "Fail! FAIL!! YOU ARE A FAILURE!!11!\n";
  return 1;
}