#pragma once

#include <span>
#include <initializer_list>

template <typename T>
std::span<T> ToSpan(std::initializer_list<T> list) {
  return std::span<const T>(list.begin(), list.size());
}
