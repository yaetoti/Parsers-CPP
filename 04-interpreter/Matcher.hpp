#pragma once
#include "LexerView.hpp"
#include "TokenRecorder.hpp"

template <typename Func>
concept LexerViewPredicate = requires(Func f, LexerView& view) {
  { f(view) } -> std::convertible_to<bool>;
};

template <typename Func>
concept LexerViewConsumer = requires(Func f, LexerView& view) {
  f(view);
};

// template <typename Func>
// concept StringViewPredicate = requires(Func f, std::string_view view) {
//   { f(view) } -> std::convertible_to<bool>;
// };

template <typename Func>
concept TokenRecorderPredicate = requires(Func f, const TokenRecorder& recorder) {
  { f(recorder) } -> std::convertible_to<bool>;
};

struct Matcher final {
  explicit Matcher(LexerView& view)
  : m_view(view)
  , m_savedState(view.GetState())
  , m_recorder(view)
  , m_isOk(true) {
  }

  operator bool() const {
    return m_isOk;
  }

  // Matcher& Store() {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_savedState = m_view.GetState();
  //   return *this;
  // }
  //
  // Matcher& Restore() {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_view.SetState(m_savedState);
  //   return *this;
  // }

  Matcher& Record() {
    if (!m_isOk) {
      return *this;
    }

    m_recorder.Reset();
    return *this;
  }

  std::string GetToken() {
    return m_recorder.GetToken();
  }

  std::string_view GetTokenView() {
    return m_recorder.GetTokenView();
  }

  // ??? WTF are you doing here? You want to repeat what we did yesterday?
  // template <StringViewPredicate Predicate>
  // Matcher& CollectTokenView(Predicate predicate) {
  //   if (!m_isOk) {
  //     return *this;
  //   }
  //
  //   m_isOk = predicate(m_recorder.GetTokenView());
  //   RestoreIfFailed();
  //   return *this;
  // }

  // Shitty function. What if I want line? column? tokenView? token?
  template <TokenRecorderPredicate Predicate>
  Matcher& CollectTokenRecorder(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = predicate(m_recorder);
    RestoreIfFailed();
    return *this;
  }

  // .CollectTokenView()???

  // Actions: Reset (OK) - no application, RestoreIfOK - no application (LexerView::State), RestoreIfFailed - utility function, called automatically (LexerView::State)
  // Philosophy: IF state is failed, view is momentarely reset and you cannot perform further actions
  // Reset???
  // RestoreIfOK???
  // ResetAndRestore???

  // .Store()???
  // .Restore()???



  // .Optional(char...)
  // .Optional(string_view...)
  // .Optional(CharPredicate...)

  template <typename... Chars>
  Matcher& Optional(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(chars...);
    return *this;
  }

  template <StringViewIsh... StringViews>
  Matcher& Optional(StringViews... views) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(views...);
    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Optional(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_view.MatchAdvance(predicate...);
    return *this;
  }

  // Advancing, state-changing

  // +.Any(char...)
  // +.Any(string_view...)
  // +.Any(CharPredicate...)

  template <typename... Chars>
  Matcher& Any(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(chars...);
    RestoreIfFailed();
    return *this;
  }

  template <StringViewIsh... StringViews>
  Matcher& Any(StringViews... views) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(views...);
    RestoreIfFailed();
    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Any(Predicate... predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = m_view.MatchAdvance(predicate...);
    RestoreIfFailed();
    return *this;
  }

  // Advancing, non-state-changing
  // +.ZeroMany(char...)
  // +.ZeroMany(string_view...)
  // +.ZeroMany(CharPredicate...)
  template <typename... Chars>
  Matcher& ZeroMany(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(chars...)) {
    }

    return *this;
  }

  template <StringViewIsh... Views>
  Matcher& ZeroMany(Views... views) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(views...)) {
    }

    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& ZeroMany(Predicate... predicates) {
    if (!m_isOk) {
      return *this;
    }

    while (m_view.MatchAdvance(predicates...)) {
    }

    return *this;
  }

  // +.Many(char...)
  // +.Many(string_view...)
  // +.Many(CharPredicate...)

  template <typename... Chars>
  Matcher& Many(Chars... chars) requires (... && std::same_as<Chars, char>) {
    if (!Any(chars...)) {
      return *this;
    }

    while (m_view.MatchAdvance(chars...)) {
    }

    return *this;
  }

  template <StringViewIsh... Views>
  Matcher& Many(Views... views) {
    if (!Any(views...)) {
      return *this;
    }

    while (m_view.MatchAdvance(views...)) {
    }

    return *this;
  }

  template <CharPredicate... Predicate>
  Matcher& Many(Predicate... predicates) {
    if (!m_isOk) {
      return *this;
    }

    if (!Any(predicates...)) {
      m_isOk = false;
      RestoreIfFailed();
      return *this;
    }

    while (m_view.MatchAdvance(predicates...)) {
    }

    return *this;
  }

  // State-changing, non-advancing
  // .Assert(bool)

  Matcher& Assert(bool value) {
    m_isOk &= value;
    RestoreIfFailed();
    return *this;
  }

  // State-changing, non-advancing
  // .Assert(ViewPredicate)???
  // .AssertMany(ViewPredicate)???
  // .AssertZeroMany(ViewPredicate)???

  template <LexerViewPredicate Predicate>
  Matcher& Assert(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    m_isOk = predicate(m_view);
    RestoreIfFailed();
    return *this;
  }

  template <LexerViewPredicate Predicate>
  Matcher& AssertZeroMany(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    while (predicate(m_view)) {
    }

    return *this;
  }

  template <LexerViewPredicate Predicate>
  Matcher& AssertMany(Predicate predicate) {
    if (!m_isOk) {
      return *this;
    }

    if (!predicate(m_view)) {
      m_isOk = false;
      RestoreIfFailed();
    }

    while (predicate(m_view)) {
    }

    return *this;
  }

  // Why not MatcherPredicate???
  // What can we do with a matcher?
  // We can capture a matcher. I don't want to write matcher->getView every fucking time

  // non-state-changing, non-advancing
  // .Perform(ViewConsumer)???

  template <LexerViewConsumer Consumer>
  Matcher& Perform(Consumer consumer) {
    if (m_isOk) {
      consumer(m_view);
    }

    return *this;
  }

private:
  void RestoreIfFailed() {
    if (!m_isOk) {
      m_view.SetState(m_savedState);
    }
  }

private:
  LexerView& m_view;
  LexerView::State m_savedState;
  TokenRecorder m_recorder;
  bool m_isOk;
};
