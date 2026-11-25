#include "uia.h"

#include <windows.h>

std::optional<TabHitResult> FindTabHitResult(POINT, bool, bool) {
  return std::nullopt;
}

bool SelectTab(const TabHitResult&) {
  return false;
}

std::optional<int> FindTabCount(HWND) {
  return std::nullopt;
}

bool IsOnTabBar(POINT) {
  return false;
}

bool IsOnBookmark(POINT) {
  return false;
}

bool IsOmniboxFocused() {
  return false;
}

bool IsOnNewTab(HWND, const std::vector<std::wstring>&) {
  return false;
}
