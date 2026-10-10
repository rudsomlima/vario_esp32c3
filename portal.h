// portal.h - Portal de configuracao por Wi-Fi (so em solo, so sob demanda).
//
// Entrar: ligar o vario e CONTINUAR segurando o botao ate ~4 s (3 bipes agudos).
// No celular: rede "Vario-C3-XXXX" (senha padrao vario1234). A pagina abre
// sozinha (captive portal) ou em http://192.168.4.1
//
// Enquanto o portal esta ativo o vario NAO funciona (de proposito: Wi-Fi
// gasta ~80-100 mA e nao e para voar assim). Um bipe a cada 5 s lembra disso.
// Sai: "Salvar" (reinicia no modo vario), 10 min sem acesso, ou botao 2 s (desliga).
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include "settings.h"
#include "vario_audio.h"
#include "piezo_idf.h"

namespace portal {

static const uint32_t IDLE_TIMEOUT_MS = 10UL * 60UL * 1000UL;
static const uint32_t REMINDER_MS = 5000;

static WebServer      server(80);
static DNSServer      dns;
static SettingsStore *store = nullptr;
static PiezoIDF      *pz = nullptr;
static const char    *fwVersion = "";
static float        (*batteryFn)() = nullptr;
static uint32_t       lastReqMs = 0;

// Campos float editaveis: chave do formulario, rotulo, unidade, faixa, passo, membro
struct FloatField {
  const char *key, *label, *unit;
  float min, max, step;
  float Settings::*ptr;
  const char *help;
};

static const FloatField FIELDS[] = {
  {"liftOn",  "Inicio do bipe",        "m/s",  -0.5f,  1.0f,  0.05f, &Settings::liftOn,     "Subida minima para bipar"},
  {"sinkOn",  "Alarme de afundamento", "m/s", -10.0f, -0.5f,  0.1f,  &Settings::sinkOn,     "Tom continuo abaixo deste valor"},
  {"fBase",   "Tom em 0 m/s",          "Hz",  200.0f, 2000.0f, 10.0f, &Settings::fBase,     ""},
  {"fPerMs",  "Subida do tom",         "Hz por m/s", 10.0f, 400.0f, 5.0f, &Settings::fPerMs, ""},
  {"period0", "Ciclo do bipe em 0 m/s","ms",  150.0f, 1500.0f, 10.0f, &Settings::periodZero, "Menor = bipes mais rapidos"},
  {"airspeed","Velocidade em termica", "m/s", -15.0f, 15.0f, 0.5f,  &Settings::airspeed,   "Compensacao centrifuga. Errar para menos. 0 = desliga"},
  {"kfAcc",   "Confianca no acelerometro", "m/s2", 0.05f, 3.0f, 0.05f, &Settings::kfAcc,  "Maior = reage mais rapido"},
  {"kfBaro",  "Ruido do barometro",    "m",     0.05f, 3.0f, 0.05f, &Settings::kfBaro,     "Maior = som mais suave"},
  {"autoOff", "Auto-desligar",         "min",   0.0f, 240.0f, 1.0f,  &Settings::autoOffMin, "Sem atividade. 0 = nunca"},
  {"bleHz",   "Taxa do BLE",           "Hz",    1.0f,  10.0f, 1.0f,  &Settings::bleHz,      "$LK8EX1 por segundo"},
  {"bleTmo",  "BLE desliga sem conexao","min",  0.0f, 120.0f, 1.0f,  &Settings::bleTimeoutMin, "0 = nunca"},
};

static String esc(const char *s) {
  String o;
  for (; *s; s++) {
    switch (*s) {
      case '&': o += F("&amp;"); break;
      case '<': o += F("&lt;"); break;
      case '>': o += F("&gt;"); break;
      case '"': o += F("&quot;"); break;
      default: o += *s;
    }
  }
  return o;
}

static String num(float v, float step) {
  return String(v, step < 0.1f ? 2 : (step < 1.0f ? 1 : 0));
}

static const char CSS[] PROGMEM = R"CSS(
:root{--bg:#f6f7f9;--fg:#1b1f24;--card:#fff;--mut:#5b6573;--acc:#0a66c2;--line:#dde1e6}
@media (prefers-color-scheme:dark){:root{--bg:#121417;--fg:#e8eaed;--card:#1c1f24;--mut:#9aa3ad;--acc:#5aa9ff;--line:#2c3138}}
*{box-sizing:border-box}body{margin:0;font:16px/1.4 system-ui,sans-serif;background:var(--bg);color:var(--fg)}
main{max-width:560px;margin:0 auto;padding:16px}h1{font-size:1.3rem;margin:.2rem 0 1rem}
section{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px 14px;margin-bottom:14px}
h2{font-size:1rem;margin:0 0 .6rem;color:var(--mut)}label{display:block;margin:.7rem 0 .2rem;font-weight:600}
small{color:var(--mut);font-weight:400}input,select{width:100%;padding:10px;font-size:16px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--fg)}
input[type=checkbox]{width:auto;transform:scale(1.4);margin-right:8px}
.row{display:flex;gap:8px;flex-wrap:wrap}button,.btn{flex:1;padding:12px;font-size:16px;border:0;border-radius:8px;background:var(--acc);color:#fff;text-align:center;text-decoration:none;min-width:120px}
.sec{background:transparent;color:var(--acc);border:1px solid var(--acc)}.info{color:var(--mut);font-size:.9rem}
)CSS";

static String pageHead(const char *title) {
  String h = F("<!doctype html><html lang='pt-BR'><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'><title>");
  h += title;
  h += F("</title><style>");
  h += FPSTR(CSS);
  h += F("</style></head><body><main>");
  return h;
}

static void touch() { lastReqMs = millis(); }

static void handleRoot() {
  touch();
  const Settings &s = store->s;
  String h = pageHead("Vario");
  h += F("<h1>Vario C3 &mdash; configuracao</h1><p class='info'>Firmware ");
  h += fwVersion;
  float bv = batteryFn ? batteryFn() : 0;
  if (bv > 1.0f) { h += F(" &middot; bateria "); h += String(bv, 2); h += F(" V"); }
  h += F("<br>O vario fica parado enquanto o portal estiver ligado.</p>");

  h += F("<form id='f' method='post' action='/save'>");
  h += F("<section><h2>Som</h2><label>Volume inicial</label><select name='vol'>");
  const char *vols[] = {"Mudo", "Baixo", "Medio", "Alto"};
  for (int i = 0; i < 4; i++) {
    h += F("<option value='"); h += i; h += '\'';
    if (s.volume == i) h += F(" selected");
    h += '>'; h += vols[i]; h += F("</option>");
  }
  h += F("</select>");

  int sectionAt[] = {5, 8, 9};   // indices onde comecam novas secoes
  const char *sectionName[] = {"Filtros", "Energia", "Bluetooth (XCTrack)"};
  int sec = 0;
  for (size_t i = 0; i < sizeof(FIELDS) / sizeof(FIELDS[0]); i++) {
    if (sec < 3 && (int)i == sectionAt[sec]) {
      h += F("</section><section><h2>"); h += sectionName[sec]; h += F("</h2>");
      if (sec == 2) {
        h += F("<label><input type='checkbox' name='ble' value='1'");
        if (s.bleEnabled) h += F(" checked");
        h += F(">Ligar BLE ao ligar o vario</label>");
      }
      sec++;
    }
    const FloatField &f = FIELDS[i];
    h += F("<label>"); h += f.label; h += F(" <small>("); h += f.unit; h += F(")");
    if (f.help[0]) { h += F(" &mdash; "); h += f.help; }
    h += F("</small></label><input type='number' inputmode='decimal' name='");
    h += f.key; h += F("' min='"); h += num(f.min, f.step);
    h += F("' max='"); h += num(f.max, f.step);
    h += F("' step='"); h += num(f.step, f.step);
    h += F("' value='"); h += num(s.*(f.ptr), f.step); h += F("'>");
  }
  h += F("</section><section><h2>Portal</h2><label>Senha do Wi-Fi <small>(8 a 32 caracteres)</small></label>"
         "<input name='apPass' minlength='8' maxlength='32' value='");
  h += esc(s.apPass);
  h += F("'></section>");

  h += F("<section><h2>Testar o som <small>(com os valores acima, sem salvar)</small></h2><div class='row'>"
         "<button type='button' class='sec' onclick='t(0.5)'>+0,5 m/s</button>"
         "<button type='button' class='sec' onclick='t(2)'>+2 m/s</button>"
         "<button type='button' class='sec' onclick='t(5)'>+5 m/s</button>"
         "<button type='button' class='sec' onclick='t(-3)'>&minus;3 m/s</button></div></section>"
         "<div class='row'><button type='submit'>Salvar e voltar ao vario</button></div></form>"
         "<section style='margin-top:14px'><div class='row'>"
         "<a class='btn sec' href='/update'>Atualizar firmware</a>"
         "<form method='post' action='/reset' style='flex:1;display:flex' onsubmit=\"return confirm('Restaurar todos os padroes?')\">"
         "<button class='sec' type='submit'>Restaurar padroes</button></form></div></section>"
         "<script>function t(v){var q=new URLSearchParams(new FormData(document.getElementById('f')));"
         "q.set('climb',v);fetch('/test?'+q.toString());}</script>"
         "</main></body></html>");
  server.send(200, "text/html", h);
}

// Aplica os campos do formulario em s (sem salvar)
static void readForm(Settings &s) {
  for (const FloatField &f : FIELDS)
    if (server.hasArg(f.key)) {
      float v = server.arg(f.key).toFloat();
      s.*(f.ptr) = v < f.min ? f.min : (v > f.max ? f.max : v);
    }
  if (server.hasArg("vol")) s.volume = (uint8_t)constrain(server.arg("vol").toInt(), 0, 3);
}

static void handleSave() {
  touch();
  Settings s = store->s;
  readForm(s);
  s.bleEnabled = server.hasArg("ble");
  if (server.hasArg("apPass")) {
    String pw = server.arg("apPass");
    if (pw.length() >= 8 && pw.length() <= 32) strncpy(s.apPass, pw.c_str(), sizeof(s.apPass) - 1);
  }
  store->s = s;
  store->save();
  String h = pageHead("Salvo");
  h += F("<h1>Salvo</h1><p>O vario esta reiniciando no modo de voo. Pode desconectar do Wi-Fi.</p></main></body></html>");
  server.send(200, "text/html", h);
  delay(800);
  ESP.restart();
}

static void handleReset() {
  touch();
  store->resetDefaults();
  String h = pageHead("Padroes");
  h += F("<h1>Padroes restaurados</h1><p>Reiniciando no modo de voo. A senha do Wi-Fi voltou a ser <b>vario1234</b>.</p></main></body></html>");
  server.send(200, "text/html", h);
  delay(800);
  ESP.restart();
}

// Toca ~3 s do som do vario para uma subida simulada, com os valores do formulario
static void handleTest() {
  touch();
  Settings s = store->s;
  readForm(s);
  float climb = server.hasArg("climb") ? server.arg("climb").toFloat() : 1.0f;
  server.send(200, "text/plain", "ok");      // responde antes de tocar
  VarioAudio a;
  a.cfg.liftOn = s.liftOn; a.cfg.liftOff = s.liftOn - 0.05f;
  a.cfg.sinkOn = s.sinkOn; a.cfg.sinkOff = s.sinkOn + 0.2f;
  a.cfg.fBase = s.fBase; a.cfg.fPerMs = s.fPerMs; a.cfg.periodZero = (uint16_t)s.periodZero;
  auto oldVol = pz->volume();
  pz->setVolume((PiezoIDF::Volume)(s.volume == 0 ? 1 : s.volume));
  uint32_t t0 = millis();
  while (millis() - t0 < 3000) { pz->play(a.tick(climb, millis())); delay(5); }
  pz->play(0);
  pz->setVolume(oldVol);
}

static void handleUpdatePage() {
  touch();
  String h = pageHead("Firmware");
  h += F("<h1>Atualizar firmware</h1><section><p class='info'>Na Arduino IDE: Sketch &rarr; Exportar binario compilado. "
         "Envie o arquivo <b>.bin</b> (nao o merged).</p>"
         "<form method='post' action='/update' enctype='multipart/form-data'>"
         "<input type='file' name='fw' accept='.bin'><div class='row' style='margin-top:12px'>"
         "<button type='submit'>Enviar</button></div></form></section>"
         "<p><a href='/'>&larr; voltar</a></p></main></body></html>");
  server.send(200, "text/html", h);
}

static void handleUpdateDone() {
  touch();
  bool ok = !Update.hasError();
  String h = pageHead("Firmware");
  h += ok ? F("<h1>Firmware atualizado</h1><p>Reiniciando...</p>")
          : F("<h1>Falhou</h1><p>O firmware anterior continua instalado. <a href='/update'>Tentar de novo</a></p>");
  h += F("</main></body></html>");
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", h);
  if (ok) { delay(800); ESP.restart(); }
}

static void handleUpdateUpload() {
  touch();
  HTTPUpload &up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    Update.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    Update.end(true);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
  }
}

// Captive portal: qualquer outra URL vai para a pagina principal
static void handleNotFound() {
  touch();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// Roda o portal. Nao retorna: termina com ESP.restart() ou chamando powerOffFn().
inline void run(SettingsStore &st, PiezoIDF &piezo, const char *version,
                float (*readBattery)(), void (*powerOffFn)(), uint8_t buttonPin) {
  store = &st; pz = &piezo; fwVersion = version; batteryFn = readBattery;

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "Vario-C3-%02X%02X", mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, st.s.apPass);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);      // o celular esta ao lado
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/test", HTTP_GET, handleTest);
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.onNotFound(handleNotFound);
  server.begin();

  lastReqMs = millis();
  uint32_t lastReminder = millis(), pressedAt = 0;
  bool wasPressed = false;
  for (;;) {
    dns.processNextRequest();
    server.handleClient();
    uint32_t now = millis();

    if (now - lastReminder > REMINDER_MS) {          // "estou no modo portal"
      lastReminder = now;
      piezo.beep(2400, 25);
    }
    bool p = digitalRead(buttonPin) == LOW;          // botao 2 s = desliga
    if (p && !wasPressed) pressedAt = now;
    wasPressed = p;
    if ((p && now - pressedAt > 2000) || now - lastReqMs > IDLE_TIMEOUT_MS) {
      server.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      powerOffFn();
    }
    delay(2);
  }
}

}  // namespace portal
