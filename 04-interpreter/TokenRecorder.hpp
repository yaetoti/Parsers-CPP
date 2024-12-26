#pragma once

#include "LexerView.hpp"

struct TokenRecorder final {
  explicit TokenRecorder(const LexerView& view)
  : m_view(view)
  , m_savedState(view.GetState()) {
  }

  void Reset() {
    m_savedState = m_view.GetState();
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_savedState.position;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_savedState.line;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_savedState.column;
  }

  [[nodiscard]] const LexerView::State& GetSavedState() const {
    return m_savedState;
  }

  [[nodiscard]] std::string_view GetTokenView() const {
    return m_view.GetTokenView(m_savedState.position, m_view.GetPosition() - m_savedState.position);
  }

  [[nodiscard]] std::string GetToken() const {
    return m_view.GetToken(m_savedState.position, m_view.GetPosition() - m_savedState.position);
  }

private:
  const LexerView& m_view;
  LexerView::State m_savedState;
};