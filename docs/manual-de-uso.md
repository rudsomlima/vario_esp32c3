# Manual de uso

Este manual é para quem vai **voar** com o vario. Para montar o aparelho, veja [montagem.md](montagem.md). Para compilar e gravar o firmware, veja [firmware.md](firmware.md).

> ⚠️ **Projeto experimental.** O firmware ainda não tem horas de voo. Ele é um auxílio sonoro e não substitui a sua leitura do ar, nem um instrumento certificado. Teste bem em solo antes do primeiro voo (veja [Antes do primeiro voo](#antes-do-primeiro-voo)).

## Sumário

- [Referência rápida](#referência-rápida)
- [Ligar](#ligar)
- [O que o som significa](#o-que-o-som-significa)
- [Volume](#volume)
- [Desligar](#desligar)
- [Bateria](#bateria)
- [Bluetooth e XCTrack](#bluetooth-e-xctrack)
- [Portal de configuração](#portal-de-configuração)
- [Onde e como fixar o vario](#onde-e-como-fixar-o-vario)
- [Antes do primeiro voo](#antes-do-primeiro-voo)
- [Ajustando o som e os filtros](#ajustando-o-som-e-os-filtros)
- [Sinais sonoros do aparelho](#sinais-sonoros-do-aparelho)
- [Problemas comuns](#problemas-comuns)
- [Perguntas frequentes](#perguntas-frequentes)

## Referência rápida

O vario tem **um botão só**.

| Ação | O que faz |
|---|---|
| Segurar ~1 s (desligado) | Liga |
| Ligar e **continuar segurando** até ~4 s | Abre o portal de configuração (Wi-Fi) |
| 1 clique | Muda o volume: mudo → baixo → médio → alto |
| 2 cliques | Liga/desliga o Bluetooth (XCTrack) |
| Segurar 2 s (ligado) | Desliga |

| Som | Significa |
|---|---|
| Bipes, cada vez mais agudos e rápidos | Subindo. Quanto mais rápido e agudo, mais forte a subida |
| Silêncio | Entre +0,1 m/s e −2,5 m/s (planeio normal) |
| Tom grave contínuo | Descendo mais que 2,5 m/s (alarme de afundamento) |
| Bipe curtinho e agudo a cada 5 s | Portal de configuração ativo: **o vario NÃO está funcionando** |

## Ligar

1. **Segure o botão por ~1 segundo** e solte.
2. **Deixe o vario parado por ~2 segundos**, apoiado ou na mão sem mexer. É quando ele calibra o giroscópio e o acelerômetro.
3. Você ouve um **tom subindo** (pronto) e, logo depois, **de 1 a 4 bipes** que indicam a carga da bateria.
4. Se o Bluetooth estiver ativado, toca mais um **"bi-BIP" subindo**.

O vario está pronto. O volume é o mesmo da última vez que você usou.

> 💡 Se você **mexer durante a calibração**, toca um **bipe grave** depois do tom de pronto. O vario funciona, mas o melhor é desligar e ligar de novo parado. Uma calibração ruim pode deixar o vario impreciso nas curvas.

> 💡 Se o vario não ligar com 1 s, segure um pouco mais. Um toque rápido não liga, de propósito: assim ele não liga sozinho dentro da mochila.

Ao conectar a bateria, ou depois de salvar no portal, ele liga direto, sem precisar do botão.

## O que o som significa

| Variação vertical | Som (padrão de fábrica) |
|---|---|
| Acima de **+0,1 m/s** | Bipes. Em +0,1 m/s são ~700 Hz, um bipe a cada ~0,6 s |
| +1 m/s | ~820 Hz, um bipe a cada ~0,4 s |
| +3 m/s | ~1060 Hz, cerca de 4,7 bipes por segundo |
| +5 m/s ou mais | ~1300 Hz ou mais agudo, bipes bem rápidos (até ~7,5 por segundo) |
| Entre +0,1 e −2,5 m/s | Silêncio |
| Abaixo de **−2,5 m/s** | Tom contínuo grave, que fica mais grave quanto mais você afunda |

Isso é a **subida do piloto**, não a da massa de ar. Dentro de uma térmica de 2 m/s, com a asa afundando 1 m/s, o vario mostra cerca de +1 m/s.

**Por que este vario reage rápido:** ele junta o barômetro com o acelerômetro. O barômetro sozinho é ruidoso, e os varios baratos compensam com filtros que atrasam a resposta 1 a 2 s. O acelerômetro sente a entrada na térmica na hora. Nos testes de simulação, a resposta chegou a 50% do valor real em ~0,26 s, contra ~1 s de um vario só com barômetro.

Os valores de início do bipe, alarme de afundamento e tons podem ser mudados no [portal](#portal-de-configuração).

## Volume

**Um clique** passa para o próximo volume. A confirmação é sonora:

| Volume | Confirmação |
|---|---|
| Mudo | 1 bipe grave |
| Baixo | 1 bipe |
| Médio | 2 bipes |
| Alto | 3 bipes |

O volume escolhido **fica salvo** para o próximo voo. O alto é bem mais forte que o médio, porque o piezo é ligado em ponte (o dobro da tensão).

> Os bipes de sistema (ligar, desligar, Bluetooth) tocam sempre no volume alto, mesmo no mudo.

## Desligar

**Segure o botão por 2 segundos.** Toca uma sequência de **três tons descendo**, e o vario desliga quando você solta.

O vario também **desliga sozinho** depois de **20 minutos sem atividade**. Contam como atividade:
- subir ou descer mais que 0,3 m/s
- girar o aparelho
- apertar o botão
- ter um celular conectado por Bluetooth

Esse tempo pode ser mudado no portal (0 = nunca desliga sozinho).

> ⚠️ Esperando na rampa por muito tempo, parado e conectado na asa? Em ar muito calmo o vario pode entender que está esquecido e desligar. Se isso acontece com você, aumente o auto-desligar no portal.

Desligado, o consumo é muito baixo (dezenas de µA). A bateria aguenta meses parada. Para guardar por muito tempo, desconecte a bateria ou use uma chave liga/desliga, se a sua montagem tiver.

## Bateria

Ao ligar, o vario dá **1 a 4 bipes** de bateria:

| Bipes | Tensão | Carga aproximada |
|---|---|---|
| 4 | > 4,0 V | cheia |
| 3 | 3,85 – 4,0 V | boa |
| 2 | 3,7 – 3,85 V | média — dá para voar |
| 1 | < 3,7 V | baixa — **carregue antes de voar** |
| nenhum | — | leitura de bateria não instalada |

**Autonomia estimada** com bateria de 400 mAh (são estimativas, ainda não medidas):

| Modo | Autonomia |
|---|---|
| Só som | ~55–65 horas |
| Com Bluetooth conectado | ~10–20 horas |

**Para carregar,** use o conector USB do **módulo carregador (TP4056)**. Normalmente o LED fica vermelho carregando e azul ou verde com a carga completa.

> ⚠️ **Não conecte o USB da placa ESP32 com a bateria ligada**, a menos que a sua montagem tenha o diodo de proteção descrito em [montagem.md](montagem.md#alimentação-e-segurança-da-bateria). Sem ele, os 5 V do USB podem chegar direto na bateria.

## Bluetooth e XCTrack

O vario pode mandar a pressão e a variação vertical para o **XCTrack**, o **XCSoar** e outros apps que leem a sentença `$LK8EX1` por Bluetooth LE. O app passa a usar o barômetro do vario, que é mais preciso que o do celular.

### Ativar

1. Com o vario ligado, **dois cliques** no botão.
2. Toca um **"bi-BIP" subindo**: o Bluetooth está ativo. A escolha fica salva para os próximos voos.
3. Para desativar, mais **dois cliques** ("BIP-bi" descendo).

### Conectar no XCTrack

1. Ative o Bluetooth do vario.
2. No XCTrack: **Menu → Preferências → Sensores**. Ative o sensor externo por **Bluetooth LE** e escolha **`Vario-C3`**.
3. Quando conectar, o XCTrack mostra a variação e a altitude vindas do vario.

### Economia automática

Com o Bluetooth ativo o vario gasta bem mais (veja [Bateria](#bateria)). Por isso:
- se **nenhum celular conectar em 5 minutos**, só o Bluetooth desliga ("BIP-bi" descendo) e o vario continua funcionando no modo econômico
- no próximo boot ele tenta conectar de novo, já que o Bluetooth continua ativado
- esse tempo pode ser mudado no portal (0 = nunca desliga)

## Portal de configuração

Pelo celular você ajusta som, filtros, auto-desligar, Bluetooth e senha, testa o som e atualiza o firmware. O portal usa Wi-Fi, que gasta muita bateria, então **só liga quando você pede** e **o vario não funciona enquanto ele está aberto**.

### Abrir

1. Com o vario desligado, **segure o botão e não solte**.
2. Em ~1 s ele liga. **Continue segurando.**
3. Em ~4 s tocam **três bipes agudos subindo**. Solte.
4. No celular, conecte na rede Wi-Fi **`Vario-C3-XXXX`**. A senha padrão é **`vario1234`**.
5. A página de configuração abre sozinha. Se não abrir, acesse **http://192.168.4.1** no navegador.

### Usar

- Mude os valores. Cada campo mostra a unidade e uma explicação curta.
- **Testar o som:** os botões **+0,5 / +2 / +5 / −3 m/s** tocam ~3 s do som do vario **com os valores da tela, antes de salvar**. Dá para afinar o som ali, no chão.
- **Salvar e voltar ao vario:** grava e reinicia já no modo de voo.
- **Restaurar padrões:** volta tudo ao padrão de fábrica, inclusive a senha (`vario1234`).
- **Atualizar firmware:** veja [firmware.md](firmware.md#atualizar-pelo-portal-ota).

### Sair

- Salvando: reinicia no modo de voo.
- Segurando o botão 2 s: desliga.
- Ficando 10 minutos sem acessar a página: desliga sozinho.

> ⚠️ Enquanto o portal estiver aberto, o vario dá um **bipe agudo curtinho a cada 5 segundos**. Se ouvir isso, **o vario não está medindo nada**. Nunca decole assim.

> 🔒 **Troque a senha padrão** no portal. Quem estiver na rede do vario pode mudar as configurações e o firmware.

## Onde e como fixar o vario

- **Orientação:** o vario usa a direção "para frente" para corrigir as curvas. Fixe com a **seta X do sensor (GY-521) apontando para frente**, na direção de voo, e com a placa **mais ou menos na horizontal**. Na selete ou no cockpit, fixe firme, sem balançar solto.
- **Longe do vento direto:** rajadas no furinho do sensor viram "subida" e "descida" falsas. Dentro do cockpit ou de um bolso fechado (mas que deixe o som sair) funciona bem.
- **Longe do sol direto:** o sensor de pressão é sensível a luz e a mudança brusca de temperatura. A espuma escura sobre ele já protege; evite deixar o vario exposto.
- **Perto do ouvido:** em vento forte o som se perde. No ombro ou no cockpit costuma ser bom.

## Antes do primeiro voo

Faça estes testes em solo. Leva 10 minutos e evita surpresas no ar.

1. **Parado em cima da mesa:** não deve apitar. Silêncio total por um minuto.
2. **Levante o vario rápido ~1 m e segure:** ele dá alguns bipes na hora e para. Isso confirma a reação rápida.
3. **Escada ou elevador:** subindo um andar devagar (~3 m em 10 s, cerca de +0,3 m/s), deve bipar devagar. Descendo, silêncio.
4. **Gire e incline o vario na mão** sem subir nem descer: deve ficar em silêncio ou quase. Se bipar muito, confira a [orientação](#onde-e-como-fixar-o-vario).
5. **Carro numa estrada com subida** (com alguém dirigindo): deve bipar subindo a serra e ficar em silêncio no plano.
6. **Bluetooth** (se usar): conecte no XCTrack em solo e veja os números mudarem ao levantar e abaixar o vario.
7. **Bateria:** confira os bipes de bateria e saia com carga cheia.

## Ajustando o som e os filtros

Tudo se ajusta no [portal](#portal-de-configuração). Mude **um item de cada vez** e voe para avaliar.

| Sintoma no voo | O que mudar no portal |
|---|---|
| Som "nervoso" demais em turbulência | Aumentar **Ruído do barômetro** (ex.: 0,25 → 0,4) |
| Demora a reagir na entrada da térmica | Aumentar **Confiança no acelerômetro** (ex.: 0,35 → 0,5) |
| Bipa como se subisse ao **centrar** a térmica, sem estar subindo | Diminuir **Velocidade em térmica** (ex.: 9 → 8). É melhor errar para menos |
| Indica afundamento falso ao **sair** da curva | Aumentar **Velocidade em térmica** aos poucos (ex.: 9 → 9,5) |
| Bipa no planeio com ar parado | Aumentar **Início do bipe** (ex.: 0,10 → 0,20) |
| Quero ouvir as "bolhas" fracas | Diminuir **Início do bipe** (ex.: 0,10 → 0,0) |
| Alarme de afundamento toca demais | Diminuir **Alarme de afundamento** (ex.: −2,5 → −3,5) |
| Som muito grave ou muito agudo | **Tom em 0 m/s** e **Subida do tom** |
| Bipes lentos ou rápidos demais | **Ciclo do bipe em 0 m/s** (menor = mais rápido) |

**Sobre a "Velocidade em térmica":** é a velocidade aproximada da sua asa centrando térmica, em m/s (km/h ÷ 3,6). O padrão é 9 m/s (~32 km/h), um valor típico de parapente em térmica. Asas mais rápidas podem usar um pouco mais. Na dúvida, **erre para menos**: errar para mais causa leituras falsas maiores. A explicação técnica está em [como-funciona.md](como-funciona.md#compensação-centrífuga).

## Sinais sonoros do aparelho

| Som | Quando | Significado |
|---|---|---|
| Tom subindo (3 notas) | Ao ligar | Pronto para voar |
| 1 a 4 bipes iguais | Logo após ligar | Carga da bateria |
| 1 bipe grave | Após ligar | Mexeu durante a calibração. Ligue de novo parado |
| 4 bipes graves curtos | Ao ligar | Defeito de fiação: o sinal de interrupção do sensor não chegou. Funciona, mas sem economia de bateria |
| 2 bipes graves longos, e desliga | Ao ligar | **Sensor de pressão (MS5611) não encontrado.** Verifique a fiação |
| 3 bipes graves longos, e desliga | Ao ligar | **Acelerômetro (MPU-6050) não encontrado.** Verifique a fiação |
| "bi-BIP" subindo | Ao ligar ou com 2 cliques | Bluetooth ligado |
| "BIP-bi" descendo | Com 2 cliques ou sozinho | Bluetooth desligado (por você, ou por ninguém ter conectado) |
| 1 bipe grave longo | Com 2 cliques | Falha ao ligar o Bluetooth |
| 3 tons descendo | Botão 2 s ou auto-desligar | Desligando |
| 3 bipes agudos subindo | Segurando o botão ao ligar | Entrando no portal (solte o botão) |
| Bipe agudo curtinho a cada 5 s | Portal aberto | **Vario parado.** Não voe |

## Problemas comuns

| Problema | Causa provável / solução |
|---|---|
| Não liga | Bateria descarregada (carregue pelo TP4056), ou toque curto demais (segure ~1 s) |
| Liga e desliga em seguida com bipes graves | Sensor não encontrado. Veja os [sinais sonoros](#sinais-sonoros-do-aparelho) e a fiação em [montagem.md](montagem.md) |
| Apita parado na mesa | Calibração com o aparelho se mexendo (religue parado), vento ou ventilador no sensor, ou sol direto |
| Bipa sozinho ao centrar a térmica | Ajuste a **Velocidade em térmica** e confira se a seta X aponta para frente |
| Som falhando, picotado ou mudo | Ajuste de firmware: veja `PIEZO_KEEP_IN_SLEEP` em [firmware.md](firmware.md#chaves-de-compilação) |
| Não aparece no XCTrack | Bluetooth desativado (2 cliques), ou ele já desligou sozinho por falta de conexão (religue o vario). Confira com o app **nRF Connect** se aparece `Vario-C3` |
| A rede do portal não aparece | Você soltou o botão antes dos 3 bipes agudos. Desligue e tente de novo segurando até ~4 s |
| Esqueci a senha do portal | Regrave o firmware com as configurações apagadas (veja [firmware.md](firmware.md#apagar-as-configurações)) |
| Desligou sozinho na rampa | Auto-desligar: aumente o tempo no portal |
| Bateria acaba rápido | Bluetooth ativo (consome bem mais), ou montagem sem o fio de interrupção (4 bipes graves no boot) |

## Perguntas frequentes

**Precisa de GPS?**
Não. O vario mede só a variação vertical. Para trilha, vento e navegação, use o XCTrack no celular com o Bluetooth do vario.

**Mostra a altitude?**
Não tem tela, mas manda a pressão para o XCTrack, que calcula a altitude. Ajuste o QNH no app.

**Funciona para asa delta / planador?**
O princípio é o mesmo. Ajuste a **Velocidade em térmica** para a sua aeronave. O projeto foi pensado e simulado para parapente.

**É à prova d'água?**
Não. Depende da sua caixa. O sensor de pressão precisa de contato com o ar, então não vede totalmente.

**Por que não usa o barômetro do celular?**
Pode usar, mas os barômetros de celular são mais ruidosos, e o app filtra mais, o que atrasa a resposta. O MS5611 com o acelerômetro dá uma resposta muito mais rápida.
