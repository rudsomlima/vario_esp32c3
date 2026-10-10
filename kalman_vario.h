// kalman_vario.h - Filtro de Kalman 3 estados para variometro
//
// Estado x = [h, v, b]
//   h : altitude (m)
//   v : velocidade vertical (m/s)
//   b : bias do acelerometro vertical (m/s^2)
//
// Entrada de controle u = aceleracao vertical medida no referencial terrestre,
// ja sem a gravidade (m/s^2). Aceleracao "verdadeira" = u - b.
// Medicao: altitude barometrica h_baro (m).
//
// Por que assim: o acelerometro responde instantaneamente (sem lag) mas deriva;
// o barometro nao deriva mas e ruidoso. O filtro usa o acelerometro para prever
// e o barometro para corrigir, estimando o bias online.
//
// Sem dependencias de Arduino: compila e testa no PC (ver test/sim_kalman.cpp).
#pragma once
#include <math.h>

class KalmanVario {
public:
  // sigmaAcc  : ruido do acelerometro vertical (m/s^2) - inclui erro de atitude
  // sigmaBias : passeio aleatorio do bias (m/s^2 por sqrt(s))
  // sigmaBaro : ruido da altitude barometrica (m)
  void begin(float h0, float sigmaAcc = 0.35f, float sigmaBias = 0.02f,
             float sigmaBaro = 0.25f) {
    h_ = h0; v_ = 0; b_ = 0;
    qa_ = sigmaAcc * sigmaAcc;
    qb_ = sigmaBias * sigmaBias;
    r_  = sigmaBaro * sigmaBaro;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) P_[i][j] = 0;
    P_[0][0] = 1.0f;   // incerteza inicial altitude (m^2)
    P_[1][1] = 1.0f;   // velocidade (m/s)^2
    P_[2][2] = 0.25f;  // bias (m/s^2)^2
  }

  // Passo de predicao: chamar a cada amostra do IMU.
  void predict(float accVert, float dt) {
    const float dt2 = dt * dt;
    const float a = accVert - b_;
    h_ += v_ * dt + 0.5f * a * dt2;
    v_ += a * dt;
    // b_ constante

    // F = [[1, dt, -dt2/2],
    //      [0,  1, -dt   ],
    //      [0,  0,  1    ]]
    float F[3][3] = {{1, dt, -0.5f * dt2}, {0, 1, -dt}, {0, 0, 1}};
    float FP[3][3], FPFt[3][3];
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) {
        float s = 0;
        for (int k = 0; k < 3; k++) s += F[i][k] * P_[k][j];
        FP[i][j] = s;
      }
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) {
        float s = 0;
        for (int k = 0; k < 3; k++) s += FP[i][k] * F[j][k];
        FPFt[i][j] = s;
      }
    // Q = G G^T qa + diag(0,0,qb*dt), G = [dt2/2, dt, 0]
    const float g0 = 0.5f * dt2, g1 = dt;
    FPFt[0][0] += g0 * g0 * qa_;
    FPFt[0][1] += g0 * g1 * qa_;
    FPFt[1][0] += g0 * g1 * qa_;
    FPFt[1][1] += g1 * g1 * qa_;
    FPFt[2][2] += qb_ * dt;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) P_[i][j] = FPFt[i][j];
  }

  // Passo de correcao: chamar a cada nova altitude barometrica.
  void update(float hBaro) {
    const float y = hBaro - h_;           // inovacao
    const float S = P_[0][0] + r_;        // H = [1 0 0]
    const float K0 = P_[0][0] / S, K1 = P_[1][0] / S, K2 = P_[2][0] / S;
    h_ += K0 * y;
    v_ += K1 * y;
    b_ += K2 * y;
    // P = (I - K H) P  -> subtrai K * (linha 0 de P)
    const float p0[3] = {P_[0][0], P_[0][1], P_[0][2]};
    for (int j = 0; j < 3; j++) {
      P_[0][j] -= K0 * p0[j];
      P_[1][j] -= K1 * p0[j];
      P_[2][j] -= K2 * p0[j];
    }
    // simetriza para robustez numerica em float
    P_[0][1] = P_[1][0] = 0.5f * (P_[0][1] + P_[1][0]);
    P_[0][2] = P_[2][0] = 0.5f * (P_[0][2] + P_[2][0]);
    P_[1][2] = P_[2][1] = 0.5f * (P_[1][2] + P_[2][1]);
  }

  float altitude() const { return h_; }
  float climb() const    { return v_; }
  float bias() const     { return b_; }

private:
  float h_, v_, b_;
  float P_[3][3];
  float qa_, qb_, r_;
};
