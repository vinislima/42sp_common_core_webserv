_Este projeto foi criado como parte do currículo da 42 por yvieira-, elvictor e vinda-si._

# Webserv

## Descrição

O Webserv é a implementação de um servidor HTTP não bloqueante escrito em C++ 98. O objetivo deste projeto é entender profundamente o funcionamento do protocolo HTTP, o gerenciamento de conexões TCP/IP através de sockets e a multiplexação de I/O (utilizando ˋpollˋ, ˋepollˋ, ˋselectˋ ou ˋkqueueˋ).

#### Principais funcionalidades suportadas:

- Métodos HTTP básicos (ˋGETˋ, ˋPOSTˋ, ˋDELETEˋ)
- Upload de arquivos
- Execução de scripts CGI
- Múltiplos servidores em diferentes portas baseados em um arquivo de configuração estruturado

## Instruções

Para compilar e executar o servidor, você precisará de um compilador C++ (como ˋc++ˋ ou ˋclang++ˋ) e do utilitário ˋmakeˋ.

#### 1. Clonar o repositório

```git clone <url-do-seu-repositorio>```<br/>
```cd webserv```


#### 2. Compilar o projeto

Utilize o Makefile fornecido para gerar o executável:

```make```


#### 3. Executar o servidor

Execute o servidor passando um arquivo de configuração como argumento:

```./webserv [caminho/para/o/arquivo_de_configuracao.conf]```


## Recursos

- RFC 1945 (HTTP/1.0): Especificação básica do protocolo HTTP.

- RFC 2616 (HTTP/1.1): Gerenciamento de conexões, cabeçalho Host e chunked transfer encoding.

- Uso de IA: Ferramentas de Inteligência Artificial (como ChatGPT/Gemini) foram utilizadas neste projeto estritamente como assistentes de estudo e depuração. Especificamente, a IA foi usada para as seguintes tarefas e partes do projeto:
  1. **Compreensão de RFCs:** Tradução e simplificação de termos técnicos complexos presentes nas documentações oficiais do protocolo HTTP (RFC 1945 e RFC 2616), essenciais para a construção das classes `Request` e `Response`.
  2. **Estudo de Multiplexação de I/O:** Esclarecimento sobre o funcionamento da função `poll()` e sobre como manipular descritores de arquivos em modo não bloqueante (`fcntl()`) na arquitetura do núcleo do servidor (`src/core/Server.cpp`).
  3. **Troubleshooting e Depuração de Código:** Auxílio na interpretação de erros de compilação do C++98 (syntax errors) e sugestões de correção para falhas apontadas por testes de memory leaks durante o desenvolvimento do `ConfigParser`