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
    *(O servidor deve retornar erro como 400 ou 405, mas **NUNCA** crashar).*

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

### 5. Estresse e Memória (Siege & Valgrind)
O servidor deve ter disponibilidade superior a 99.5% e 0 vazamentos de memória.
1.  Inicie o servidor com Valgrind (Linux) ou Leaks (Mac):
    ```bash
    valgrind --leak-check=full ./webserv
    ```
2.  Em outro terminal, execute o Siege:
    ```bash
    siege -b -c 50 -t 30s http://localhost:8080/
    ```
3.  Observe a disponibilidade (Availability) no final do Siege.
4.  Pare o servidor (`Ctrl+C`) e verifique o relatório do Valgrind. Nenhuma memória deve ser perdida (All heap blocks were freed).