#pragma once
#include <stdint.h>

struct WifiRecoveryActions {
  bool startAccessPoint = false;
  bool reconnect = false;
};
// Control de tiempos sin bloquear la pantalla ni el portal. Seguro al desbordar millis().
class WifiRecovery {
 public:
  static constexpr uint32_t AP_DELAY_MS = 15000;
  static constexpr uint32_t RETRY_MS = 30000;
  static constexpr uint32_t AP_RETRY_MS = 5000;
  void begin(uint32_t now) {
    connected_ = false;
    disconnectedAt_ = lastAttempt_ = lastApAttempt_ = now;
  }
  WifiRecoveryActions tick(uint32_t now, bool connected, bool credentials, bool apActive, bool scanning) {
    WifiRecoveryActions actions;
    if (connected) { connected_ = true; return actions; }
    if (connected_) { begin(now); }
    if (!apActive && (!credentials || uint32_t(now - disconnectedAt_) >= AP_DELAY_MS) &&
        uint32_t(now - lastApAttempt_) >= AP_RETRY_MS) {
      actions.startAccessPoint = true;
      lastApAttempt_ = now;
    }
    if (credentials && !scanning && uint32_t(now - lastAttempt_) >= RETRY_MS) {
      actions.reconnect = true;
      lastAttempt_ = now;
    }
    return actions;
  }
 private:
  bool connected_ = false;
  uint32_t disconnectedAt_ = 0, lastAttempt_ = 0, lastApAttempt_ = 0;
};
