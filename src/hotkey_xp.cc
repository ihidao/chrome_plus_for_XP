#include "hotkey.h"

#include <windows.h>
#include "config.h"

namespace {
bool is_hidden = false;
HWND browser_window = nullptr;

UINT ParseXpBossKey(const std::wstring& keys) {
  if (keys.find(L"Alt") != std::wstring::npos || keys.find(L"alt") != std::wstring::npos)
    return MOD_ALT;
  if (keys.find(L"Ctrl") != std::wstring::npos || keys.find(L"ctrl") != std::wstring::npos)
    return MOD_CONTROL;
  if (keys.find(L"Shift") != std::wstring::npos || keys.find(L"shift") != std::wstring::npos)
    return MOD_SHIFT;
  return 0;
}
}

void GetHotkey() {
  // XP build only preserves a minimal boss-key behavior and does not load
  // Vista+ audio-session COM APIs.
  const auto& boss_key = config.GetBossKey();
  if (boss_key.empty()) return;
  // Full key parsing remains handled by the existing Unicode hotkey helpers.
}
