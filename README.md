# ESP32 Music Player

Uma ESP32 toca ritmos em 3 buzzers passivos. A própria placa hospeda a página: ela entra no seu Wi-Fi, e você abre `http://esp32-music.local` no navegador (PC ou celular) para ver as músicas e usar os botões **Começar** e **Pausar**.

## Estrutura
```text
data/web/index.html      interface (servida pela placa)
data/web/style.css       estilos da interface
data/musicas/*.txt       uma música por arquivo
src/main.cpp             firmware: Wi-Fi, servidor HTTP, API e player dos buzzers
src/secrets.example.h    modelo das credenciais do Wi-Fi
platformio.ini
```
Tudo que está em `data/` vai para a placa com `uploadfs`.

## Componentes e ligações
- ESP32 DevKit + cabo USB (para gravar o firmware e alimentar)
- 3 buzzers passivos de 2 pinos

| Buzzer | + (sinal) | − |
|---|---|---|
| 1 | GPIO 18 | GND |
| 2 | GPIO 19 | GND |
| 3 | GPIO 21 | GND |

Os três tocam o mesmo padrão. Se algum buzzer consumir mais de ~20 mA, use um transistor NPN + resistor de ~1 kΩ naquele canal. Para mudar a altura do som, altere `TONE_HZ` em `src/main.cpp`.

## Como usar
1. Copie `src/secrets.example.h` para `src/secrets.h` e preencha `WIFI_SSID` e `WIFI_PASSWORD` (o `secrets.h` não vai para o GitHub). A rede precisa ser de 2,4 GHz.
2. Envie o firmware: `pio run -t upload`.
3. Envie a página e as músicas: `pio run -t uploadfs` (no VS Code: PlatformIO → esp32dev → Platform → **Upload Filesystem Image**).
4. Abra o Serial Monitor (`pio device monitor`) e veja a linha `Wi-Fi conectado. Acesse http://esp32-music.local ou http://<IP>`.
5. No navegador, de qualquer aparelho na mesma rede, abra `http://esp32-music.local` (se o nome não funcionar, use o IP mostrado).
6. Clique em uma música e use **Começar** e **Pausar**. Pausar e Começar de novo continua de onde parou; escolher outra música começa ela do início.

Se a rede for institucional (faculdade), ela pode isolar os aparelhos entre si. Nesse caso, use o roteador do celular como ponto de acesso.

## Ranking (Firebase) e personagens
Ao dar Começar, dois personagens pixelados da música aparecem dançando, um de cada lado da página (Mario e Luigi, Harry e Hermione, Capitão e Marujo, Pianista e Elise), e a página mostra um ranking das músicas mais tocadas. A contagem sobe +1 cada vez que a música completa um ciclo inteiro do padrão (a página descobre isso pelo contador `ciclos` da API). Deixando uma música rodando em loop, ela soma a cada volta. Pausar e retomar não conta, só terminar o ciclo.

Para ativar o ranking:
1. No [console do Firebase](https://console.firebase.google.com), crie um projeto e um **Realtime Database**.
2. Em **Regras**, permita leitura e escrita em `ranking` (público, sem login; qualquer pessoa com a URL consegue alterar):
   ```json
   { "rules": { "ranking": { ".read": true, ".write": true } } }
   ```
3. Copie a URL do banco para `FIREBASE_URL` no início do script de `data/web/index.html` e rode `pio run -t uploadfs`.

Sem `FIREBASE_URL` o ranking fica desligado, mas o resto funciona. O ranking é gravado pelo navegador, então o aparelho que abre a página precisa ter internet. Músicas novas ganham um personagem genérico; para criar um próprio, adicione uma entrada em `DANCERS`.

## Volume
A página tem uma barra de **Volume** (0 a 100%), acima do ranking. Ela chama `POST /api/volume`, e a placa ajusta o duty do PWM dos buzzers na hora, mesmo com a música tocando.

## Comandos PlatformIO
```bash
pio run                # compilar
pio run -t upload      # enviar o firmware
pio run -t uploadfs    # enviar a página e as músicas (pasta data/)
pio device monitor     # Serial Monitor (115200), mostra o IP da placa
```

## Adicionar uma música
Crie um arquivo `.txt` em `data/musicas/`. O nome do arquivo vira o nome da música (ex.: `Tetris.txt` → "Tetris"). Dentro, as durações em ms separadas por vírgula: som, pausa, som, pausa...
```text
120,60,120,120,240,480
```
- Quantidade **par** (se for ímpar, o último valor é ignorado), até 512 valores, até 20 músicas.
- A lista aparece em ordem alfabética.

Depois rode `pio run -t uploadfs` (não precisa regravar o firmware). A página pega a lista da placa sozinha.

## Como funciona
- **Firmware** (C++, PlatformIO, Arduino): os 3 pinos ficam no mesmo canal PWM (LEDC, 2 kHz), que é o que faz o buzzer passivo soar. Ligar e desligar o som é só mudar o duty.
- **Sem `delay()` na reprodução**: o tempo de cada passo é controlado com `millis()` em `updatePlayer()`, chamado no `loop()` junto de `server.handleClient()`.
- **Estados**: parado, tocando e pausado. Pausar guarda quanto do passo já passou, então Começar continua do mesmo ponto. Ao terminar o padrão, a música recomeça em loop.
- **Músicas e página**: ficam no LittleFS. A lista de músicas é montada na inicialização (ordem alfabética) e o conteúdo só é lido quando a música toca.
- **Página**: HTML, CSS e JavaScript puros, sem bibliotecas, frameworks ou CDNs. Ela consulta `/api/estado` a cada 500 ms para se manter sincronizada com a placa. A única chamada externa é o `fetch` ao Firebase, para o ranking.

## API HTTP da placa

| Método | Rota | Resposta / ação |
|---|---|---|
| GET | `/` e `/style.css` | A página |
| GET | `/api/musicas` | Lista de nomes, ex.: `["Fur Elise","Harry Potter"]` |
| GET | `/api/estado` | Estado atual (JSON abaixo) |
| POST | `/api/tocar?n=<n>` | Toca a música `n` (se estava pausada, continua; outra música começa do início) |
| POST | `/api/pausar` | Pausa e guarda a posição |
| POST | `/api/volume?v=<0-100>` | Ajusta o volume |

Todas as rotas de `/api` respondem com o estado:
```json
{"estado":"tocando","musica":1,"volume":100,"ciclos":7,"cicloMusica":1}
```
`estado` é `tocando`, `pausado` ou `parado` (`musica` vale `-1` quando parado). `ciclos` é o total de voltas completas desde que a placa ligou e `cicloMusica` é a música que completou a última volta. Se mudar a API, altere o firmware e a página juntos.

## Regras do projeto
- Prioridade: simplicidade. Não acrescentar recursos nem dependências que não foram pedidos.
- Servidor HTTP só com bibliotecas que já vêm no framework Arduino da ESP32; página sem bibliotecas externas.
- Credenciais do Wi-Fi só em `src/secrets.h`, que não vai para o Git.
- Sem comentários no código (`main.cpp` e `index.html`).
- Não trocar o hardware sem autorização.
- Não afirmar que o hardware foi testado sem acesso físico à placa.

## Testes manuais
- [ ] `pio run` compila e `pio run -t upload` envia o firmware.
- [ ] `pio run -t uploadfs` envia a página e as músicas.
- [ ] O Serial Monitor mostra o endereço da placa.
- [ ] A página abre no PC e no celular e lista as 4 músicas.
- [ ] Começar toca a música selecionada nos 3 buzzers.
- [ ] Pausar silencia; Começar continua do mesmo ponto.
- [ ] Escolher outra música e Começar troca e começa do início.
- [ ] O volume muda o som na hora.
- [ ] O ranking sobe ao fim de cada ciclo.
- [ ] Se o Wi-Fi cair, a página mostra o aviso e volta sozinha quando a conexão voltar.

## Limitações
Todas as músicas usam o mesmo tom (`TONE_HZ`): só variam a duração de sons e pausas. Os padrões são ritmos inspirados nas músicas, **não** reproduções fiéis. A API não tem login: qualquer pessoa na mesma rede controla os buzzers. O Wi-Fi só é configurado na inicialização; se a rede estiver fora do ar quando a placa ligar, reinicie-a depois que ela voltar. Compilar não prova que o buzzer funciona; é preciso testar na placa.
