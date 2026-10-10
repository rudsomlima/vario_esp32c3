# Vario ESP32-C3 — Fase 4 (portal de configuração)

Vario sonoro com fusão acelerômetro + barômetro (Kalman), para parapente. A CPU dorme em light sleep entre as amostras e é acordada 50 vezes por segundo pelo INT do MPU-6050. Opcionalmente, envia `$LK8EX1` por Bluetooth LE para o XCTrack ou o XCSoar. Um portal Wi-Fi, que só liga quando você pede, permite ajustar tudo pelo celular.

**Hardware:** ESP32-C3 SuperMini + GY-521 (MPU-6050) + GY-63 (MS5611) + piezo passivo + botão + LiPo via TP4056.

## Material

| Item | Obs. |
|---|---|
| ESP32-C3 SuperMini | Dessoldar o LED vermelho de energia |
| GY-521 (MPU-6050) | Dessoldar o LED de energia |
| GY-63 (MS5611) | Cobrir com espuma **escura** (o sensor é sensível a luz e a vento) |
| Piezo **passivo** 20–30 mm | Sem oscilador interno. Teste: com 3 V contínuos ele não apita |
| Botão táctil + resistor de 100 kΩ | Pull-up externo no GPIO3 |
| TP4056 **com proteção** (DW01) | Carregador da LiPo |
| LiPo de 300–500 mAh | — |
| 2× 1 MΩ + 100 nF | Divisor para ler a bateria |

## Ligações

```
SuperMini        Ligar em
---------        -------------------------------------------
3V3          ->  VCC do GY-521 e do GY-63
GND          ->  GND de todos
GPIO6 (SDA)  ->  SDA do GY-521 e do GY-63
GPIO7 (SCL)  ->  SCL do GY-521 e do GY-63
GPIO5        <-  INT do GY-521   (OBRIGATÓRIO: é o que acorda a CPU)
GPIO3        <-  botão -> GND   (+ 100k do GPIO3 para 3V3)
GPIO4        <-  BAT+ --1M-- GPIO4 --1M-- GND   (+100nF do GPIO4 ao GND)
GPIO0, GPIO1 ->  os dois terminais do piezo
5V           <-  OUT+ do TP4056   (GND <- OUT-)
```

- Não use GPIO2, 8 e 9 (definem o modo de boot) nem GPIO18 e 19 (USB).
- **Montagem do IMU:** a seta **X do GY-521 aponta para a frente** (direção de voo), com a placa mais ou menos na horizontal. Isso é necessário para a compensação centrífuga (`airspeed`).

## Compilar

1. Arduino IDE com o core **esp32 da Espressif, versão 3.x** (Gerenciador de Placas).
2. Placa **ESP32C3 Dev Module**, com **USB CDC On Boot = Enabled**.
3. Instalar a biblioteca **NimBLE-Arduino 2.x** (h2zero) pelo Gerenciador de Bibliotecas.
4. **Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA)"**. Com Wi-Fi + BLE, o firmware não cabe na partição padrão (1,2 MB), e esse esquema mantém a atualização pelo portal (OTA).
5. Abrir `vario_esp32c3.ino` e gravar.

**Gravar com o vario ligado:** no light sleep a porta USB some do PC. Para gravar:
- desligue o vario (botão 2 s) e conecte o USB, ou
- segure o botão BOOT, aperte RESET e solte o BOOT.

| Chave no `.ino` | Padrão | O que faz |
|---|---|---|
| `LOW_POWER` | 1 | Light sleep entre as amostras. Com 0, fica igual à fase 1 |
| `PIEZO_KEEP_IN_SLEEP` | 1 | O PWM do som roda no oscilador RC_FAST e continua tocando com a CPU dormindo. **Se o som sair falhando, engasgando ou mudo, ponha 0**: a CPU fica acordada só enquanto apita |
| `DEBUG_PLOT` | 0 | Com 1, os dados vão para o Serial Plotter e o sleep **desliga**, porque o USB cai no light sleep |

## Uso

| Ação | Resultado |
|---|---|
| Segurar o botão ~1 s | Liga. Deixe **parado ~2 s** para calibrar. Toca um tom subindo e depois 1 a 4 bipes de bateria |
| 1 clique | Volume: mudo → baixo → médio → alto (fica salvo para o próximo voo) |
| 2 cliques | Liga/desliga o BLE (fica salvo). Liga = dois bipes subindo; desliga = dois descendo |
| Segurar 2 s | Desliga (três tons descendo) |
| Ligar e **continuar segurando** até ~4 s | **Portal de configuração** (três bipes agudos subindo: solte o botão) |
| 20 min sem subir/descer e sem girar | Desliga sozinho (`autoOffMin`) |
| BLE ligado e nenhum celular conectado por 5 min | Só o BLE desliga (dois bipes descendo) e o vario volta ao modo econômico. No próximo boot ele tenta de novo |
| 2 bipes graves no boot | MS5611 não encontrado (fiação ou endereço) |
| 3 bipes graves no boot | MPU-6050 não encontrado |
| 1 bipe grave depois de ligar | Mexeu durante a calibração. Funciona, mas desligue e ligue de novo parado |
| 4 bipes curtos graves no boot | O INT do MPU não chegou no GPIO5. O vario funciona, mas **sem dormir** (consumo da fase 1) |

**Som:**
- **Bipes** a partir de +0,1 m/s. Sobem de tom e ficam mais rápidos com a subida.
- **Tom contínuo grave** abaixo de −2,5 m/s (alarme de afundamento).
- Os limiares, tons e filtros são salvos na flash e editados pelo portal. Os padrões ficam em `settings.h`.

## Testes em solo (antes de voar)

1. Gravar com `DEBUG_PLOT 1` e abrir o **Serial Plotter** (115200). Aparecem `alt`, `climb`, `bias` e `aVert`. Depois dos testes, volte para `DEBUG_PLOT 0`.
2. **Parado na mesa:** `climb` deve ficar em ±0,05 m/s e `bias` deve se acomodar perto de 0.
3. **Levantar o vario ~1 m rápido e segurar:** `climb` dá um pico positivo imediato e volta a 0. A altitude sobe ~1 m e fica.
4. **Escada ou elevador:** subindo um andar (~3 m em 10 s), `climb` deve ficar em +0,3 m/s e o vario deve apitar.
5. **Girar o vario na mão** (inclinar, rodar) sem subir nem descer: `climb` deve ficar perto de 0. Se não ficar, confira a montagem e o `airspeed`.
6. **Carro numa rampa ou serra:** é o melhor teste antes do voo.
7. **BLE:** dois cliques, e no celular o app **nRF Connect** deve mostrar `Vario-C3` com o serviço `6E400001…`. Ao assinar as notificações do TX (`6E400003…`) chegam linhas `$LK8EX1,…`.

## XCTrack

1. Ligue o BLE no vario (dois cliques).
2. No XCTrack: **Menu → Preferências → Sensores** → ative o sensor externo por **Bluetooth LE** e escolha `Vario-C3`.
3. A sentença enviada é `$LK8EX1,pressão(Pa),99999,vario(cm/s),temp,bateria(V),*CS`, a 10 Hz (`bleHz`). O campo de altitude vai como 99999 (não disponível), então o app calcula a altitude pela pressão.

O vario do har-in-air usa o mesmo serviço (Nordic UART) e foi validado com o XCTrack. Este firmware ainda não foi testado com o app.

## Portal de configuração

1. **Ligue o vario segurando o botão e continue segurando** até ouvir três bipes agudos subindo (~4 s). Solte.
2. No celular, conecte na rede **`Vario-C3-XXXX`**, senha **`vario1234`**. A página costuma abrir sozinha. Se não abrir, acesse **http://192.168.4.1**.
3. Ajuste o que quiser. Os botões **Testar o som** tocam ~3 s do vario simulando +0,5 / +2 / +5 / −3 m/s **com os valores da tela, sem salvar**. Dá para afinar o som ali mesmo, no chão.
4. **Salvar e voltar ao vario**: grava e reinicia já no modo de voo.

| O que tem na página | |
|---|---|
| Som | Volume inicial, início do bipe, alarme de afundamento, tom em 0 m/s, subida do tom, ciclo do bipe |
| Filtros | Velocidade em térmica (compensação centrífuga), confiança no acelerômetro, ruído do barômetro |
| Energia | Auto-desligar |
| Bluetooth | Ligar BLE ao ligar, taxa do $LK8EX1, desligar sem conexão |
| Portal | Senha do Wi-Fi |
| Atualizar firmware | Envia o `.bin` (Arduino IDE: *Sketch → Exportar binário compilado*). Se falhar, o firmware anterior continua |
| Restaurar padrões | Volta tudo ao `settings.h`, inclusive a senha `vario1234` |

**Por que não fica ligado gastando bateria:**
- O portal **só liga sob demanda**, segurando o botão no boot. No uso normal o Wi-Fi nunca é iniciado.
- Enquanto o portal está ativo, **o vario não funciona** e toca um **bipe curto a cada 5 s**. Não dá para decolar sem perceber.
- **Desliga sozinho** depois de 10 min sem acesso à página. Também desliga com o botão segurado por 2 s.
- O Wi-Fi transmite com potência reduzida (8,5 dBm), porque o celular está ao lado.

## Ajuste fino (depois dos primeiros voos)

Tudo isto se ajusta pelo portal. Entre parênteses, o nome do campo em `settings.h`.

| Sintoma | O que mexer |
|---|---|
| Som "nervoso" demais em turbulência | Aumentar o ruído do barômetro (`kfBaro`, 0,25 → 0,4) ou diminuir a confiança no acelerômetro (`kfAcc`) |
| Demora a reagir na entrada da térmica | Aumentar a confiança no acelerômetro (`kfAcc`, 0,35 → 0,5) |
| Subida falsa ao centrar a térmica | Diminuir a velocidade em térmica (`airspeed`). É melhor errar para menos |
| Afundamento falso ao sair da curva | Aumentar a velocidade em térmica (`airspeed`) aos poucos |
| Apita no planeio com ar parado | Subir o início do bipe (`liftOn`) |

## Simulação no PC

`test/sim_flight.cpp` simula um voo com os mesmos filtros do firmware. Os sensores têm ruído e erros típicos de MPU-6050 barato (offsets, escala, bias residual do giroscópio).

```
cd test
g++ -O2 -std=c++17 -Istubs -I.. sim_flight.cpp -o sim_flight && ./sim_flight
```

Resultado atual:

| Cenário | Acel + baro | Só baro |
|---|---|---|
| Degrau −1 → +2 m/s, tempo até 50% / 90% | **0,26 s / 0,42 s** | 1,04 s / 1,84 s |
| Ruído em regime (RMS) | **0,026 m/s** | 0,10 m/s |
| Curva de 30°, erro RMS, sem compensação centrífuga | 0,52 m/s | — |
| Curva de 30°, erro RMS, com compensação (9 m/s) | **0,044 m/s** | — |
| Curva de 45°, erro RMS, com compensação (9 m/s) | **0,070 m/s** | — |
| Balanço lateral de 15°, erro RMS | **0,08 m/s** | 0,31 m/s |

A simulação é idealizada: não tem turbulência real, vibração nem erro de pressão estática. Ela valida a lógica, não substitui o voo.

`test/test_lk8ex1.cpp` confere o formato e o checksum da sentença BLE, inclusive a quebra em pedaços quando o MTU é pequeno:

```
g++ -std=c++17 -Istubs_esp -I.. test_lk8ex1.cpp -o test_lk8ex1 && ./test_lk8ex1
```

`test/render_portal.cpp` gera a página do portal em `test/portal_preview.html`, para ver no navegador do PC:

```
g++ -std=c++17 -Istubs_esp -I.. render_portal.cpp -o render_portal && ./render_portal
```

## Consumo

Estimativa com datasheets. **Não foi medido.**

| Item | Consumo médio |
|---|---|
| MPU-6050 (acelerômetro + giroscópio) | ~3,9 mA ← o maior consumo |
| MS5611 a 25 Hz | ~0,33 mA |
| ESP32-C3: ~1–1,5 ms acordado a cada 20 ms + light sleep | ~1,5–2,5 mA |
| Regulador, divisor, TP4056 | ~0,05 mA |
| **Total em silêncio** | **~6–7 mA** |
| Apitando, com `PIEZO_KEEP_IN_SLEEP 1` | +~0,3–0,5 mA |
| Apitando, com `PIEZO_KEEP_IN_SLEEP 0` | +~10 mA enquanto houver som |
| **Com o BLE ativo** (sem light sleep, rádio ligado) | **~20–40 mA** no total |
| Portal Wi-Fi (só em solo, no máximo 10 min sem uso) | ~80–100 mA |

- **Autonomia:** ~55–65 h com 400 mAh sem BLE. A fase 1 dava ~13 h.
- **Com BLE:** ~10–20 h com 400 mAh. É estimativa: o har-in-air **mediu ~85 mA** com BLE no vario dele, então meça o seu. Com o rádio ativo, este firmware não usa light sleep, porque o controlador BLE do core Arduino não aguenta o sleep manual. Por isso o BLE vem desligado e se desliga sozinho sem conexão.
- **Desligado** (deep sleep): ~50–80 µA, ou seja, meses de bateria parada. Para guardar por muito tempo, uma chave física na bateria resolve.

**Como medir:** multímetro na escala de mA **em série com a bateria**, com o USB desconectado. O multímetro tira a média dos picos de 50 Hz. Meça três situações:
- parado e em silêncio
- apitando (levante e abaixe o vario)
- desligado (escala de µA)

**O próximo salto:** trocar o MPU-6050 por um LSM6DSO ou BMI270 (~0,5 mA) baixa o total para ~3 mA, ou ~130 h. Só o driver do IMU muda.

## Problemas conhecidos / o que conferir no hardware

- **Som com `PIEZO_KEEP_IN_SLEEP 1`:** o LEDC rodando no RC_FAST durante o light sleep depende da versão do ESP-IDF dentro do core. Se der problema, use 0.
- Se o vario parecer "lento" ou o som picotar, teste com `LOW_POWER 0` para separar o problema do sleep do resto.
- A frequência do RC_FAST tem alguns % de imprecisão. O tom fica um pouco diferente de um aparelho para outro, o que não importa para o vario.

## Limitações

- Com o BLE ativo, não há light sleep.
- A calibração do acelerômetro corrige só a escala geral. Os offsets por eixo ficam com o estado de bias do Kalman.
- O portal não tem login além da senha do Wi-Fi. Quem estiver conectado na rede do vario pode mudar as configurações e o firmware. Troque a senha padrão.
- O código não foi testado em hardware real. Só passou pela checagem de sintaxe com stubs, pela simulação e pelos testes de PC (`test/`).
