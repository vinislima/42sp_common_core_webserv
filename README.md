_Este projeto foi criado como parte do currículo da 42 por yvieira-, meandrad e vinda-si._

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

- Uso de IA: