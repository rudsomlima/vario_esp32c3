// Testa a sentenca $LK8EX1 (formato + checksum + quebra por MTU)
// g++ -std=c++17 -Istubs_esp -I.. test_lk8ex1.cpp -o test_lk8ex1 && ./test_lk8ex1
#include <cstdio>
#include <cstring>
#include "Arduino.h"
#include "ble_lk8ex1.h"

static bool checkSentence(const std::string &s) {
  size_t star = s.find('*');
  if (s[0] != '$' || star == std::string::npos) return false;
  unsigned cs = 0;
  for (size_t i = 1; i < star; i++) cs ^= (unsigned char)s[i];
  unsigned got = 0;
  sscanf(s.c_str() + star + 1, "%2X", &got);
  return cs == got && s.substr(s.size() - 2) == "\r\n";
}

int main() {
  BleLK8EX1 b;
  b.begin("Vario-C3");
  int fails = 0;

  g_bleConnected = 0; g_bleOut.clear();
  b.send(95000.4f, 1.234f, 21.6f, 3.92f);
  printf("desconectado: %zu bytes enviados (esperado 0)\n", g_bleOut.size());
  fails += g_bleOut.size() != 0;

  struct { uint16_t mtu; float p, v, t, bat; } cases[] = {
    {64, 95000.4f, 1.234f, 21.6f, 3.92f},
    {23, 101325.f, -2.50f, -5.4f, 0.0f},     // MTU minimo -> em pedacos; sem bateria -> 999
    {64, 80000.f, 0.004f, 0.0f, 4.15f},
  };
  for (auto &c : cases) {
    g_bleConnected = 1; g_bleMtu = c.mtu; g_bleOut.clear(); g_bleNotifies = 0;
    b.send(c.p, c.v, c.t, c.bat);
    bool ok = checkSentence(g_bleOut);
    std::string shown = g_bleOut.substr(0, g_bleOut.size() - 2);
    printf("MTU %2u: %-48s notificacoes=%d checksum %s\n", c.mtu, shown.c_str(), g_bleNotifies, ok ? "OK" : "ERRADO");
    fails += !ok;
  }
  printf(fails ? "\nFALHOU (%d)\n" : "\nTUDO OK\n", fails);
  return fails;
}
