# ESP32 Music Player

Uma ESP32 toca ritmos em 3 buzzers passivos. Uma página no computador mostra as músicas e tem os botões **Começar** e **Pausar**. Não usa Wi-Fi, internet nem rede: a página fala com a placa pelo cabo USB.

## Estrutura
```text
frontend/index.html   interface (abre no PC, não vai para a placa)
frontend/style.css    estilos da interface
musicas/*.txt         uma música por arquivo (gravadas na placa com uploadfs)
src/main.cpp          firmware: lê as músicas, toca os buzzers e recebe comandos pelo USB
platformio.ini
```

## Componentes e ligações
- ESP32 DevKit + cabo USB de dados
- 3 buzzers passivos de 2 pinos

| Buzzer | + (sinal) | − |
|---|---|---|
| 1 | GPIO 18 | GND |
| 2 | GPIO 19 | GND |
| 3 | GPIO 21 | GND |

Os três tocam o mesmo padrão. Se algum buzzer consumir mais de ~20 mA, use um transistor NPN + resistor de ~1 kΩ naquele canal. Para mudar a altura do som, altere `TONE_HZ` em `src/main.cpp`.

## Como usar
1. Envie o firmware: `pio run -t upload`.
2. Envie as músicas: `pio run -t uploadfs` (no VS Code: PlatformIO → esp32dev → Platform → **Upload Filesystem Image**).
3. **Feche o Serial Monitor**, se estiver aberto (só um programa pode usar a porta USB por vez).
4. Abra `frontend/index.html` no **Chrome ou Edge** do computador (dois cliques no arquivo).
5. Clique em **Conectar ESP32** e escolha a porta da placa (ex.: `COM3`).
6. Clique em uma música e use **Começar** e **Pausar**. Pausar e Começar de novo continua de onde parou; escolher outra música começa ela do início.

Não funciona no celular nem no Firefox/Safari, que não têm Web Serial.

## Ranking (Firebase) e personagens
Ao dar Começar, dois personagens pixelados da música aparecem dançando, um de cada lado da página (Mario e Luigi, Harry e Hermione, Capitão e Marujo, Pianista e Elise), e a página mostra um ranking das músicas mais tocadas. A contagem sobe +1 cada vez que a música completa um ciclo inteiro do padrão (a placa avisa com `C:<n>`). Deixando uma música rodando em loop, ela soma a cada volta. Pausar e retomar não conta, só terminar o ciclo.

Para ativar o ranking:
1. No [console do Firebase](https://console.firebase.google.com), crie um projeto e um **Realtime Database**.
2. Em **Regras**, permita leitura e escrita em `ranking` (público, sem login; qualquer pessoa com a URL consegue alterar):
   ```json
   { "rules": { "ranking": { ".read": true, ".write": true } } }
   ```
3. Copie a URL do banco para `FIREBASE_URL` no início do script de `frontend/index.html`.

Sem `FIREBASE_URL` o ranking fica desligado, mas o resto funciona. Músicas novas ganham um personagem genérico; para criar um próprio, adicione uma entrada em `DANCERS`.

## Volume
A página tem uma barra de **Volume** (0 a 100%), acima do ranking. Ela envia `W<n>` à placa, que ajusta o duty do PWM dos buzzers na hora, mesmo com a música tocando. No Serial Monitor: `W50` põe o volume em 50%.

## Comandos PlatformIO
```bash
pio run                # compilar
pio run -t upload      # enviar o firmware
pio run -t uploadfs    # enviar as músicas (pasta musicas/)
pio device monitor     # Serial Monitor (115200)
```

## Adicionar uma música
Crie um arquivo `.txt` em `musicas/`. O nome do arquivo vira o nome da música (ex.: `Tetris.txt` → "Tetris"). Dentro, as durações em ms separadas por vírgula: som, pausa, som, pausa...
```text
120,60,120,120,240,480
```
- Quantidade **par** (se for ímpar, o último valor é ignorado), até 512 valores, até 20 músicas.
- A lista aparece em ordem alfabética.

Depois rode `pio run -t uploadfs` (não precisa regravar o firmware). A página pega a lista da placa sozinha.

## Testar sem a página
No Serial Monitor, digite `L` (lista), `T0` (toca a música 0) ou `P` (pausa).

## Como funciona
- **Firmware** (C++, PlatformIO, Arduino): os 3 pinos ficam no mesmo canal PWM (LEDC, 2 kHz), que é o que faz o buzzer passivo soar. Ligar e desligar o som é só mudar o duty.
- **Sem `delay()`**: o tempo de cada passo é controlado com `millis()` em `updatePlayer()`, chamado no `loop()`. A leitura do Serial também não bloqueia.
- **Estados**: parado, tocando e pausado. Pausar guarda quanto do passo já passou, então Começar continua do mesmo ponto. Ao terminar o padrão, a música recomeça em loop.
- **Músicas**: lidas do LittleFS. A lista é montada na inicialização (ordem alfabética) e o conteúdo só é lido quando a música toca.
- **Página**: HTML, CSS e JavaScript puros (`index.html` + `style.css`), sem bibliotecas, frameworks ou CDNs. A única rede usada é o `fetch` ao Firebase, para o ranking.

## Protocolo serial (115200 baud, uma linha por mensagem)

| Direção | Mensagem | Significado |
|---|---|---|
| PC → ESP32 | `L` | Pede a lista de músicas |
| PC → ESP32 | `T<n>` | Toca a música `n` (se estava pausada, continua; outra música começa do início) |
| PC → ESP32 | `P` | Pausa e guarda a posição |
| PC → ESP32 | `W<n>` | Volume de 0 a 100 |
| ESP32 → PC | `M<n>:<nome>` | Uma música da lista |
| ESP32 → PC | `S:tocando:<n>`, `S:pausado:<n>`, `S:parado` | Estado atual, enviado após cada comando |
| ESP32 → PC | `C:<n>` | A música `n` completou um ciclo (a página soma +1 no ranking) |

Linhas desconhecidas são ignoradas dos dois lados. Se mudar o protocolo, altere o firmware e a página juntos.

## Regras do projeto
- Prioridade: simplicidade. Não acrescentar recursos nem dependências que não foram pedidos.
- Firmware sem Wi-Fi e sem servidor; página sem bibliotecas externas.
- Sem comentários no código (`main.cpp` e `index.html`).
- Não trocar o hardware sem autorização.
- Não afirmar que o hardware foi testado sem acesso físico à placa.

## Testes manuais
- [ ] `pio run` compila e `pio run -t upload` envia o firmware.
- [ ] A página conecta e lista as 4 músicas.
- [ ] Começar toca a música selecionada nos 3 buzzers.
- [ ] Pausar silencia; Começar continua do mesmo ponto.
- [ ] Escolher outra música e Começar troca e começa do início.
- [ ] Desconectar o cabo mostra "Desconectado" sem travar a página.

## Limitações
Todas as músicas usam o mesmo tom (`TONE_HZ`): só variam a duração de sons e pausas. Os padrões são ritmos inspirados nas músicas, **não** reproduções fiéis. Compilar não prova que o buzzer funciona; é preciso testar na placa.
