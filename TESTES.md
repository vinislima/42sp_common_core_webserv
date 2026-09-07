# Guia de Avaliação e Testes (Scale)

Este guia mapeia os requisitos da régua de avaliação da 42 para que possam ser testados rapidamente durante a defesa.



### 1. Testes de Configuração (`.conf`)
*   **Múltiplas portas e sites:** Verifique no `.conf` blocos `server` com portas diferentes (ex: 8080 e 8081).
*   **Páginas de erro padrão:** Acesse uma página que não existe para ver o HTML de erro padrão, ou o arquivo customizado via `error_page`.
*   **Limite de corpo de requisição:**
    ```bash
    curl -X POST -i -d "CORPO MUITO GRANDE" http://localhost:8080/post_body
    ```
    *(Deve retornar `413 Payload Too Large` se o `client_max_body_size` for menor que o texto enviado).*
*   **Restrição de Métodos:**
    ```bash
    curl -X DELETE -i http://localhost:8080/
    ```
    *(Deve retornar `405 Method Not Allowed` se a raiz só aceitar GET).*

### 2. Verificações Básicas (GET, POST, DELETE)
*   **GET Normal:** `curl -i http://localhost:8080/`
*   **Upload de Arquivo (POST):**
    ```bash
    curl -X POST -i -d "Conteudo de teste" http://localhost:8080/files/meu_teste.txt
    ```
*   **Deletar Arquivo (DELETE):**
    ```bash
    curl -X DELETE -i http://localhost:8080/files/meu_teste.txt
    ```
*   **Requisição Desconhecida / Malformada:**
    ```bash
    curl -X BATATA -i http://localhost:8080/
    ```
    *(Deve retornar `501 Not Implemented` - método que o servidor nunca
    implementa, não `405`, que é reservado a método real recusado por uma
    rota específica. Em qualquer caso, o servidor **NUNCA** deve crashar).*

### 3. Testes de CGI
*   **GET e POST em CGI:** Utilize o executável `tester` oficial da 42 ou o script Python de teste:
    ```bash
    curl -i http://localhost:8080/scripts/teste.py?nome=avaliador
    ```
*   **Script com Loop Infinito (Timeout):**
    Crie um script que rode `while True`. Acesse-o pelo navegador. O navegador ficará carregando, mas se você abrir outra aba e acessar o site principal, ele deve responder normalmente. O servidor não pode travar e deve matar o CGI após o timeout.
*   **Script com Erro (Crash):**
    Acesse um script que faz divisão por zero. O servidor deve detectar a falha do processo filho e retornar `500 Internal Server Error` sem cair.

### 4. Testes via Navegador Web
Abra o navegador, acesse `http://localhost:8080` e abra a aba "Network" (Rede) no Inspecionar Elemento:
*   Navegue pelo site estático (CSS, Imagens e HTML devem carregar).
*   Acesse um diretório sem `index.html` e com `autoindex on` para ver a listagem de arquivos.
*   Acesse a rota de redirecionamento (ex: `/google`) e verifique se o status é `301` ou `302`.

### 5. Estresse e Memória (Siege & Leaks/Valgrind)

O servidor deve ter disponibilidade superior a 99.5% e 0 vazamentos de memória.
Use `www/empty.html` (página vazia de verdade) para não distorcer o tempo de
resposta com custo de I/O de disco/CGI - é o que a régua pede literalmente.

**No Linux**, com Valgrind:
```bash
valgrind --leak-check=full ./webserv default.conf
```

**No Mac** (Valgrind não roda no Apple Silicon), com o `leaks` nativo do
sistema — testado e confirmado nesta auditoria (`0 leaks for 0 total leaked
bytes` em tráfego normal e no caminho de timeout de CGI):
```bash
# Suba o servidor em background e guarde o PID
./webserv default.conf &
SERVER_PID=$!

# ... rode o siege e os testes manuais que quiser nesse meio tempo ...

# Depois de gerar tráfego, verifique o heap do processo ainda vivo
leaks $SERVER_PID
```

Em outro terminal, execute o Siege:

```bash
siege -b -c 50 -t 30s http://localhost:8080/empty.html
```

Observe a disponibilidade (Availability) no final do Siege — deve ficar
próxima de 100% (confirmado nesta auditoria: 100.00% em 336k transações,
0 falhas, memória do processo voltando ao valor de antes do teste depois
da carga acabar).
