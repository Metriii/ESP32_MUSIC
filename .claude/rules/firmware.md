# Regras do firmware

- Sem Wi-Fi ou servidor: a placa só conversa pelo Serial (USB, 115200). LittleFS guarda só as músicas.
- Nunca usar `delay()` na reprodução: usar `millis()` em `updatePlayer()` chamado no `loop()`; ler o Serial sem bloquear.
- 3 buzzers passivos nos GPIO 18, 19 e 21, no mesmo canal LEDC (`ledcWriteTone`, tom em `TONE_HZ`); `digitalWrite` só faz "tic". Não trocar o componente sem autorização.
- Músicas em `musicas/<Nome>.txt` (LittleFS): durações em ms separadas por vírgula, par = som, ímpar = silêncio. Sem comentários `//` no `main.cpp`.
- Protocolo: recebe `L`, `T<n>`, `P`; envia `M<n>:<nome>` e `S:tocando:<n>` / `S:pausado:<n>` / `S:parado`. Mudou o protocolo, mude o `frontend/index.html` junto.
- Não afirmar que o hardware foi testado; compilar não prova o funcionamento do buzzer.
