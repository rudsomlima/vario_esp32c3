# Montagem do hardware

## Lista de material

| Qtd | Item | Observação |
|---|---|---|
| 1 | **ESP32-C3 SuperMini** | Placa minúscula com USB-C nativo |
| 1 | **GY-63** (MS5611) | Sensor de pressão. Compre de vendedor confiável: há muitas falsificações |
| 1 | **GY-521** (MPU-6050) | Acelerômetro + giroscópio |
| 1 | **Piezo passivo** 20–30 mm | Transdutor **sem** oscilador interno. Teste: ligado direto em 3 V, ele **não** apita (só dá um clique) |
| 1 | **TP4056 com proteção** | O módulo com 6 pinos (OUT+/OUT−) e chip DW01. Evite o de 4 pinos sem proteção |
| 1 | **LiPo 1S** 300–500 mAh | Com placa de proteção, se possível |
| 1 | **Diodo Schottky** 1N5817 ou SS14 | **Proteção da bateria** (veja [Alimentação e segurança](#alimentação-e-segurança-da-bateria)) |
| 1 | Botão táctil | Do tipo "push", normalmente aberto |
| 1 | Resistor 100 kΩ | Pull-up do botão |
| 2 | Resistor 1 MΩ | Divisor para ler a bateria |
| 1 | Capacitor 100 nF | Filtro da leitura da bateria |
| — | Espuma **escura** de célula aberta | Cobre o MS5611 (luz e vento) |
| — | Caixa | Impressa em 3D ou adaptada |
| opc. | Chave liga/desliga deslizante | Desconecta a bateria para guardar ou gravar |

**Custo aproximado:** US$ 10–15 em peças.

## Ligações

```
ESP32-C3 SuperMini           Ligar em
------------------           ------------------------------------------------
3V3                    ->    VCC do GY-521  e  VCC do GY-63
GND                    ->    GND de todos os módulos
GPIO6  (SDA)           ->    SDA do GY-521  e  SDA do GY-63
GPIO7  (SCL)           ->    SCL do GY-521  e  SCL do GY-63
GPIO5                  <-    INT do GY-521            (obrigatório: acorda a CPU)
GPIO3                  <-    botão -> GND
                             + resistor 100 kΩ entre GPIO3 e 3V3
GPIO4                  <-    divisor: BAT+ --[1M]-- GPIO4 --[1M]-- GND
                             + capacitor 100 nF entre GPIO4 e GND
GPIO0  e  GPIO1        ->    os dois terminais do piezo (sem polaridade)
5V                     <-    catodo do diodo Schottky  (anodo no OUT+ do TP4056)
GND                    <-    OUT− do TP4056

TP4056:  B+ / B−  ->  bateria       OUT+ / OUT−  ->  vario (via diodo)
```

```
 TP4056 OUT+ ──►|── 5V (SuperMini)         ►| = diodo Schottky
                ^                               faixa (catodo) virada para o 5V
            1N5817 / SS14
```

**Pinos que não podem ser usados:**
- GPIO2, GPIO8 e GPIO9: definem o modo de boot (o GPIO8 tem o LED azul)
- GPIO18 e GPIO19: são o USB

**Por que esses pinos:**
- **O botão precisa ficar entre GPIO0 e GPIO5.** No ESP32-C3, só eles acordam o chip do deep sleep (desligado).
- A leitura de bateria usa o GPIO4, que é entrada analógica do ADC1.

## Alimentação e segurança da bateria

> ⚠️ **Importante.** O USB da SuperMini também está ligado no pino 5V. Sem o diodo, conectar o USB da placa com a bateria ligada joga os **5 V do USB direto na saída do TP4056**, ou seja, **na bateria**. Isso pode sobrecarregar a LiPo. O diodo Schottky entre o OUT+ do TP4056 e o pino 5V bloqueia esse caminho.

- **Com o diodo,** você pode conectar o USB da SuperMini para gravar ou depurar com a bateria ligada. A placa passa a ser alimentada pelo USB, e a bateria fica isolada.
- **O custo do diodo** é uma queda de ~0,2–0,3 V. O vario funciona até a bateria chegar a ~3,4 V, o que ainda corresponde a ~10% de carga numa LiPo. O alerta de 1 bipe de bateria aparece antes disso, abaixo de 3,7 V.
- **Alternativa ou complemento:** uma **chave liga/desliga** entre o TP4056 e o vario. Desligue a chave antes de conectar o USB da SuperMini.
- **Para carregar,** use sempre o USB do **TP4056**, nunca o da SuperMini.
- **A leitura de bateria** (GPIO4) é feita direto no BAT+, antes do diodo, então mostra a tensão real da bateria.

## Modificações nos módulos

1. **SuperMini: dessolde o LED vermelho de energia.** Ele fica aceso o tempo todo e gasta ~1–2 mA, uma boa parte do consumo total. O LED azul (GPIO8) fica desligado e não precisa sair.
2. **GY-521: dessolde o LED de energia.** O motivo é o mesmo.
3. **GY-63:** não precisa mudar nada. O módulo já vem com o protocolo I²C selecionado e o endereço 0x77.

Os módulos GY-521 e GY-63 têm reguladores próprios e resistores de pull-up no I²C. Alimentar o VCC deles com 3,3 V funciona normalmente.

## Orientação do GY-521 (importante)

O firmware corrige o erro que a força centrífuga causa nas curvas, e para isso precisa saber onde é a frente.

- A **seta X impressa no GY-521 deve apontar para frente**, na direção de voo, quando o vario estiver instalado.
- A placa deve ficar **mais ou menos na horizontal**. Inclinações de 20–30° são toleradas.
- Se for mais prático montar com o X para trás, coloque a **Velocidade em térmica negativa** no portal (ex.: −9).

## Montagem na caixa

- **MS5611:** cubra o sensor com um pedaço de **espuma escura**. Ele é sensível a luz (o gel do sensor reage ao sol) e a jatos de ar. A caixa precisa de um **furo de ventilação** pequeno, protegido do vento direto.
- **Piezo:** o som fica mais forte com o disco colado pela borda numa parede da caixa, na frente de um furo de ~3–5 mm. A cavidade atrás funciona como caixa de ressonância.
- **Botão:** acessível com luva, mas protegido de toques acidentais, por exemplo levemente rebaixado.
- **Fios curtos** no I²C (SDA/SCL).
- **Fixação dos sensores:** o GY-521 e o MS5611 precisam estar firmes. Uma placa solta vibrando vira ruído no acelerômetro.
- **Bateria:** isole bem e não deixe nada pressionando a LiPo.

## Primeiro teste de bancada

1. Grave o firmware (veja [firmware.md](firmware.md)).
2. Grave uma vez com `DEBUG_PLOT 1` e abra o Serial Plotter. Devem aparecer `alt`, `climb`, `bias` e `aVert`.
3. Se ao ligar tocarem **2 bipes graves**, o MS5611 não respondeu. Com **3 bipes graves**, foi o MPU-6050. Confira SDA, SCL, 3V3 e GND.
4. Com **4 bipes graves curtos**, o INT do GY-521 não chegou no GPIO5. O vario funciona, mas sem economia de bateria.

## Medindo o consumo

1. Multímetro na escala de **mA**, **em série com a bateria** (entre o B+ da LiPo e o B+ do TP4056), com todos os USB desconectados.
2. Meça:
   - **ligado, parado e em silêncio:** esperado ~6–7 mA
   - **apitando** (levante e abaixe o vario): um pouco mais
   - **com Bluetooth ativo:** esperado ~20–40 mA (estimativa)
   - **desligado,** na escala de µA: esperado ~50–80 µA
3. O multímetro faz a média dos picos do ciclo de 50 Hz, então a leitura média já serve.

Os valores esperados são estimativas a partir de datasheets. Se você medir, abra uma issue com os números: ajuda a validar o projeto.

## Alternativas de hardware

| Troca | Efeito |
|---|---|
| MPU-6050 → **LSM6DSO** ou **BMI270** | O IMU cai de ~3,9 mA para ~0,5 mA, e o total para ~3 mA (~130 h). Precisa de um driver novo para o IMU, no lugar de `imu_mpu6050.h` |
| ESP32-C3 → **nRF52840** (ex.: Seeed XIAO nRF52840 Sense) | Dorme muito melhor e mantém o BLE em baixo consumo. Os filtros (`kalman_vario.h`, `mahony_gravity.h`, `vario_audio.h`) são C++ puro e portáveis |
| GY-63 → MS5611 soldado direto numa placa própria | Menos volume e menos fiação |
