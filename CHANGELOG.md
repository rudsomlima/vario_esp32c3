# Histórico de versões

## 0.4.0 — portal de configuração

- Portal Wi-Fi sob demanda: segurar o botão ~4 s ao ligar. Captive portal, rede WPA2 `Vario-C3-XXXX`
- Edita som, filtros, auto-desligar, Bluetooth e senha, com limites de valor
- "Testar o som": toca a subida simulada com os valores da tela, sem salvar
- Atualização de firmware (OTA) e restaurar padrões
- Proteções de energia: vario parado e bipe a cada 5 s com o portal aberto; desliga após 10 min sem acesso

## 0.3.0 — Bluetooth LE

- `$LK8EX1` pelo Nordic UART Service (NimBLE-Arduino 2.x) para XCTrack e XCSoar
- BLE desligado por padrão; 2 cliques liga/desliga; desliga sozinho sem conexão
- Configurações persistentes na NVS (`settings.h`)
- Botão com contagem de cliques

## 0.2.0 — baixo consumo

- Light sleep acordado pelo INT do MPU-6050 a 50 Hz
- PWM do piezo no RC_FAST, que continua tocando no light sleep
- Deep sleep ao desligar, acordado pelo botão; auto-desligar
- Volume salvo entre voos

## 0.1.0 — primeira versão

- MS5611 não bloqueante com compensação de 2ª ordem
- MPU-6050 a 50 Hz com calibração no boot
- Mahony com ganho adaptativo e compensação centrífuga
- Kalman de 3 estados (altitude, vario, bias)
- Áudio com histerese e piezo em ponte
- Simulação de voo no PC
