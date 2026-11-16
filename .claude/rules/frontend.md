# Regras do frontend

- Um único arquivo `frontend/index.html` (HTML, CSS e JS puro), aberto direto no Chrome/Edge do computador.
- Comunicação com a ESP32 só por Web Serial (cabo USB). Sem rede, `fetch()`, frameworks, CDNs ou recursos externos.
- Elementos: botão Conectar, lista de músicas, Começar, Pausar e o estado atual. Nada além disso sem pedido.
- Tema escuro, fonte do sistema. Nomes de músicas inseridos via `textContent`.
