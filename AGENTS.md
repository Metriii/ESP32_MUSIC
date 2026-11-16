# AGENTS.md — ESP32 Music Player

## 1. Contexto

Uma ESP32 toca padrões rítmicos em buzzers passivos. Uma página web simples, aberta no computador, mostra as músicas e permite **Começar** e **Pausar**.

**Não há Wi-Fi, internet nem rede de nenhum tipo.** A página conversa com a ESP32 pelo cabo USB, usando a Web Serial API do navegador.

Prioridade: simplicidade. Não acrescentar recursos que não foram pedidos.

## 2. Tecnologias

- C++, PlatformIO, Arduino Framework, ESP32 DevKit.
- HTML, CSS e JavaScript puro em um único arquivo.
- Web Serial API (Chrome ou Edge no computador; não funciona em celular nem no Firefox/Safari).

Não usar Wi-Fi, servidor HTTP, bibliotecas externas, frameworks ou CDNs. O LittleFS (nativo) guarda só os arquivos de música.

## 3. Hardware

- ESP32 DevKit ligada ao PC por cabo USB (dados, não só carga).
- 3 buzzers passivos de 2 pinos: GPIO 18, 19 e 21 (+) e GND (−). Os três tocam o mesmo padrão.
- Buzzer passivo precisa de PWM: os 3 pinos ficam no mesmo canal LEDC (`ledcWriteTone`), com o tom em `TONE_HZ`.

Por enquanto todas as músicas usam um tom fixo; só a duração de sons e pausas varia. Não trocar o componente sem autorização.

## 4. Músicas

Uma música por arquivo em `musicas/<Nome>.txt` (o `data_dir` do PlatformIO), gravados na placa com `pio run -t uploadfs`. O nome do arquivo é o nome da música; o conteúdo são durações em ms separadas por vírgula: índice par = som, índice ímpar = silêncio, quantidade par (ímpar descarta o último), até 512 valores e 20 músicas. A lista é lida na inicialização e ordenada alfabeticamente; a música é carregada do arquivo ao tocar. O `main.cpp` não leva comentários `//`.

| Nº | Música |
|---|---|
| 0 | Fur Elise |
| 1 | Harry Potter |
| 2 | Piratas do Caribe |
| 3 | Super Mario |

Os padrões são ritmos inspirados nas músicas, não reproduções fiéis. Ao terminar, a música recomeça em loop.

## 5. Protocolo Serial (115200 baud, uma linha por mensagem)

| Direção | Mensagem | Significado |
|---|---|---|
| PC → ESP32 | `L` | Pede a lista de músicas |
| PC → ESP32 | `T<n>` | Toca a música `n`; se ela estiver pausada, continua de onde parou; outra música começa do início |
| PC → ESP32 | `P` | Pausa (desliga os buzzers e guarda a posição) |
| ESP32 → PC | `M<n>:<nome>` | Uma música da lista |
| ESP32 → PC | `S:tocando:<n>`, `S:pausado:<n>`, `S:parado` | Estado atual, enviado após cada comando |

A ESP32 envia a lista e o estado ao ligar e após `L`. Linhas desconhecidas são ignoradas dos dois lados.

## 6. Firmware

- Reprodução não bloqueante: `millis()` em `updatePlayer()`, sem `delay()`.
- Leitura do Serial sem bloquear.
- Funções separadas: tocar, pausar, atualizar o player, ler comandos.

## 7. Interface (`frontend/index.html`)

- Botão **Conectar ESP32**, lista de músicas (clique seleciona), **Começar**, **Pausar** e o texto de estado.
- Tema escuro, fonte do sistema, sem recursos externos.
- Nomes inseridos com `textContent`.
- Mostrar mensagem clara se o navegador não tiver Web Serial ou se a porta estiver ocupada.

## 8. Estrutura

```text
ESP32-Music-Player/
├── AGENTS.md
├── CLAUDE.md
├── README.md
├── platformio.ini
├── musicas/        (um .txt por música, vai para o LittleFS com uploadfs)
├── frontend/
│   └── index.html   (aberto no PC, não vai para a placa)
└── src/
    └── main.cpp     (lê as músicas, buzzers, comandos Serial)
```

## 9. Regras de desenvolvimento

1. Inspecionar os arquivos antes de alterar.
2. Não adicionar dependências.
3. Não modificar o hardware sem autorização.
4. Mudou o protocolo, mude o firmware e a página juntos.
5. Não afirmar que o hardware foi testado sem acesso físico; compilar não prova que o buzzer funciona.

## 10. Testes manuais

- [ ] `pio run` compila e `pio run -t upload` envia.
- [ ] A página conecta e lista as 4 músicas.
- [ ] Começar toca a música selecionada nos 3 buzzers.
- [ ] Pausar silencia; Começar continua do mesmo ponto.
- [ ] Escolher outra música e Começar troca e começa do início.
- [ ] Desconectar o cabo mostra "Desconectado" sem travar a página.
