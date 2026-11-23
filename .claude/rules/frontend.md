# Regras do frontend

- Um único arquivo `frontend/index.html` (HTML, CSS e JS puro), aberto direto no Chrome/Edge do computador.
- Comunicação com a ESP32 só por Web Serial (cabo USB). A única rede permitida é o `fetch()` da página para a API REST do Firebase Realtime Database (ranking, regras públicas, sem login). Sem SDK, frameworks, CDNs ou outros recursos externos.
- Elementos: botão Conectar, lista de músicas, Começar, Pausar, estado atual, personagens pixelados (canvas), barra de volume e ranking.
- Tema escuro, fonte do sistema. Nomes de músicas inseridos via `textContent`.
- O firmware não usa rede: o ranking é contado pela página.
