# Roteiro de Defesa — Webserv (42)

Este documento é um roteiro **passo a passo**, mapeado 1:1 contra a régua de
avaliação oficial (`evaluation_webserv`) e o `subject.pdf`, e calibrado para
**este repositório específico** (arquivos `default.conf`, `second.conf`,
`third.conf`, rotas e scripts reais em `www/`). Siga a ordem: ela é a ordem
em que a régua avalia (README → código → config → checagens básicas → CGI →
navegador → portas → siege/valgrind → bônus).

> Convenção: `$` = terminal 1 (servidor), `$2` = terminal 2 (cliente/testes).
> Todos os comandos assumem que você está na raiz do repo.

---

## 0. Preparação

```bash
$ make re                      # build limpo, sem religação (relinking) suspeita
$ echo $?                      # deve ser 0, sem warnings (compilamos com -Wall -Wextra -Werror)
$ ls webserv
```

Se `make re` falhar ou aparecer qualquer relinking incremental estranho ao
rodar `make` duas vezes seguidas sem tocar em nada → **flag "Compilação
Inválida"**.

```bash
$ make            # roda de novo sem alterar nada
# Esperado: "make: Nothing to be done for 'all'." (nenhum objeto recompilado)
```

---

## 1. README e Verificação de Conformidade

Abra `README.md` e confira, item a item:

- [ ] 1ª linha em itálico, exatamente:
  `_Este projeto foi criado como parte do currículo da 42 por <login1>[, <login2>...]._`
  → Neste repo: `_Este projeto foi criado como parte do currículo da 42 por yvieira-, elvictor e vinda-si._`
- [ ] Seção **"Descrição"** explicando o propósito/visão geral.
- [ ] Seção **"Instruções"** (compilação/instalação/execução).
- [ ] Seção **"Recursos"** com referências e explicação de uso de IA (para
  quais tarefas/partes do projeto).

Se **qualquer** um destes faltar → nota 0 imediata. Confirme os quatro antes
de prosseguir.

---

## 2. Verifique o código e faça perguntas

Peça ao grupo para abrir `src/core/Server.cpp` ao vivo e responda/mostre:

1. **Explicação básica de um servidor HTTP** — bind/listen/accept, parse de
   request, montagem de response.
2. **Qual função é usada para multiplexação de I/O?**
   → `poll()`, chamada em `Server.cpp:367` (`poll(&_pollFds[0], _pollFds.size(), 1000)`).
3. **Como funciona o `poll()`?** — o grupo deve explicar `pollfd`, `events`
   vs `revents`, e o timeout de 1000 ms usado aqui.
4. **Existe um único `poll()` no loop principal, cobrindo accept + read/write
   de todos os clientes simultaneamente?**
   → Confirme que só há **uma** chamada a `poll()` (`_runEventLoop`,
   `Server.cpp:363-600`) e que o mesmo array `_pollFds` contém os sockets de
   escuta (accept), os sockets de cliente (leitura/escrita) **e** os pipes de
   CGI/arquivo. Se o grupo tiver um `poll()`/`select()` separado por cliente
   ou por listener → **nota 0, avaliação termina**.
5. **Apenas uma leitura OU uma escrita por cliente por iteração do
   `poll()`?** Peça para mostrarem o trecho de dispatch:
   ```cpp
   // Server.cpp ~563-595
   if (_pollFds[i].revents & POLLIN)  { ... _handleClientRead(fd) ...  }
   if (_pollFds[i].revents & POLLOUT) { ... _handleClientWrite(fd) ... }
   ```
   Cada `pollfd` de cliente é registrado com `events = POLLIN` **ou**
   `events = POLLOUT` (nunca os dois ao mesmo tempo — trocado explicitamente
   após terminar de ler o header/servir a resposta), então por definição só
   um dos dois blocos executa por fd por volta do loop. Confirme isso lendo
   o código com o grupo.
6. **Todo `read/recv/write/send` em socket trata erro removendo o cliente?**
   Localize e confira cada ocorrência:
   - `recv()` em `_handleClientRead` (`Server.cpp:129`) → `if (bytesRead <= 0) return false;` e o chamador fecha o fd e apaga o cliente.
   - `send()` em `_handleClientWrite` (`Server.cpp:315`) → `if (sent <= 0) { ...; return false; }`.
   - `read()`/`write()` dos pipes de CGI (`Server.cpp:405,423`) e de arquivo
     (`Server.cpp:500,527`) → todos tratam `<= 0` fechando o fd e removendo
     das tabelas `_cgiToClient`/`_fileToClient`.
   **Importante:** confirme que a checagem é `<= 0` (cobre erro `-1` **e**
   fechamento `0`), não apenas um dos dois — a régua exige checar ambos.
7. **`errno` é verificado após `read/recv/write/send`?**
   → `grep -rn errno src/` não deve retornar nada relacionado a essas
   chamadas. Neste repo, `errno` **não é usado** para decidir fluxo — ok. Se
   o grupo checar `errno` para decidir o que fazer (fora de log), é
   **nota 0 imediata**.
8. **Algum FD é lido/escrito sem passar pelo `poll()`?**
   → Toda leitura de socket de cliente, pipe de CGI e arquivo (para servir
   GET/salvar POST) passa pelo loop de `poll()` (client read/write, CGI pipes,
   file read/write fds — todos registrados em `_pollFds`). Não deve haver
   `read()`/`write()` bloqueante fora desse loop (ex: leitura de arquivo de
   config não conta, é antes do loop). Peça para o grupo confirmar não haver
   nenhum I/O de socket "direto" em outro lugar do código.

Se qualquer ponto acima não estiver claro/correto → avaliação para aqui.

---

## 3. Configuração (`.conf`)

Use `default.conf` (o principal, com todas as features) como base:

```conf
server {                                   # Servidor 1: local.com:8080
    listen 8080;  server_name local.com;
    client_max_body_size 30M;
    root ./www; autoindex off; index index.html;
    error_page 404 /404.html; error_page 403 /403.html; error_page 413 /413.html;
    location /google { allow_methods GET; return 301 http://www.google.com; }
    location /novo/   { allow_methods GET POST; return 302 /files/; }
    location /files/  { allow_methods GET POST DELETE; root ./www/arquivos_pesados; autoindex on; upload_store ./www/arquivos_pesados/files; }
    location /scripts/{ root ./www/; autoindex on; cgi_ext .py; cgi_pass /usr/bin/python3; }
}
server { listen 8080; server_name meuteste.com; root ./www/teste_host; index index.html; }  # vhost
server { listen 8081; server_name porta_diferente.com; root ./www/secreta; autoindex on; }  # 2ª porta
```

```bash
$ ./webserv default.conf
```

### 3.1 Códigos de status HTTP
Antes de mais nada, tenha a [lista oficial de status codes](https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Status) aberta.
Confira, ao longo de todos os testes abaixo, que o código retornado é o
**correto** para a situação (não apenas "parece certo"). Códigos suportados
neste servidor (`src/http/Response.cpp:32-46`): `200, 201, 204, 301, 302,
303, 307, 308, 400, 403, 404, 405, 413, 500, 501`.

### 3.2 Vários sites em interfaces/portas diferentes
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/
$2 curl -i -H "Host: porta_diferente.com" http://localhost:8081/
```
Devem servir `www/index.html` e `www/secreta/` respectivamente.

### 3.3 Página de erro padrão / customizada
```bash
$2 curl -i http://localhost:8080/nao-existe        # deve devolver 404 e o corpo de www/404.html
$2 curl -X DELETE -i http://localhost:8080/google   # allow_methods só GET → 405, corpo customizado se configurado, senão default
```
Compare: renomeie/mova temporariamente `www/404.html` (com acordo do grupo,
não é obrigatório) para ver a página de erro **default gerada pelo
servidor** e depois reverta — ou simplesmente teste um código sem
`error_page` configurada (ex.: 400, 405) para ver a página default embutida.

### 3.4 Limite do corpo da requisição (`client_max_body_size`)
`default.conf` define `30M` no server; use `second.conf` (`client_max_body_size 20;`
= 20 **bytes**) para um teste rápido e determinístico:
```bash
$ ./webserv second.conf
$2 curl -X POST -i -H "Content-Type: plain/text" --data "corpo curto" http://localhost:8080/files/x.txt
# 11 bytes < 20 → deve aceitar (200/201)
$2 curl -X POST -i -H "Content-Type: plain/text" --data "corpo maior que vinte bytes com certeza" http://localhost:8080/files/x.txt
# > 20 bytes → deve devolver 413 Payload Too Large
```

### 3.5 Rotas para diretórios diferentes
```bash
$2 curl -i http://localhost:8080/                 # root ./www
$2 curl -i http://localhost:8080/files/            # root ./www/arquivos_pesados (autoindex on)
$2 curl -i http://localhost:8080/scripts/          # root ./www/ (cgi)
```
Confirme (com o grupo lendo `ConfigParser.cpp`) que cada `location` tem seu
próprio `root`, sobrescrevendo o do `server`.

### 3.6 Arquivo padrão ao pedir um diretório (`index`)
```bash
$2 curl -i http://localhost:8080/          # deve servir www/index.html (index index.html;)
```

### 3.7 Lista de métodos aceitos por rota (com e sem permissão)
```bash
$2 curl -X POST   -i http://localhost:8080/google        # location /google só permite GET → 405
$2 curl -X DELETE -i http://localhost:8080/google        # idem → 405
$2 curl -X POST   -i --data "x" http://localhost:8080/files/permitido.txt   # /files/ permite POST → 200/201
$2 curl -X DELETE -i http://localhost:8080/files/permitido.txt              # /files/ permite DELETE → 200/204
```

---

## 4. Verificações básicas (GET, POST, DELETE)

Use `telnet` e `curl` com `default.conf` rodando:

```bash
$2 telnet localhost 8080
GET / HTTP/1.1
Host: local.com

```
Observe manualmente a resposta completa (status line + headers + body).

```bash
$2 curl -i http://localhost:8080/                                          # GET → 200
$2 curl -X POST -i --data "conteudo de teste" http://localhost:8080/files/meu_teste.txt   # POST → 200/201 (upload_store)
$2 ls www/arquivos_pesados/files/                                          # confirme o arquivo foi criado no disco
$2 curl -i http://localhost:8080/files/meu_teste.txt                       # GET de volta → deve retornar o conteúdo enviado
$2 curl -X DELETE -i http://localhost:8080/files/meu_teste.txt             # DELETE → 200/204
$2 curl -i http://localhost:8080/files/meu_teste.txt                       # GET de novo → 404 (arquivo removido)
```

Requisição desconhecida/malformada (não pode derrubar o servidor):
```bash
$2 curl -X BATATA -i http://localhost:8080/          # método inexistente
$2 printf 'GARBAGE_LINE_SEM_SENTIDO\r\n\r\n' | nc localhost 8080   # request malformado via socket cru
```
Esperado: algum código de erro (400/405/501) e o **servidor continua de pé**
(confira no terminal 1 que o processo não morreu, e mande outro GET normal
em seguida).

---

## 5. Verificar CGI

Rota configurada: `location /scripts/` → `cgi_ext .py; cgi_pass /usr/bin/python3;`,
script real em `www/scripts/teste.py`.

### 5.1 GET
```bash
$2 curl -i "http://localhost:8080/scripts/teste.py?nome=avaliador"
```
Confira no corpo da resposta a variável `QUERY_STRING` sendo repassada e o
CGI listando as `HTTP_*` env vars — prova de que o `execve()` recebeu o
ambiente correto.

### 5.2 POST
```bash
$2 curl -X POST -i --data "corpo enviado via POST para o CGI" "http://localhost:8080/scripts/teste.py"
```
O script lê `sys.stdin.read()` — confira que o corpo aparece refletido na
resposta ("Corpo recebido via POST").

### 5.3 CGI executa no diretório correto (paths relativos)
Peça ao grupo para criar, junto com você, um script que abra um arquivo por
caminho relativo (ex.: `open("dado.txt")`) dentro de `www/scripts/` e
confirme que ele encontra o arquivo — prova que o `chdir()`/cwd do processo
filho é o diretório do script, não o cwd do `webserv`.

### 5.4 CGI com erro (o servidor não pode cair)
Crie com o grupo, ao vivo, um script `www/scripts/erro.py`:
```python
#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n", end="")
x = 1 / 0   # ZeroDivisionError -> processo termina com erro
```
```bash
$2 curl -i http://localhost:8080/scripts/erro.py
```
Esperado: `500 Internal Server Error` e o processo `webserv` **continua
rodando** (o código trata isso: quando o pipe de leitura do CGI fecha sem
dado — `Server.cpp:420-471` — e o `waitpid()` colhe o filho). Confirme com
um GET normal logo em seguida.

### 5.5 CGI com loop infinito (ponto de atenção deste projeto)
```python
#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n", end="")
while True:
    pass
```
```bash
$2 curl -i http://localhost:8080/scripts/loop.py &     # fica pendurado
$2 curl -i http://localhost:8080/                       # em outra aba/terminal, deve responder normalmente
```
**Esperado pela régua:** o servidor deve seguir respondendo outras
requisições (isso funciona aqui, pois o `poll()` não bloqueia por causa de
um pipe sem dado) **e** deve matar o CGI travado após um timeout.
⚠️ **Revisando `Server.cpp`/`Client.hpp`, não foi encontrado nenhum
mecanismo de timeout que dê `kill()`/`SIGKILL` no `cgiPid` travado** — o
pipe fica aberto indefinidamente e o processo filho vira um "zumbi" preso em
loop até o `webserv` ser encerrado. **Teste isso ao vivo com o grupo antes
da defesa** e, se confirmado, implemente um timeout (ex.: registrar
`cgiStartTime` no `Client` e, em `_checkTimeouts()`, `kill(cgiPid, SIGKILL)`
+ `waitpid` quando exceder alguns segundos) — hoje isso é um ponto que pode
custar pontos na seção "Verificar CGI".

---

## 6. Verificar com um navegador

Com `default.conf` rodando, abra o navegador e a aba **Network**:

- [ ] `http://localhost:8080/` → inspecione request/response headers.
- [ ] Navegue pelo site estático (verifique se há CSS/imagens em `www/`; se
  não houver, ao menos confirme que o HTML de `index.html` carrega puro).
- [ ] URL incorreta: `http://localhost:8080/isso-nao-existe` → 404.
- [ ] Listagem de diretório: `http://localhost:8080/files/` (autoindex on).
- [ ] URL redirecionada: `http://localhost:8080/google` → deve navegar (301)
  para `google.com`; e `http://localhost:8080/novo/` → 302 para `/files/`.
- [ ] Qualquer outra rota livre (CGI via navegador, `/scripts/teste.py?x=1`).

---

## 7. Problemas de porta

### 7.1 Várias interfaces/portas
```bash
$ ./webserv default.conf
$2 curl -i http://localhost:8080/     # local.com
$2 curl -i http://localhost:8081/     # porta_diferente.com (www/secreta)
```

### 7.2 Vários sites na mesma interface:porta (host virtual)
`default.conf` já configura `local.com` e `meuteste.com`, ambos em `:8080`.
Este grupo **optou por virtual host** — valide que funciona corretamente:
```bash
$2 curl -i -H "Host: local.com"    http://localhost:8080/    # serve ./www
$2 curl -i -H "Host: meuteste.com" http://localhost:8080/    # serve ./www/teste_host
$2 curl -i http://localhost:8080/                             # sem Host explícito -> confirme qual server "default" responde e se é coerente
```
Pergunte por que escolheram host virtual em vez de erro de config, e
confirme que o comportamento é sempre o mesmo (determinístico).

### 7.3 Múltiplos `webserv` com interface:porta em comum
```bash
$ ./webserv default.conf &          # ocupa 8080/8081
$ ./webserv third.conf              # third.conf também escuta 8080
```
Esperado: o segundo processo deve falhar no `bind()` (`EADDRINUSE`), imprimir
o erro (`Server.cpp:82`, "Erro: Falha no bind...") e **encerrar com código 1**
sem crashar (`main.cpp:32-35` captura a exceção). Confirme:
```bash
$ echo $?     # deve ser 1, não SIGSEGV/abort
```
Pergunte ao grupo por que escolheram esse comportamento (falhar cedo em vez
de ignorar a porta duplicada) e confirme que é coerente e documentado.

---

## 8. Siege & teste de estresse

```bash
$ ./webserv default.conf &
$2 which siege || brew install siege      # ou apt/pacman conforme o SO
```

### 8.1 Disponibilidade > 99.5%
```bash
$2 siege -b -c 30 -t 30s http://localhost:8080/
```
No relatório final, confira `Availability` — deve ser **> 99.5%**. Ajuste
`-c` (clientes concorrentes) combinando com o grupo para um teste justo
(o SO pode limitar `ulimit -n`; ver `ulimit -n` se houver erro de "too many
open files").

### 8.2 Sem vazar memória / sem crescer indefinidamente
Em outro terminal, monitore o processo durante o siege:
```bash
$2 while true; do ps -o rss,vsz,pid,comm -p $(pgrep webserv); sleep 2; done
```
RSS não deve crescer de forma monotônica e ilimitada ao longo do teste.

### 8.3 Sem conexões penduradas
```bash
$2 netstat -an | grep 8080 | grep -c ESTABLISHED    # ou: ss -tan | grep :8080
```
Rode logo após o siege terminar — não deve haver um número anômalo de
conexões em `ESTABLISHED`/`CLOSE_WAIT` acumulando (o `_checkTimeouts()` do
`Server.cpp` derruba conexões ociosas após 60s — confirme esperando esse
tempo se houver sobra).

### 8.4 Uso contínuo sem reiniciar o servidor
```bash
$2 siege -b -c 30 -t 30s http://localhost:8080/
$2 siege -b -c 30 -t 30s http://localhost:8080/     # repita sem reiniciar o webserv
```
Ambos devem terminar com disponibilidade alta, sem precisar reiniciar o
processo entre execuções.

### 8.5 Sem vazamento de memória (Valgrind/leaks)
```bash
$ valgrind --leak-check=full --show-leak-kinds=all ./webserv default.conf
```
Em outro terminal, exercite bastante o servidor (siege leve, GET/POST/DELETE,
CGI, upload) e então `Ctrl+C` no terminal do valgrind. No resumo, confira:
```
All heap blocks were freed -- no leaks are possible
```
ou, no mínimo, `definitely lost: 0 bytes`. Preste atenção especial:
- Após CGI (fork/execve) — memória do processo pai não deve vazar por
  request de CGI.
- Após upload/delete de arquivo.
- Após conexões que dão timeout/erro (client removido do `_clients`/`_pollFds`).

No macOS, alternativa:
```bash
$ leaks --atExit -- ./webserv default.conf
```

---

## 9. Parte Bônus (avaliar **somente** se a obrigatória passou 100%)

Verificado neste repositório:

- **Cookies e sessão:** `grep -ri "cookie\|session" src/` não retorna nada.
  **Não implementado** — pule este item bônus (não pontua).
- **Múltiplos sistemas de CGI:** apenas `cgi_ext .py` / `cgi_pass
  /usr/bin/python3` está configurado (`default.conf`, `ConfigParser.cpp`).
  Se o parser aceitar múltiplos `location`s com `cgi_ext`/`cgi_pass`
  diferentes (ex.: `.php` além de `.py`), teste registrando um segundo
  interpretador; caso contrário, **não implementado** — pule este item.

---

## 10. Checklist final da régua (marque durante a defesa)

- [ ] README completo (seção 1)
- [ ] Código: `poll()` único, 1 leitura/escrita por cliente, erros tratados,
  sem `errno` de controle de fluxo, sem I/O fora do `poll()` (seção 2)
- [ ] Compilação sem religação (seção 0)
- [ ] Config: status codes corretos, múltiplos sites/portas, error page,
  `client_max_body_size`, rotas por diretório, index default, allow_methods
  (seção 3)
- [ ] GET/POST/DELETE ok, método desconhecido não derruba, status corretos,
  upload+retrieve (seção 4)
- [ ] CGI GET/POST, diretório correto, erro tratado (500, sem crash),
  loop infinito não trava outros clientes ⚠️ ver risco de timeout (seção 5)
- [ ] Navegador: estático, 404, listagem, redirect (seção 6)
- [ ] Portas: múltiplas interfaces, vhost mesma interface:porta, bind
  duplicado não crasha (seção 7)
- [ ] Siege > 99.5%, sem leak, sem conexão pendurada, uso contínuo (seção 8)
- [ ] Nenhum crash/segfault durante toda a defesa (senão nota 0)
- [ ] Bônus (só se tudo acima passou): cookies/sessão, múltiplos CGI (seção 9)
