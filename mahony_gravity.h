// mahony_gravity.h - Estimador de atitude (Mahony) so para extrair a
// aceleracao vertical no referencial terrestre.
//
// Detalhe importante para voo livre: numa curva coordenada o piloto (pendulo)
// alinha com a gravidade APARENTE, e |a| = g/cos(bank). Se o filtro confiar no
// acelerometro durante a curva, a gravidade estimada "segue" a aparente e o
// vario mostra subida falsa (~1/cos(30deg)-1 = 15% de g!). Por isso o ganho de
// correcao pelo acelerometro e reduzido a zero quando |a| se afasta de 1 g:
// durante a curva a atitude vem so do giroscopio.
// Alem disso, a aceleracao centripeta (omega x v, com v = velocidade de trim)
// e descontada antes de usar o acelerometro como referencia de gravidade.
//
// Sem dependencias de Arduino.
#pragma once
#include <math.h>

class MahonyGravity {
public:
  // Velocidade de trim da asa (m/s) para a compensacao centrifuga. 0 = desliga.
  // Requer o eixo X do IMU apontando para FRENTE (se apontar para tras, use negativo).
  float airspeed = 9.0f;

  void begin(float kp = 0.5f, float ki = 0.01f) {
    kp_ = kp; ki_ = ki;
    q0_ = 1; q1_ = q2_ = q3_ = 0;
    ibx_ = iby_ = ibz_ = 0;
  }

  // Inicializa a atitude a partir de uma leitura de acelerometro parado (g).
  void alignToAccel(float ax, float ay, float az) {
    float n = sqrtf(ax * ax + ay * ay + az * az);
    if (n < 1e-3f) return;
    ax /= n; ay /= n; az /= n;
    // quaternion que leva o eixo z do corpo ao vetor medido
    float roll = atan2f(ay, az);
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));
    float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    q0_ = cr * cp; q1_ = sr * cp; q2_ = cr * sp; q3_ = -sr * sp;
  }

  // ax..az em g, gx..gz em rad/s, dt em s.
  // Retorna a aceleracao vertical (m/s^2, positivo = para cima), sem gravidade.
  float update(float ax, float ay, float az, float gx, float gy, float gz, float dt) {
    // gravidade estimada (unitaria) no referencial do corpo
    float vx = 2.0f * (q1_ * q3_ - q0_ * q2_);
    float vy = 2.0f * (q0_ * q1_ + q2_ * q3_);
    float vz = q0_ * q0_ - q1_ * q1_ - q2_ * q2_ + q3_ * q3_;

    // Compensacao centrifuga: numa curva a aceleracao centripeta e omega x v.
    // v = airspeed * (eixo X do corpo projetado no plano horizontal estimado).
    // Tirando isso do acelerometro, sobra ~so a gravidade, mesmo em curva.
    float cx = ax, cy = ay, cz = az;
    if (airspeed != 0.0f) {
      float fx = 1.0f - vx * vx, fy = -vx * vy, fz = -vx * vz;   // X - (X.u)u
      float fn = sqrtf(fx * fx + fy * fy + fz * fz);
      if (fn > 0.3f) {
        float k = airspeed / (fn * 9.80665f);                       // -> unidades de g
        fx *= k; fy *= k; fz *= k;
        cx -= gy * fz - gz * fy;
        cy -= gz * fx - gx * fz;
        cz -= gx * fy - gy * fx;
      }
    }

    float an = sqrtf(cx * cx + cy * cy + cz * cz);
    // ganho adaptativo: 1 em 1 g, cai linearmente a 0 com desvio de 0.1 g
    float w = 1.0f - fabsf(an - 1.0f) / 0.1f;
    if (w < 0) w = 0;

    if (w > 0 && an > 1e-3f) {
      float nx = cx / an, ny = cy / an, nz = cz / an;
      // erro = medido x estimado
      float ex = ny * vz - nz * vy;
      float ey = nz * vx - nx * vz;
      float ez = nx * vy - ny * vx;
      ibx_ += ki_ * w * ex * dt;
      iby_ += ki_ * w * ey * dt;
      ibz_ += ki_ * w * ez * dt;
      gx += kp_ * w * ex + ibx_;
      gy += kp_ * w * ey + iby_;
      gz += kp_ * w * ez + ibz_;
    } else {
      gx += ibx_; gy += iby_; gz += ibz_;
    }

    // integra quaternion
    float hdt = 0.5f * dt;
    float qa = q0_, qb = q1_, qc = q2_;
    q0_ += (-qb * gx - qc * gy - q3_ * gz) * hdt;
    q1_ += (qa * gx + qc * gz - q3_ * gy) * hdt;
    q2_ += (qa * gy - qb * gz + q3_ * gx) * hdt;
    q3_ += (qa * gz + qb * gy - qc * gx) * hdt;
    float qn = 1.0f / sqrtf(q0_ * q0_ + q1_ * q1_ + q2_ * q2_ + q3_ * q3_);
    q0_ *= qn; q1_ *= qn; q2_ *= qn; q3_ *= qn;

    // recalcula a vertical com a atitude nova e projeta a aceleracao
    vx = 2.0f * (q1_ * q3_ - q0_ * q2_);
    vy = 2.0f * (q0_ * q1_ + q2_ * q3_);
    vz = q0_ * q0_ - q1_ * q1_ - q2_ * q2_ + q3_ * q3_;
    float aVert_g = ax * vx + ay * vy + az * vz - 1.0f;
    return aVert_g * 9.80665f;
  }

private:
  float kp_, ki_;
  float q0_, q1_, q2_, q3_;
  float ibx_, iby_, ibz_;
};
