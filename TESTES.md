# Roteiro de Defesa — Webserv (42)

Este documento é um roteiro **passo a passo**, mapeado 1:1 contra a régua de
avaliação oficial (`evaluation_webserv`) e o `subject.pdf`, calibrado para
**este repositório específico** (`default.conf`, `second.conf`, `third.conf`,
rotas e scripts reais em `www/`). Siga a ordem: ela é a ordem em que a régua
avalia (README → código → config → checagens básicas → CGI → navegador →
portas → siege/valgrind → bônus).

> Convenção: `$` = terminal 1 (servidor), `$2` = terminal 2 (cliente/testes).
> Todos os comandos assumem que você está na raiz do repo.

---

## 0. Preparação

```bash
$ make re                      # build limpo, sem religação (relinking) suspeita
$ echo $?                      # deve ser 0, sem warnings (-Wall -Wextra -Werror -std=c++98)
$ make                         # roda de novo sem tocar em nada
# Esperado: "make: Nothing to be done for 'all'." (nenhum objeto recompilado)
```

Se `make re` falhar, ou `make` sozinho religar algo sem motivo → **flag
"Compilação Inválida"**.

---

## 1. README e Verificação de Conformidade

Abra `README.md` e confira, item a item. **O subject exige o README em
inglês** (aviso em destaque no `subject.pdf`) — o texto abaixo já é a versão
em inglês exigida, não uma tradução da régua:

- [ ] 1ª linha em itálico, exatamente:
  `_This project has been created as part of the 42 curriculum by <login1>[, <login2>...]._`
  → Neste repo: `_This project has been created as part of the 42 curriculum by yvieira-, elvictor, vinda-si._`
- [ ] Seção **"Description"** explicando o propósito/visão geral.
- [ ] Seção **"Instructions"** (compilação/instalação/execução).
- [ ] Seção **"Resources"** com referências e explicação de uso de IA (para
  quais tarefas/partes do projeto foi usada).

Se **qualquer** um destes faltar → nota 0 imediata. Confirme os quatro antes
de prosseguir.

---

## 2. Verifique o código e faça perguntas

Peça ao grupo para abrir `src/core/Server.cpp` ao vivo e responda/mostre:

1. **Explicação básica de um servidor HTTP** — bind/listen/accept, parse de
   request, montagem de response.
2. **Qual função é usada para multiplexação de I/O?**
   → `poll()`, chamada uma única vez no loop principal (`_runEventLoop`).
3. **Como funciona o `poll()`?** — o grupo deve explicar `pollfd`, `events`
   vs `revents`, e o timeout usado no loop.
4. **Existe um único `poll()` no loop principal, cobrindo accept + leitura/
   escrita de todos os clientes simultaneamente?**
   → Confirme que só há **uma** chamada a `poll()` e que o mesmo array de
   `pollfd` contém os sockets de escuta (accept), os sockets de cliente
   (leitura/escrita) **e** os pipes de CGI/arquivo. Se houver um `poll()`/
   `select()` separado por cliente ou por listener → **nota 0, avaliação
   termina imediatamente**.
5. **Apenas uma leitura OU uma escrita por cliente por iteração do
   `poll()`?** Peça para mostrarem o dispatch principal do loop:
   ```cpp
   if (_pollFds[i].revents & POLLIN)  { ... leitura do cliente ...  }
   if (_pollFds[i].revents & POLLOUT) { ... escrita no cliente ... }
   ```
   Cada `pollfd` de cliente é registrado com `events = POLLIN` **ou**
   `events = POLLOUT` (nunca os dois ao mesmo tempo — trocado explicitamente
   após terminar de ler o header/servir a resposta), então por definição só
   um dos dois blocos executa por fd por volta do loop. Confirme lendo o
   código com o grupo.
6. **Todo `read/recv/write/send` em socket trata erro removendo o cliente?**
   Localize com o grupo cada ocorrência (`grep -n "recv(\|send(\|read(\|write(" src/core/Server.cpp`)
   e confirme que **todas** checam `<= 0` (cobre erro `-1` **e** fechamento
   `0` — a régua exige checar ambos os casos, não só um) e, ao detectar,
   fecham o fd e removem o cliente/processo das estruturas internas.
7. **`errno` é verificado após `read/recv/write/send`?**
   ```bash
   $2 grep -rn "errno" src/
   ```
   → Não deve retornar nada usado para decidir fluxo de execução (apenas log,
   se houver). Se o grupo checar `errno` para decidir o que fazer, é
   **nota 0 imediata**.
8. **Algum FD é lido/escrito sem passar pelo `poll()`?**
   → Toda leitura de socket de cliente, pipe de CGI e arquivo (servir GET/
   salvar POST) deve passar pelo array observado pelo `poll()`. Peça ao
   grupo para confirmar que não há nenhum `read()`/`write()`/`recv()`/`send()`
   bloqueante em socket ou pipe fora desse loop (leitura do arquivo `.conf`
   no início do programa não conta, é antes do loop de eventos).

Se qualquer ponto acima não estiver claro ou incorreto → avaliação para aqui.

---

## 3. Configuração (`.conf`)

Use `default.conf` (o mais completo, com todas as features) como base:

```conf
server {                                   # Servidor 1: local.com:8080
    listen 8080;  server_name local.com;
    client_max_body_size 30M;
    root ./www; autoindex off; index index.html;
    error_page 404 /404.html; error_page 403 /403.html; error_page 413 /413.html;
    location / { allow_methods GET; }                         # restringe a raiz a GET
    location /google { allow_methods GET; return 301 http://www.google.com; }
    location /novo/   { allow_methods GET POST; return 302 /files/; }
    location /files/  { allow_methods GET POST DELETE; root ./www/arquivos_pesados; autoindex on; upload_store ./www/arquivos_pesados/files; }
    location /scripts/{ root ./www/; autoindex on; cgi_ext .py; cgi_pass /usr/bin/python3; }
}
server { listen 8080; server_name meuteste.com; root ./www/teste_host; index index.html; }  # vhost, mesma porta
server { listen 8081; server_name porta_diferente.com; root ./www/secreta; autoindex on; }  # 2ª porta
```

```bash
$ ./webserv default.conf
```

### 3.1 Códigos de status HTTP
Antes de mais nada, tenha a [lista oficial de status codes](https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Status) aberta e
confira, em **todos** os testes abaixo, que o código retornado é o correto
para a situação. Códigos suportados neste servidor: `200, 201, 204, 301, 302,
303, 307, 308, 400, 403, 404, 405, 413, 431, 500, 501, 504` (o `504` é
devolvido só no timeout de CGI, seção 5.5; o `431` no request-line/headers
maiores que 8192 bytes).

### 3.2 Vários sites em interfaces/portas diferentes
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/
$2 curl -i -H "Host: porta_diferente.com" http://localhost:8081/
```
Devem servir `www/index.html` e `www/secreta/` respectivamente.

### 3.3 Página de erro padrão / customizada
```bash
$2 curl -i http://localhost:8080/nao-existe        # 404, corpo de www/404.html
$2 curl -X DELETE -i -H "Host: local.com" http://localhost:8080/       # location "/" só permite GET -> 405
```
Para ver a página de erro **default gerada pelo servidor** (sem
`error_page` configurada), provoque um código sem página customizada — por
exemplo, o próprio 405 acima (só há `error_page` para 404/403/413 em
`default.conf`) — e compare com o 404 (que usa `www/404.html`).

### 3.4 Limite do corpo da requisição (`client_max_body_size`)
`default.conf` define `30M` no server; use `second.conf`
(`client_max_body_size 20;` = 20 **bytes**) para um teste rápido e
determinístico:
```bash
$ ./webserv second.conf
$2 curl -X POST -i -H "Content-Type: plain/text" --data "corpo curto" http://localhost:8080/files/x.txt
# 11 bytes < 20 -> deve aceitar (200/201)
$2 curl -X POST -i -H "Content-Type: plain/text" --data "corpo maior que vinte bytes com certeza" http://localhost:8080/files/x.txt
# > 20 bytes -> deve devolver 413 Payload Too Large
```
`default.conf`/`second.conf` também permitem `client_max_body_size` por
`location` (sobrescrevendo o valor do `server`) — se quiser demonstrar isso,
adicione a diretiva dentro de um bloco `location` e repita o teste apontando
para essa rota.

### 3.5 Rotas para diretórios diferentes
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/           # root ./www
$2 curl -i -H "Host: local.com" http://localhost:8080/files/     # root ./www/arquivos_pesados (autoindex on)
$2 curl -i -H "Host: local.com" http://localhost:8080/scripts/   # root ./www/ (cgi)
```
Confirme (com o grupo lendo `ConfigParser.cpp`/`LocationConfig.cpp`) que cada
`location` tem seu próprio `root`, sobrescrevendo o do `server`.

### 3.6 Arquivo padrão ao pedir um diretório (`index`)
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/          # deve servir www/index.html (index index.html;)
```

### 3.7 Lista de métodos aceitos por rota (com e sem permissão)
```bash
$2 curl -X POST   -i -H "Host: local.com" http://localhost:8080/google      # location /google só permite GET -> 405
$2 curl -X DELETE -i -H "Host: local.com" http://localhost:8080/google      # idem -> 405
$2 curl -X POST   -i -H "Host: local.com" --data "x" http://localhost:8080/files/permitido.txt   # /files/ permite POST -> 200/201
$2 curl -X DELETE -i -H "Host: local.com" http://localhost:8080/files/permitido.txt               # /files/ permite DELETE -> 200/204
```

> Repare que todos os `curl` acima usam `-H "Host: local.com"` explicitamente:
> como HTTP/1.1 exige o header `Host`, e há mais de um `server_name` na porta
> 8080 (`local.com` e `meuteste.com`), sem esse header o servidor cai no
> "default server" da porta (o primeiro `server{}` daquela porta no `.conf`,
> que aqui já é `local.com` — mas prefira ser explícito nos testes).

---

## 4. Verificações básicas (GET, POST, DELETE)

Use `telnet` e `curl` com `default.conf` rodando:

```bash
$2 telnet localhost 8080
GET / HTTP/1.1
Host: local.com

```
Observe manualmente a resposta completa (status line + headers + body). Note
que HTTP/1.1 usa Keep-Alive por padrão neste servidor — a conexão pode ficar
aberta aguardando outra requisição; para fechar, envie `Connection: close`
ou aperte `Ctrl+]` seguido de `quit`.

```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/                                          # GET -> 200
$2 curl -X POST -i -H "Host: local.com" --data "conteudo de teste" http://localhost:8080/files/meu_teste.txt   # POST -> 200/201 (upload_store)
$2 ls www/arquivos_pesados/files/                                          # confirme o arquivo foi criado no disco
$2 curl -i -H "Host: local.com" http://localhost:8080/files/meu_teste.txt                       # GET de volta -> deve retornar o conteúdo enviado
$2 curl -X DELETE -i -H "Host: local.com" http://localhost:8080/files/meu_teste.txt             # DELETE -> 200/204
$2 curl -i -H "Host: local.com" http://localhost:8080/files/meu_teste.txt                       # GET de novo -> 404 (arquivo removido)
```

Requisição desconhecida/malformada (não pode derrubar o servidor):
```bash
$2 curl -X BATATA -i -H "Host: local.com" http://localhost:8080/          # método que o servidor nunca implementa
$2 printf 'GARBAGE_LINE_SEM_SENTIDO\r\n\r\n' | nc localhost 8080          # request malformado via socket cru
```
Esperado: `501 Not Implemented` para o método inexistente (`BATATA`) — este
servidor distingue **501** (método que ele nunca implementa, tipo `PATCH`/
`BATATA`) de **405** (método real, mas recusado por `allow_methods` de uma
rota específica, como nos testes da seção 3.7). Confirme que os dois casos
retornam códigos diferentes e corretos. Em qualquer caso, o **servidor
continua de pé** — confira no terminal 1 que o processo não morreu, e mande
outro GET normal em seguida.

Teste também path traversal (deve ser bloqueado, não vazar arquivos fora do
`root`):
```bash
$2 curl -i -H "Host: local.com" "http://localhost:8080/../../../../etc/passwd"
$2 curl -i -H "Host: local.com" "http://localhost:8080/files/../../etc/passwd"
```
Esperado: erro (400/403/404), nunca o conteúdo de `/etc/passwd`.

---

## 5. Verificar CGI

Rota configurada: `location /scripts/` → `cgi_ext .py; cgi_pass /usr/bin/python3;`,
script real em `www/scripts/teste.py`.

### 5.1 GET
```bash
$2 curl -i -H "Host: local.com" "http://localhost:8080/scripts/teste.py?nome=avaliador"
```
Confira no corpo da resposta a variável `QUERY_STRING` sendo repassada e o
CGI listando as `HTTP_*` env vars — prova de que o `execve()` recebeu o
ambiente correto.

### 5.2 POST
```bash
$2 curl -X POST -i -H "Host: local.com" --data "corpo enviado via POST para o CGI" "http://localhost:8080/scripts/teste.py"
```
O script lê `sys.stdin.read()` — confira que o corpo aparece refletido na
resposta ("Corpo recebido via POST").

### 5.3 CGI executa no diretório correto (paths relativos)
Peça ao grupo para criar, junto com você, um script que abra um arquivo por
caminho relativo (ex.: `open("dado.txt")`) dentro de `www/scripts/` e
confirme que ele encontra o arquivo — prova que o cwd do processo filho é o
diretório do script, não o cwd do `webserv`.

### 5.4 CGI com erro (o servidor não pode cair)
Crie com o grupo, ao vivo, um script `www/scripts/erro.py`:
```python
#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n", end="")
x = 1 / 0   # ZeroDivisionError -> processo termina com exit code != 0
```
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/scripts/erro.py
```
Esperado: `500 Internal Server Error` (o servidor checa o exit code do
processo filho via `waitpid`, não só se o pipe fechou) e o processo
`webserv` **continua rodando**. Confirme com um GET normal logo em seguida.

### 5.5 CGI com loop infinito (timeout)
```python
#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n", end="")
while True:
    pass
```
```bash
$2 curl -i -H "Host: local.com" http://localhost:8080/scripts/loop.py &     # fica pendurado
$2 curl -i -H "Host: local.com" http://localhost:8080/                       # em outro terminal, deve responder normalmente e na hora
```
Esperado: o servidor segue respondendo outras requisições normalmente
(não bloqueia por causa de um CGI travado), e depois de **10 segundos** o
processo do CGI travado recebe `SIGKILL` (timeout implementado em
`Server::_checkCgiTimeouts`), a 1ª requisição em background recebe
`504 Gateway Timeout` (não `500` — é um timeout do "gateway" CGI, não um
erro interno do servidor) e não fica pendurada para sempre. Confirme
observando o log `[TIMEOUT] CGI (PID ...) excedeu 10s...` no terminal do
servidor, e que não sobra processo zumbi (`ps aux | grep loop.py` depois de
~15s não deve mostrar nada).

---

## 6. Verificar com um navegador

Com `default.conf` rodando, abra o navegador e a aba **Network**:

> **Sem acesso a `/etc/hosts`?** Nas máquinas do campus (`sudo`/edição de
> `/etc/hosts` bloqueados) dá pra resolver `local.com`/`meuteste.com` sem
> nenhum acesso de root, direto no navegador:
>
> - **Chrome** (recomendado — restringe a resolução só a esses dois nomes,
>   sem afetar o resto da internet, o que importa pro teste do redirect pro
>   `google.com` real mais abaixo):
>   ```bash
>   google-chrome --user-data-dir="$HOME/.chrome-webserv-test" \
>     --host-resolver-rules="MAP local.com 127.0.0.1, MAP meuteste.com 127.0.0.1" \
>     http://local.com:8080/
>   ```
>   Use um `--user-data-dir` novo/dedicado: se já existe uma janela do Chrome
>   aberta, a flag só é lida na criação de um processo novo, então abrir uma
>   aba na instância já existente a ignora silenciosamente.
> - **Firefox** (alternativa): `about:config` → criar `network.dns.forceResolve`
>   (string) = `127.0.0.1`. É uma pref **global** (não dá pra restringir a
>   hostnames específicos), então desligue-a (apague o valor) antes de testar
>   o redirect `/google` → `google.com`, senão ele também cai em `127.0.0.1`.
>
> Se você tiver `sudo` disponível, o caminho mais simples continua sendo
> `echo "127.0.0.1 local.com meuteste.com" | sudo tee -a /etc/hosts`.

- [ ] `http://local.com:8080/` → inspecione request/response headers.
- [ ] Navegue pelo site estático (`www/index.html`); confira que o
  `Content-Type` da resposta é `text/html` (tabela MIME implementada em
  `Response.cpp` cobre `.html/.css/.js/.json/.png/.jpg/...`).
- [ ] URL incorreta: `http://local.com:8080/isso-nao-existe` → 404.
- [ ] Listagem de diretório: `http://local.com:8080/files/` (autoindex on).
- [ ] URL redirecionada: `http://local.com:8080/google` → deve navegar (301)
  para `google.com`; e `http://local.com:8080/novo/` → 302 para `/files/`.
- [ ] `http://meuteste.com:8080/` → confirme que serve `www/teste_host/`, um
  site **diferente** do `local.com`, na mesma porta (virtual host).
- [ ] Qualquer outra rota livre (CGI via navegador,
  `/scripts/teste.py?x=1`).

---

## 7. Problemas de porta

### 7.1 Várias interfaces/portas
```bash
$ ./webserv default.conf
$2 curl -i -H "Host: local.com" http://localhost:8080/     # local.com:8080
$2 curl -i -H "Host: porta_diferente.com" http://localhost:8081/   # porta_diferente.com:8081
```

### 7.2 Vários sites na mesma interface:porta (host virtual)
`default.conf` já configura `local.com` e `meuteste.com`, ambos em `:8080`.
Este grupo **optou por virtual host** — valide que funciona corretamente:
```bash
$2 curl -i -H "Host: local.com"    http://localhost:8080/    # serve ./www
$2 curl -i -H "Host: meuteste.com" http://localhost:8080/    # serve ./www/teste_host
$2 curl -i http://localhost:8080/                             # sem Host -> cai no "default server" da porta (1º server{} do .conf nessa porta)
```
Pergunte por que escolheram host virtual em vez de erro de config, e
confirme que o comportamento é sempre o mesmo (determinístico) — inclusive
que o matching usa porta **+** `server_name` (não apenas `server_name`
global), então dois `server_name` iguais em portas diferentes não se
confundem.

### 7.3 Múltiplos `webserv` com interface:porta em comum
```bash
$ ./webserv default.conf &          # ocupa 8080/8081
$ ./webserv third.conf              # third.conf também escuta 8080
```
Esperado: o segundo processo deve falhar no `bind()` (`EADDRINUSE`), imprimir
o erro e **encerrar com código 1** sem crashar:
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

Use `www/empty.html` (página realmente vazia) para o teste de disponibilidade
puro — é o que a régua pede literalmente ("uma requisição GET simples em uma
página vazia").

### 8.1 Disponibilidade > 99.5%
```bash
$2 siege -b -c 30 -t 30s http://local.com:8080/empty.html
```
No relatório final, confira `Availability` — deve ser **> 99.5%**. Combine
com o grupo os valores de `-c` (clientes concorrentes), `-d` (delay antes de
reconectar) e `-r` (retries) antes de rodar — depende do SO/`ulimit -n`.

### 8.2 Sem vazar memória / sem crescer indefinidamente
Em outro terminal, monitore o processo durante o siege:
```bash
$2 while true; do ps -o rss,vsz,pid,comm -p $(pgrep webserv); sleep 2; done
```
RSS não deve crescer de forma monotônica e ilimitada ao longo do teste.

### 8.3 Sem conexões penduradas
```bash
$2 ss -tan | grep :8080 | grep -c ESTAB    # ou: netstat -an | grep 8080 | grep -c ESTABLISHED
```
Rode logo após o siege terminar — não deve haver um número anômalo de
conexões em `ESTABLISHED`/`CLOSE_WAIT` acumulando (há timeout de conexão
ociosa no servidor — confirme esperando um pouco se houver sobra).

### 8.4 Uso contínuo sem reiniciar o servidor
```bash
$2 siege -b -c 30 -t 30s http://local.com:8080/empty.html
$2 siege -b -c 30 -t 30s http://local.com:8080/empty.html     # repita sem reiniciar o webserv
```
Ambos devem terminar com disponibilidade alta, sem precisar reiniciar o
processo entre execuções.

### 8.5 Sem vazamento de memória (Valgrind/leaks)

**Linux**, com Valgrind:
```bash
$ valgrind --leak-check=full --show-leak-kinds=all ./webserv default.conf
```

**macOS** (Valgrind não roda em Apple Silicon), com `leaks` nativo:
```bash
$ ./webserv default.conf &
$ SERVER_PID=$!
# ... gere tráfego (siege leve + testes manuais de GET/POST/DELETE/CGI/upload,
#     incluindo o caminho de timeout de CGI da seção 5.5) ...
$ leaks $SERVER_PID
```

Em qualquer um dos dois, gere bastante tráfego variado antes de checar
(GET/POST/DELETE, upload, CGI normal, CGI com erro, CGI com timeout) e então
finalize (`Ctrl+C` no Valgrind, ou apenas `leaks $SERVER_PID` com o processo
ainda vivo). Esperado:
- Valgrind: `All heap blocks were freed -- no leaks are possible` (ou, no
  mínimo, `definitely lost: 0 bytes`).
- `leaks`: `0 leaks for 0 total leaked bytes`.

Preste atenção especial a: memória do CGI (fork/execve) não vazando por
requisição, upload/delete de arquivo, e conexões que dão timeout/erro
(cliente removido de todas as estruturas internas).

---

## 9. Parte Bônus (avaliar **somente** se a obrigatória passou 100%)

Verificado neste repositório:

- **Cookies e sessão:**
  ```bash
  $2 grep -ri "cookie\|session" src/
  ```
  Não retorna nada relacionado a um sistema funcional de cookie/sessão.
  **Não implementado** — pule este item bônus (não pontua).
- **Múltiplos sistemas de CGI:** apenas `cgi_ext .py` / `cgi_pass
  /usr/bin/python3` está configurado em `default.conf`. Se o parser aceitar
  múltiplos `location`s com `cgi_ext`/`cgi_pass` diferentes (ex.: `.php`
  além de `.py`), teste registrando um segundo interpretador; caso
  contrário, **não implementado** — pule este item.

---

## 10. Checklist final da régua (marque durante a defesa)

- [ ] README completo (seção 1)
- [ ] Código: `poll()` único, 1 leitura/escrita por cliente, erros tratados,
  sem `errno` de controle de fluxo, sem I/O fora do `poll()` (seção 2)
- [ ] Compilação sem religação (seção 0)
- [ ] Config: status codes corretos, múltiplos sites/portas, error page,
  `client_max_body_size` (global e por location), rotas por diretório,
  index default, allow_methods (seção 3)
- [ ] GET/POST/DELETE ok, método desconhecido -> 501 (não derruba o
  servidor), status corretos, upload+retrieve, sem path traversal (seção 4)
- [ ] CGI GET/POST, diretório correto, erro do script -> 500 sem crash,
  loop infinito -> timeout de 10s mata o processo e libera o cliente
  (seção 5)
- [ ] Navegador: estático (com Content-Type correto), 404, listagem,
  redirect, virtual host (seção 6)
- [ ] Portas: múltiplas interfaces, vhost mesma interface:porta, bind
  duplicado não crasha (seção 7)
- [ ] Siege > 99.5% em `www/empty.html`, sem leak, sem conexão pendurada,
  uso contínuo (seção 8)
- [ ] Nenhum crash/segfault durante toda a defesa (senão nota 0)
- [ ] Bônus (só se tudo acima passou): cookies/sessão, múltiplos CGI — ambos
  **não implementados** neste repositório, pular (seção 9)
