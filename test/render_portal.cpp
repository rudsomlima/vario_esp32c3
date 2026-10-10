// Gera a pagina principal do portal em portal_preview.html (para conferir no navegador)
// g++ -std=c++17 -Istubs_esp -I.. render_portal.cpp -o render_portal && ./render_portal
#include <cstdio>
#include "Arduino.h"
#include "portal.h"
static float fakeBattery() { return 3.95f; }
int main() {
  SettingsStore st; PiezoIDF pz;
  portal::store = &st; portal::pz = &pz; portal::fwVersion = "0.4.0"; portal::batteryFn = fakeBattery;
  portal::handleRoot();
  FILE *f = fopen("portal_preview.html", "w");
  fputs(portal::server.lastBody.c_str(), f); fclose(f);
  printf("%u bytes\n", portal::server.lastBody.length());
}
