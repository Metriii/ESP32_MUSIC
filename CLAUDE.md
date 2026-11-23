# CLAUDE.md — ESP32 Music Player

Leia `AGENTS.md` antes de qualquer alteração: ele é a especificação do projeto.

## Resumo
ESP32 **sem Wi-Fi ou rede** (só a página usa o Firebase REST para o ranking, e mostra um personagem pixelado dançando por música), com 3 buzzers passivos (GPIO 18, 19, 21) que tocam padrões rítmicos (som/silêncio).
A página `frontend/index.html`, aberta no Chrome/Edge do PC, controla a placa pelo cabo USB (Web Serial):
lista de músicas, Começar e Pausar (continua de onde parou).

## Estrutura
- `platformio.ini` — env `esp32dev`, Arduino framework.
- `musicas/*.txt` — uma música por arquivo; vão ao LittleFS com `pio run -t uploadfs`.
- `src/main.cpp` — firmware: lê as músicas, toca os buzzers, recebe comandos Serial. Sem comentários `//`.
- `frontend/index.html` — interface (arquivo único, não vai para a placa).
- `.claude/rules/` — regras de firmware e frontend.

## Comandos
- `pio run` — compilar
- `pio run -t upload` — enviar o firmware à ESP32
- `pio run -t uploadfs` — enviar as músicas (`musicas/`)
- `pio device monitor` — Serial (115200); feche antes de conectar a página

## Regras-chave
- Sem `delay()` na reprodução; usar `millis()`.
- Firmware sem rede; página sem bibliotecas, frameworks ou CDNs (só `fetch` ao Firebase).
- Não afirmar teste de hardware sem acesso físico.
- Responder em português.
