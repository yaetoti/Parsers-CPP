#pragma once

inline bool EqualsIgnoreCase(char c1, char c2) {
  return tolower(c1) == tolower(c2);
}

inline bool EqualsIgnoreCase(std::string_view s1, std::string_view s2) {
  if (s1.size() != s2.size()) {
    return false;
  }

  auto c1 = s1.begin(), c2 = s2.begin();
  while (c1 != s1.end() && tolower(*c1++) == tolower(*c2++))
    ;

  return c1 == s1.end();
}
