# Vario C3

**Vario sonoro de baixo consumo para parapente**, feito com um ESP32-C3 SuperMini, um sensor de pressão MS5611 e um acelerômetro/giroscópio MPU-6050. Custa ~US$ 15 em peças.

O vario junta acelerômetro e barômetro num filtro de Kalman, para uma resposta quase sem atraso. Corrige o erro da força centrífuga ao centrar térmicas e dorme entre as amostras para durar dezenas de horas. Opcionalmente, manda os dados para o **XCTrack** por Bluetooth LE. Todas as configurações são ajustadas pelo celular num portal Wi-Fi, que só liga quando você pede.

> ⚠️ **Status: experimental.** O firmware foi validado em simulação e com testes no PC, mas **ainda não voou**. Use como auxílio, teste bem em solo e não confie a sua segurança a ele. Relatos de montagem, medições de consumo e voos são muito bem-vindos nas issues.

## Recursos

- **Resposta rápida:** fusão acelerômetro + barômetro por Kalman. Na simulação, chega a 50% de uma entrada de térmica em ~0,26 s, contra ~1 s de um vario só com barômetro, e com ¼ do ruído.
- **Sem bipe falso nas curvas:** a compensação centrífuga e o ganho adaptativo no filtro de atitude levam o erro numa curva de 30° de 0,52 para 0,04 m/s RMS na simulação.
- **Baixo consumo:** a CPU fica em light sleep entre as amostras e é acordada pelo sensor a 50 Hz. O som continua tocando com a CPU dormindo. Estimativa de **~6–7 mA**, ou ~55–65 h com uma LiPo de 400 mAh.
- **Bluetooth LE para o XCTrack e o XCSoar:** sentença `$LK8EX1` pelo Nordic UART Service. Vem desligado por padrão, liga com dois cliques e se desliga sozinho sem conexão.
- **Portal de configuração Wi-Fi:** abre segurando o botão no boot. Tem teste de som antes de salvar, atualização de firmware (OTA) e desliga sozinho.
- **Um botão só:** volume, Bluetooth, liga/desliga e portal. O volume e as configurações ficam salvos.
- **Piezo em ponte:** dobro da tensão no piezo (~6 dB a mais), sem amplificador.

## Documentação

| Documento | Para quem |
|---|---|
| 📖 [Manual de uso](docs/manual-de-uso.md) | Pilotos: botões, sons, XCTrack, portal, ajustes, problemas |
| 🔧 [Montagem](docs/montagem.md) | Lista de material, ligações, **segurança da bateria**, caixa |
| 💾 [Firmware](docs/firmware.md) | Compilar, gravar, OTA, chaves de compilação, tabela de configurações |
| 🧠 [Como funciona](docs/como-funciona.md) | Filtros (Mahony + Kalman), energia, $LK8EX1, simulação |
| 📝 [Histórico de versões](CHANGELOG.md) | O que mudou em cada versão |

## Início rápido

1. **Monte** conforme [docs/montagem.md](docs/montagem.md). Não esqueça o **diodo Schottky** de proteção da bateria e a seta **X do GY-521 apontando para frente**.
2. **Prepare a Arduino IDE:**
   - core **esp32 3.x** (Espressif)
   - biblioteca **NimBLE-Arduino 2.x**
   - placa **ESP32C3 Dev Module**
   - **USB CDC On Boot: Enabled**
   - **Partition Scheme: Minimal SPIFFS (1.9MB APP with OTA)**
3. **Clone** numa pasta chamada `vario_esp32c3`, abra `vario_esp32c3.ino` e grave. Use o modo de gravação: segure BOOT, aperte RESET, solte BOOT.
4. **Ligue:** segure o botão 1 s e deixe o vario parado 2 s. Toca um tom subindo seguido dos bipes de bateria.
5. **Teste em solo** antes de voar: veja o [checklist](docs/manual-de-uso.md#antes-do-primeiro-voo).

## Hardware

| Peça | Função |
|---|---|
| ESP32-C3 SuperMini | Microcontrolador, BLE e Wi-Fi |
| GY-63 (MS5611) | Pressão: resolução de ~10 cm |
| GY-521 (MPU-6050) | Acelerômetro + giroscópio |
| Piezo passivo 20–30 mm | Som |
| TP4056 com proteção + LiPo 1S | Bateria e carga |
| Diodo Schottky, botão, resistores | Proteção, controle, leitura da bateria |

## Testes no PC

Os filtros não dependem do Arduino e são testados no PC:

```
cd test
g++ -O2 -std=c++17 -Istubs -I.. sim_flight.cpp -o sim_flight && ./sim_flight
```

| Cenário (simulado) | Acel + baro | Só barômetro |
|---|---|---|
| Entrada em térmica, tempo até 50% | **0,26 s** | 1,04 s |
| Ruído em regime | **0,026 m/s** | 0,10 m/s |
| Curva de 30°, erro RMS | **0,044 m/s** | — |
| Balanço lateral de 15°, erro RMS | **0,08 m/s** | 0,31 m/s |

Mais testes ($LK8EX1, página do portal, sintaxe do sketch) estão em [docs/firmware.md](docs/firmware.md#testes-no-pc).

## Estrutura

```
vario_esp32c3.ino    laço principal, botão, sleep
ms5611.h             barômetro
imu_mpu6050.h        acelerômetro/giroscópio
mahony_gravity.h     atitude + compensação centrífuga   (C++ puro)
kalman_vario.h       Kalman altitude/vario/bias          (C++ puro)
vario_audio.h        som do vario                         (C++ puro)
piezo_idf.h          PWM do piezo em ponte
ble_lk8ex1.h         Bluetooth LE / $LK8EX1
settings.h           configurações na flash
portal.h             portal Wi-Fi
docs/                documentação
test/                simulação de voo e testes no PC
```

## Contribuindo

Issues e pull requests são bem-vindos. Os mais úteis agora:
- **medições reais de consumo**, com o método descrito em [montagem.md](docs/montagem.md#medindo-o-consumo)
- **relatos de voo:** ajustes que funcionaram, comportamento em térmica e turbulência
- teste com o **XCTrack** e outros apps
- driver para um IMU de baixo consumo (LSM6DSO, BMI270) e port para nRF52

## Créditos

Inspirado pelos projetos de vario do [har-in-air](https://github.com/har-in-air). Referências completas em [como-funciona.md](docs/como-funciona.md#referências-e-créditos).
