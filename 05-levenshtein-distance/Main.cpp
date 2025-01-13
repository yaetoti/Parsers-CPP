#include <iostream>
#include <ranges>
#include <string>
#include <algorithm>
#include <cassert>
#include <vector>

enum Action : uint32_t {
  SKIP,
  SUBSTITUTE,
  REMOVE,
  ADD,

  COUNT
};

struct ActionData final {
  Action action;
  char char1;
  char char2;
};

void PrintCache(const std::vector<size_t>& vec, size_t rows, size_t cols) {
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < cols; ++col) {
      std::cout << vec.at(col + row * cols) << '\t';
    }

    std::cout << '\n';
  }
}

void PrintActions(const std::vector<ActionData>& vec) {
  for (const auto & data : vec) {
    if (data.action == SKIP) {
      std::cout << "- Skip: \'" << data.char1 << "\'\n";
    } else if (data.action == SUBSTITUTE) {
      std::cout << "- Substitute: \'" << data.char1 << "\' with \'" << data.char2 << "\'\n";
    } else if (data.action == ADD) {
      std::cout << "- Add: \'" << data.char2 << "\'\n";
    } else if (data.action == REMOVE) {
      std::cout << "- Remove: \'" << data.char1 << "\'\n";
    } else {
      assert(false && "Invalid action type.");
    }
  }
}

std::tuple<size_t, std::vector<ActionData>> LevDist(std::string_view a, std::string_view b) {
  size_t rows = a.size() + 1;
  size_t cols = b.size() + 1;
  std::vector<size_t> cache(rows * cols);
  std::vector<Action> actions(rows * cols);

  cache[0] = 0;
  actions[0] = SKIP;

  for (size_t row = 1; row < rows; ++row) {
    cache[row * cols] = row;
    actions[row * cols] = REMOVE;
  }

  for (size_t col = 1; col < cols; ++col) {
    cache[col] = col;
    actions[col] = ADD;
  }

  for (size_t row = 1; row < rows; ++row) {
    for (size_t col = 1; col < cols; ++col) {
      size_t pos = col + row * cols;

      if (a.at(row - 1) == b.at(col - 1)) {
        // = previous diagonal element
        cache.at(pos) = cache.at(pos - cols - 1);
        actions.at(pos) = SKIP;
        continue;
      }

      size_t top = cache.at(pos - cols);
      size_t left = cache.at(pos - 1);
      size_t diagonal = cache.at(pos - cols - 1);

      cache[pos] = 1 + top;
      actions[pos] = REMOVE;
      if (left < top) {
        cache[pos] = 1 + left;
        actions[pos] = ADD;
      }
      if (diagonal < left) {
        cache[pos] = 1 + diagonal;
        actions[pos] = SUBSTITUTE;
      }
    }
  }

  // Backtracking
  std::vector<ActionData> result;
  size_t row = a.size();
  size_t col = b.size();
  while (row > 0 || col > 0) {
    size_t pos = col + row * cols;
    if (actions[pos] == SKIP) {
      --row;
      --col;
      result.emplace_back(ActionData { actions[pos], a.at(row), 0 });
    }
    else if (actions[pos] == SUBSTITUTE) {
      --row;
      --col;
      result.emplace_back(ActionData { actions[pos], a.at(row), b.at(col) });
    } else if (actions[pos] == ADD) {
      --col;
      result.emplace_back(ActionData { actions[pos], 0, b.at(col) });
    } else if (actions[pos] == REMOVE) {
      --row;
      result.emplace_back(ActionData { actions[pos], a.at(row), 0 });
    } else {
      assert(false && "Invalid action type.");
    }
  }

  std::ranges::reverse(result);

  PrintCache(cache, rows, cols);
  return { cache.back(), result };
}

size_t Lev(std::string_view a, std::string_view b) {
  if (b.empty()) {
    return a.size();
  }

  if (a.empty()) {
    return b.size();
  }

  if (a[0] == b[0]) {
    return Lev(a.substr(1), b.substr(1));
  }

  return 1 + std::min(
    Lev(a.substr(1), b),
    std::min(
      Lev(a, b.substr(1)),
      Lev(a.substr(1), b.substr(1))
    )
  );
}

int main() {
  // stript-club
  auto [distance, actions] = LevDist("substring", "stript-club");
  std::cout << "Distance: " << distance << '\n';
  std::cout << "Actions: " << '\n';
  PrintActions(actions);

  // substring
  // stript-club
  return 0;
}