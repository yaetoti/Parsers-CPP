#pragma once

#include "TokenizerView.hpp"

struct TokenRecorder final {
  explicit TokenRecorder(const TokenizerView& view)
  : m_view(view)
  , m_position(view.GetPosition())
  , m_line(view.GetLine())
  , m_column(view.GetColumn()) {
  }

  [[nodiscard]] size_t GetPosition() const {
    return m_position;
  }

  [[nodiscard]] size_t GetLine() const {
    return m_position;
  }

  [[nodiscard]] size_t GetColumn() const {
    return m_position;
  }

  [[nodiscard]] std::string GetToken() const {
    return m_view.ExtractToken(m_position, m_view.GetPosition() - m_position);
  }

private:
  const TokenizerView& m_view;
  size_t m_position;
  size_t m_line;
  size_t m_column;
};