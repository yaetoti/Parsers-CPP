#pragma once

#include <memory>
#include <string>
#include <vector>

struct TruthTableRecord final {
  explicit TruthTableRecord(std::vector<bool> values, bool result)
  : values(std::move(values))
  , result(result) {
  }

  explicit TruthTableRecord(std::vector<bool>&& values, bool result)
  : values(std::move(values))
  , result(result) {
  }

  std::vector<bool> values;
  bool result;
};

struct TruthTable final {
  explicit TruthTable(std::vector<std::string> parameterNames)
  : m_parameterNames(std::move(parameterNames)) {
  }

  void EmplaceRecord(std::unique_ptr<TruthTableRecord>&& record) {
    m_records.emplace_back(std::move(record));
  }

  [[nodiscard]] const std::vector<std::string>& GetParameterNames() const {
    return m_parameterNames;
  }

  [[nodiscard]] const std::vector<std::unique_ptr<TruthTableRecord>>& GetRecords() const {
    return m_records;
  }

private:
  std::vector<std::string> m_parameterNames;
  std::vector<std::unique_ptr<TruthTableRecord>> m_records;
};
