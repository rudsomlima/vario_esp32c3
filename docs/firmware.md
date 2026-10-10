# Firmware: compilar, gravar e configurar

## Requisitos

| Item | Versão / ajuste |
|---|---|
| Arduino IDE | 2.x (a 1.8 também funciona) |
| Core da placa | **esp32** da Espressif, **3.x** (Gerenciador de Placas). O driver do som usa APIs do ESP-IDF 5 |
| Biblioteca | **NimBLE-Arduino 2.x** (h2zero), pelo Gerenciador de Bibliotecas |
| Placa | **ESP32C3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| Partition Scheme | **Minimal SPIFFS (1.9MB APP with OTA)** |
| CPU Frequency | indiferente: o firmware força 80 MHz |

Com Wi-Fi (portal) + BLE + servidor web, o firmware **não cabe na partição padrão** de 1,2 MB. O esquema "Minimal SPIFFS" tem 1,9 MB por aplicação e mantém duas partições, o que é necessário para a atualização pelo portal (OTA).

## Baixar e compilar

A Arduino IDE exige que a pasta tenha o mesmo nome do `.ino`. Clone com o nome `vario_esp32c3`:

```
git clone https://github.com/<usuario>/<repositorio>.git vario_esp32c3
```

Abra `vario_esp32c3/vario_esp32c3.ino`, selecione a placa e as opções da tabela acima e compile.

## Gravar

O firmware passa quase todo o tempo em light sleep ou deep sleep, e **nesses modos a porta USB some do PC**: o USB do ESP32-C3 desliga junto com a CPU. Por isso, grave sempre no **modo de gravação**:

1. Conecte o USB da SuperMini.
2. Segure o botão **BOOT** da placa, aperte e solte o **RESET**, e solte o **BOOT**. A porta COM aparece e fica estável.
3. Grave pela Arduino IDE.
4. Depois de gravar, aperte **RESET** para rodar o firmware novo.

Com `DEBUG_PLOT 1` o sleep fica desligado e a porta permanece, então as gravações seguintes funcionam direto.

> ⚠️ Não conecte o USB da SuperMini com a bateria ligada se a montagem **não** tiver o diodo de proteção. Veja [montagem.md](montagem.md#alimentação-e-segurança-da-bateria).

## Atualizar pelo portal (OTA)

1. Na Arduino IDE: **Sketch → Exportar binário compilado**. Isso cria uma pasta `build/…` dentro do projeto.
2. Use o arquivo **`vario_esp32c3.ino.bin`**. **Não** use o `…merged.bin`.
3. Abra o [portal](manual-de-uso.md#portal-de-configuração) e toque em **Atualizar firmware**. Escolha o `.bin` e envie.
4. Se der certo, o vario reinicia com o firmware novo. Se falhar, o firmware anterior continua instalado.

As configurações salvas são mantidas na atualização.

## Apagar as configurações

Para voltar tudo ao padrão sem usar o portal (por exemplo, se esqueceu a senha do Wi-Fi):

- Arduino IDE: **Tools → Erase All Flash Before Sketch Upload → Enabled** e grave de novo pelo USB. Depois volte para *Disabled*.

## Chaves de compilação

Ficam no início de `vario_esp32c3.ino`:

| Chave | Padrão | O que faz |
|---|---|---|
| `LOW_POWER` | `1` | Light sleep entre as amostras. Com `0`, a CPU fica acordada (consumo bem maior). Útil para isolar problemas |
| `PIEZO_KEEP_IN_SLEEP` | `1` | O PWM do som usa o oscilador interno RC_FAST e continua tocando com a CPU dormindo. **Se o som sair falhando, picotado ou mudo, ponha `0`**: a CPU fica acordada só enquanto apita |
| `DEBUG_PLOT` | `0` | Com `1`, manda `alt`, `climb`, `bias` e `aVert` para o Serial Plotter a 10 Hz. **Desliga o sleep**, porque o USB cai no light sleep |

Outras constantes no mesmo arquivo:

| Constante | Padrão | O que é |
|---|---|---|
| `FW_VERSION` | `"0.4.0"` | Versão mostrada no portal |
| `BLE_NAME` | `"Vario-C3"` | Nome no Bluetooth |
| `PIN_*` | ver [montagem.md](montagem.md#ligações) | Pinos |
| `BARO_EVERY_TICKS` | `2` | Barômetro a 50/2 = 25 Hz |
| `TEMP_EVERY_CONV` | `25` | 1 leitura de temperatura do MS5611 a cada 25 conversões (1 s) |
| `AUTO_OFF_CLIMB` / `AUTO_OFF_GYRO` | 0,3 m/s / 0,5 rad/s | O que conta como "atividade" para o auto-desligar |
| `CLICK_GAP_MS` | `400` | Janela para o segundo clique |

## Configurações (salvas na flash)

Ficam em `settings.h` (struct `Settings`), são salvas na NVS e editadas pelo portal. Valores fora da faixa são limitados ao salvar.

| Campo | No portal | Padrão | Faixa | Unidade | Efeito |
|---|---|---|---|---|---|
| `volume` | Volume inicial | 3 (alto) | 0–3 | — | 0 mudo, 1 baixo, 2 médio, 3 alto. Também é salvo pelo botão |
| `liftOn` | Início do bipe | 0,10 | −0,5 a 1,0 | m/s | Subida mínima para bipar. Desliga com 0,05 m/s a menos (histerese) |
| `sinkOn` | Alarme de afundamento | −2,5 | −10 a −0,5 | m/s | Tom contínuo abaixo deste valor. Desliga com 0,2 m/s a mais |
| `fBase` | Tom em 0 m/s | 700 | 200–2000 | Hz | Frequência do bipe em 0 m/s |
| `fPerMs` | Subida do tom | 120 | 10–400 | Hz por m/s | Quanto o tom sobe por m/s (limite de 2200 Hz) |
| `periodZero` | Ciclo do bipe em 0 m/s | 600 | 150–1500 | ms | Ciclo = periodZero / (1 + 0,6·subida), mínimo de 130 ms |
| `airspeed` | Velocidade em térmica | 9,0 | −15 a 15 | m/s | Compensação centrífuga. 0 desliga. Negativo se o X do GY-521 apontar para trás |
| `kfAcc` | Confiança no acelerômetro | 0,35 | 0,05–3 | m/s² | Ruído de processo do Kalman. Maior = reage mais rápido, mais ruído |
| `kfBaro` | Ruído do barômetro | 0,25 | 0,05–3 | m | Ruído de medição do Kalman. Maior = som mais suave, mais lento |
| `autoOffMin` | Auto-desligar | 20 | 0–240 | min | Sem atividade. 0 = nunca |
| `bleEnabled` | Ligar BLE ao ligar o vario | não | — | — | Também alternado com 2 cliques |
| `bleHz` | Taxa do BLE | 10 | 1–10 | Hz | Sentenças `$LK8EX1` por segundo |
| `bleTimeoutMin` | BLE desliga sem conexão | 5 | 0–120 | min | 0 = nunca |
| `apPass` | Senha do Wi-Fi | `vario1234` | 8–32 caracteres | — | Senha WPA2 do portal |

O tom do alarme de afundamento (450 Hz + 40 Hz por m/s, mínimo de 200 Hz) e o ciclo mínimo do bipe ficam em `vario_audio.h`.

## Estrutura do código

| Arquivo | Função |
|---|---|
| `vario_esp32c3.ino` | Laço principal, botão, sleep, inicialização, auto-desligar |
| `ms5611.h` | Driver não bloqueante do MS5611, com compensação de 2ª ordem em double e CRC da PROM |
| `imu_mpu6050.h` | Driver do MPU-6050: 50 Hz, DLPF, INT em latch, calibração |
| `mahony_gravity.h` | Atitude (filtro Mahony) com ganho adaptativo e compensação centrífuga. Devolve a aceleração vertical |
| `kalman_vario.h` | Kalman de 3 estados: altitude, vario, bias do acelerômetro |
| `vario_audio.h` | Mapeia a subida em frequência e cadência do bipe |
| `piezo_idf.h` | PWM do piezo em ponte, pelo driver LEDC do ESP-IDF |
| `ble_lk8ex1.h` | `$LK8EX1` pelo Nordic UART Service (NimBLE) |
| `settings.h` | Configurações com padrões, limites e persistência na NVS |
| `portal.h` | Portal Wi-Fi: página, teste de som, OTA, captive portal |

`mahony_gravity.h`, `kalman_vario.h` e `vario_audio.h` **não dependem do Arduino**. Compilam no PC e são portáveis para outros microcontroladores.

## Testes no PC

Na pasta `test/`, com qualquer g++ que suporte C++17:

```
cd test

# Simulação de voo: degrau de térmica, curvas de 30/45°, balanço de pêndulo
g++ -O2 -std=c++17 -Istubs -I.. sim_flight.cpp -o sim_flight && ./sim_flight

# Formato e checksum do $LK8EX1, e quebra por MTU
g++ -std=c++17 -Istubs_esp -I.. test_lk8ex1.cpp -o test_lk8ex1 && ./test_lk8ex1

# Gera a página do portal em portal_preview.html
g++ -std=c++17 -Istubs_esp -I.. render_portal.cpp -o render_portal && ./render_portal

# Checagem de sintaxe do sketch contra stubs das APIs do ESP32
g++ -std=c++17 -fsyntax-only -Istubs_esp -I.. -x c++ ../vario_esp32c3.ino
```

Os `stubs*` imitam o mínimo das APIs do Arduino e do ESP-IDF, só para compilar no PC. **Não substituem** compilar com o core ESP32 de verdade.
