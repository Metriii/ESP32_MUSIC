# ESP32 Music Player

Uma ESP32 toca ritmos em 3 buzzers passivos. Uma página no computador mostra as músicas e tem os botões **Começar** e **Pausar**. Não usa Wi-Fi, internet nem rede: a página fala com a placa pelo cabo USB.

## Estrutura
```text
frontend/index.html   interface (abre no PC, não vai para a placa)
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

## Limitações
Todas as músicas usam o mesmo tom (`TONE_HZ`): só variam a duração de sons e pausas. Os padrões são ritmos inspirados nas músicas, **não** reproduções fiéis. Compilar não prova que o buzzer funciona; é preciso testar na placa.
