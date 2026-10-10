// sim_flight.cpp - Simulacao de voo para validar Mahony + Kalman + audio no PC.
//
// g++ -O2 -std=c++17 -Istubs -I.. sim_flight.cpp -o sim_flight && ./sim_flight
//
// Verdade em 1 kHz; MPU a 50 Hz; MS5611 a 25 Hz. Sensores com ruido e erros
// tipicos de um MPU-6050 barato (offset de acelerometro, erro de escala,
// bias residual do giro).
#define _USE_MATH_DEFINES
#include <cstdio>
#include <cmath>
#include <random>
#include <vector>
#include "Arduino.h"
#include "ms5611.h"
#include "mahony_gravity.h"
#include "kalman_vario.h"
#include "vario_audio.h"

static const double G = 9.80665;
std::mt19937 rng(42);
std::normal_distribution<double> N01(0.0, 1.0);

struct Truth {
  double h = 1000, vz = 0;       // altitude e vel. vertical do PILOTO
  double roll = 0, yaw = 0;      // atitude do piloto (pendula alinhado com a gravidade aparente)
  double rollRate = 0, yawRate = 0;
  double fx = 0, fy = 0, fz = G; // forca especifica no corpo (o que o acelerometro mede)
};

// Cenario: devolve vel. vertical da asa, bank de curva e balanco de pendulo
struct Scenario {
  // retorna: wing climb (m/s), bank coordenado (rad), angulo de balanco (rad)
  virtual void at(double t, double &wingVz, double &bank, double &swing) = 0;
  virtual ~Scenario() {}
};

static double smoothstep(double t, double t0, double dur) {
  double x = (t - t0) / dur;
  if (x <= 0) return 0;
  if (x >= 1) return 1;
  return x * x * (3 - 2 * x);
}

static float gAssumedV = 9.0f;   // velocidade de trim configurada no vario
static double gBankDeg = 30.0;

struct StepScenario : Scenario {   // planeio -1.0 m/s -> termica +2.0 m/s em 0.5 s
  void at(double t, double &v, double &b, double &s) override {
    v = -1.0 + 3.0 * smoothstep(t, 20.0, 0.5); b = 0; s = 0;
  }
};
struct TurnScenario : Scenario {   // subida constante +2.0, entra em curva 30 graus
  void at(double t, double &v, double &b, double &s) override {
    v = 2.0;
    b = (gBankDeg * M_PI / 180.0) * (smoothstep(t, 20.0, 3.0) - smoothstep(t, 80.0, 3.0));
    s = 0;
  }
};
struct SwingScenario : Scenario {  // descendo -1.0 com balanco lateral de 15 graus
  void at(double t, double &v, double &b, double &s) override {
    v = -1.0; b = 0;
    s = (15.0 * M_PI / 180.0) * smoothstep(t, 10.0, 5.0) * sin(1.5 * t);  // ~0.24 Hz
  }
};

struct Result { double t50 = -1, t90 = -1, maxErr = 0, rmsErr = 0; int falseBeepTicks = 0; };

// useAccel=false -> vario "so barometro" (mesmo Kalman com u = 0) para comparar
static Result run(Scenario &sc, bool useAccel, double tEnd, double errFrom,
                  bool stepMetric, double falseBeepIfTrueBelow, bool print) {
  const double dtT = 0.001;
  const double L = 7.0, V = 10.0;   // comprimento das linhas (m), velocidade (m/s)

  // erros do sensor
  const double abx = 0.03, aby = -0.02, abz = 0.04;  // offset acel (g)
  const double ascale = 1.015;                       // erro de escala
  const double gbx = 0.3 * M_PI / 180, gby = -0.2 * M_PI / 180, gbz = 0.25 * M_PI / 180; // rad/s residual
  const double aNoise = 0.0018, gNoise = 0.0005;     // rms por amostra a 50 Hz
  const double baroNoise = 0.20;                     // m rms

  Truth tr;
  MahonyGravity ahrs;
  KalmanVario kf;
  VarioAudio audio;

  // "calibracao" no solo: escala do acelerometro para |a|=1 g em repouso
  double ax0 = abx * ascale, ay0 = aby * ascale, az0 = (1.0 + abz) * ascale;
  float accelScale = (float)(1.0 / sqrt(ax0 * ax0 + ay0 * ay0 + az0 * az0));
  ahrs.begin();
  ahrs.airspeed = gAssumedV;
  ahrs.alignToAccel((float)ax0, (float)ay0, (float)az0);
  if (useAccel) kf.begin((float)tr.h, 0.35f, 0.02f, 0.25f);
  else          kf.begin((float)tr.h, 1.0f, 1e-6f, 0.25f);

  double swingPrev = 0, swingRatePrev = 0, wingH = tr.h;
  double v0 = 0, v1 = 0, dummy1, dummy2;
  sc.at(0, v0, dummy1, dummy2);
  sc.at(tEnd, v1, dummy1, dummy2);

  Result r; double sumSq = 0; int nErr = 0;
  int step = 0;
  for (double t = 0; t < tEnd; t += dtT, step++) {
    double wingVz, bank, swing;
    sc.at(t, wingVz, bank, swing);
    double swingRate = (swing - swingPrev) / dtT;
    double swingAcc = (swingRate - swingRatePrev) / dtT;
    if (step < 2) { swingRate = 0; swingAcc = 0; }
    swingPrev = swing; swingRatePrev = swingRate;

    // asa
    double wingVzPrev = tr.vz;
    wingH += wingVz * dtT;
    // pendulo: piloto a L abaixo da asa, angulo "swing" no plano lateral
    double pz = -L * cos(swing);
    double pzd = L * sin(swing) * swingRate;
    double pzdd = L * (cos(swing) * swingRate * swingRate + sin(swing) * swingAcc);
    double pyd = L * cos(swing) * swingRate;   (void)pyd;
    double pydd = L * (-sin(swing) * swingRate * swingRate + cos(swing) * swingAcc);

    double hNew = wingH + pz;
    double vzNew = wingVz + pzd;
    double azTrue = (step > 1) ? (vzNew - wingVzPrev) / dtT : 0;
    // aceleracao vertical = derivada da vel. da asa + pzdd (usa analitico p/ pendulo)
    double wingAz = 0;
    {
      double a, b2, c;
      sc.at(t + dtT, a, b2, c);
      wingAz = (a - wingVz) / dtT;
    }
    azTrue = wingAz + pzdd;
    tr.h = hNew; tr.vz = vzNew;

    // curva coordenada: centripeta horizontal g*tan(bank), yaw rate = -g tan(bank)/V
    double aCent = G * tan(bank);
    tr.yawRate = -aCent / V;
    tr.yaw += tr.yawRate * dtT;
    double rollTotal = bank + swing;
    double rollPrev = tr.roll;
    tr.roll = rollTotal;
    tr.rollRate = (step > 0) ? (tr.roll - rollPrev) / dtT : 0;

    // forca especifica em eixos "lateral" (antes do yaw): centripeta em -y
    // + aceleracao lateral do pendulo (no plano lateral) + vertical + gravidade
    double fyE = -aCent + pydd;   // no referencial que gira com o yaw
    double fzE = azTrue + G;
    // para o corpo: R = Rz(yaw) Rx(roll) -> f_body = Rx(roll)^T * [0, fyE, fzE]
    double c = cos(tr.roll), s = sin(tr.roll);
    tr.fx = 0;
    tr.fy = c * fyE + s * fzE;
    tr.fz = -s * fyE + c * fzE;
    // taxas no corpo
    double p = tr.rollRate, q = tr.yawRate * s, rr = tr.yawRate * c;

    // ---- sensores a 50 Hz ----
    if (step % 20 == 0) {
      float ax = (float)(((tr.fx / G) + abx + aNoise * N01(rng)) * ascale * accelScale);
      float ay = (float)(((tr.fy / G) + aby + aNoise * N01(rng)) * ascale * accelScale);
      float az = (float)(((tr.fz / G) + abz + aNoise * N01(rng)) * ascale * accelScale);
      float gx = (float)(p + gbx + gNoise * N01(rng));
      float gy = (float)(q + gby + gNoise * N01(rng));
      float gz = (float)(rr + gbz + gNoise * N01(rng));
      float aV = ahrs.update(ax, ay, az, gx, gy, gz, 0.02f);
      kf.predict(useAccel ? aV : 0.0f, 0.02f);
      if (step % 40 == 0) kf.update((float)(tr.h + baroNoise * N01(rng)));

      double err = kf.climb() - tr.vz;
      if (t >= errFrom) {
        sumSq += err * err; nErr++;
        if (fabs(err) > r.maxErr) r.maxErr = fabs(err);
      }
      if (stepMetric) {
        double frac = (kf.climb() - v0) / (v1 - v0);
        if (r.t50 < 0 && frac >= 0.5) r.t50 = t - 20.0;
        if (r.t90 < 0 && frac >= 0.9) r.t90 = t - 20.0;
      }
      uint16_t f = audio.tick(kf.climb(), (uint32_t)(t * 1000));
      if (t > 5 && tr.vz < falseBeepIfTrueBelow && f > 0 && kf.climb() > 0) r.falseBeepTicks++;
      if (print && step % 500 == 0)
        printf("  t=%5.1f  verdade=%+5.2f  vario=%+5.2f  bias=%+6.3f\n", t, tr.vz, kf.climb(), kf.bias());
    }
  }
  r.rmsErr = nErr ? sqrt(sumSq / nErr) : 0;
  return r;
}

int main() {
  // 1) MS5611: exemplo do datasheet -> 20.07 C, 100009 Pa
  uint16_t C[8] = {0, 40127, 36924, 23317, 23282, 33464, 28312, 0};
  float p, tc;
  MS5611::compensate(C, 9085466, 8569150, p, tc);
  printf("MS5611 datasheet: T=%.2f C (esperado 20.07)  P=%.1f Pa (esperado ~100009)\n\n", tc, p);

  // 2) Degrau planeio -> termica
  {
    StepScenario a, b;
    Result ra = run(a, true, 40, 25, true, -99, false);
    Result rb = run(b, false, 40, 25, true, -99, false);
    printf("DEGRAU -1.0 -> +2.0 m/s (rampa de 0.5 s)\n");
    printf("  acel+baro : 50%% em %.2f s, 90%% em %.2f s, ruido rms em regime %.3f m/s\n", ra.t50, ra.t90, ra.rmsErr);
    printf("  so baro   : 50%% em %.2f s, 90%% em %.2f s, ruido rms em regime %.3f m/s\n\n", rb.t50, rb.t90, rb.rmsErr);
  }
  // 3) Curva subindo a +2.0 (asa a 10 m/s reais); varia bank e velocidade configurada
  printf("CURVA (entra t=20 s, sai t=80 s), subida real +2.0 m/s, asa a 10 m/s\n");
  for (double bank : {30.0, 45.0}) {
    for (float vcfg : {0.0f, 9.0f, 8.0f, 11.0f}) {
      gBankDeg = bank; gAssumedV = vcfg;
      TurnScenario a;
      Result ra = run(a, true, 100, 15, false, -99, false);
      printf("  bank %2.0f  V_config=%4.1f %-16s erro max %.2f m/s, rms %.3f m/s\n", bank, vcfg,
             vcfg == 0 ? "(sem compensacao)" : "", ra.maxErr, ra.rmsErr);
    }
  }
  gBankDeg = 30.0; gAssumedV = 9.0f;
  printf("\n");
  // 4) Balanco de pendulo descendo -1.0
  {
    SwingScenario a, b;
    Result ra = run(a, true, 60, 15, false, -0.2, false);
    Result rb = run(b, false, 60, 15, false, -0.2, false);
    printf("BALANCO lateral 15 graus descendo -1.0 m/s\n");
    printf("  acel+baro : erro max %.2f, rms %.3f m/s, ticks de bipe falso %d\n", ra.maxErr, ra.rmsErr, ra.falseBeepTicks);
    printf("  so baro   : erro max %.2f, rms %.3f m/s, ticks de bipe falso %d\n", rb.maxErr, rb.rmsErr, rb.falseBeepTicks);
  }
  return 0;
}
