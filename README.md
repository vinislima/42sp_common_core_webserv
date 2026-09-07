_This project has been created as part of the 42 curriculum by yvieira-, elvictor, vinda-si._

# Webserv

## Description

Webserv is a non-blocking HTTP/1.1 server written from scratch in C++98. The goal of the project is to understand, at a low level, how the HTTP protocol actually works end to end: parsing raw requests off a TCP socket, managing many concurrent connections with a single multiplexed event loop (`poll()`), serving a fully static website, handling file uploads, running CGI scripts, and returning accurate HTTP status codes for every situation — including malformed requests, oversized bodies, and CGI failures.

Everything the server does — which ports it listens on, which sites it serves, routing, redirects, upload storage, directory listing, CGI — is driven entirely by a configuration file passed on the command line, in a syntax inspired by NGINX's `server`/`location` blocks.

### Key features

- `GET`, `POST` and `DELETE` methods, plus accurate `501 Not Implemented` for any method the server doesn't support at all, and `405 Method Not Allowed` for a method a specific route forbids.
- Single non-blocking `poll()` loop driving every socket, pipe and (optionally) disk file read/write — no blocking I/O anywhere in the request path.
- HTTP/1.1 Keep-Alive with request pipelining support; HTTP/1.0 falls back to connection-per-request unless `Connection: keep-alive` is explicit.
- Virtual hosts: multiple `server{}` blocks can share the same `listen` port and are told apart by the `Host` header, matched per-port so a name configured on one port never leaks into another.
- File uploads, including real `multipart/form-data` (browser `<form>` / `curl -F`) and raw uploads, with per-location `upload_store` and `client_max_body_size` (overridable per route, inherited from the server otherwise).
- CGI execution (`fork()` + `execve()`) based on file extension, run in the script's own directory, with the full CGI environment variable set and support for chunked request bodies and CGI output with no `Content-Length` (EOF-terminated).
- A dedicated timeout kills a CGI script stuck in an infinite loop instead of hanging the connection forever; a CGI that exits with a non-zero status returns `500` instead of a bogus `200`.
- Directory listing (`autoindex`), a configurable default index file, HTTP redirects, custom error pages with a built-in fallback, and Path Traversal protection (`..` can never resolve outside a location's configured root).

## Instructions

You need a C++ compiler (`c++` or `clang++`) and `make`.

### 1. Clone the repository

```bash
git clone <your-repository-url>
cd webserv
```

### 2. Build

```bash
make
```

`make re` forces a full rebuild; `make clean`/`make fclean` remove the object files (and, for `fclean`, the binary too).

### 3. Run

```bash
./webserv [path/to/config_file.conf]
```

The configuration file is optional — omitting it falls back to `default.conf` in the current directory. `default.conf`, `second.conf` and `third.conf` in this repository are ready-to-use examples.

### Configuration file example

```nginx
server {
    listen 8080;
    server_name local.com;
    client_max_body_size 30M;

    root ./www;
    autoindex off;
    index index.html;

    error_page 404 /404.html;

    location /files/ {
        allow_methods GET POST DELETE;
        root ./www/arquivos_pesados;
        autoindex on;
        upload_store ./www/arquivos_pesados/files;
    }

    location /scripts/ {
        cgi_ext .py;
        cgi_pass /usr/bin/python3;
    }
}
```

## Resources

- [RFC 1945 (HTTP/1.0)](https://datatracker.ietf.org/doc/html/rfc1945) — the baseline HTTP specification.
- [RFC 2616 (HTTP/1.1)](https://datatracker.ietf.org/doc/html/rfc2616) — connection management, the `Host` header, and chunked transfer encoding.
- [RFC 7230](https://datatracker.ietf.org/doc/html/rfc7230) / [RFC 7578](https://datatracker.ietf.org/doc/html/rfc7578) — message syntax (header case-insensitivity, framing) and `multipart/form-data`, used while hardening the request parser and the upload path.
- [CGI: Common Gateway Interface (RFC 3875)](https://datatracker.ietf.org/doc/html/rfc3875) — the environment variables and process model behind `Response::_handleCGI`.

### AI usage

AI tools (ChatGPT / Gemini / Claude) were used throughout this project strictly as study and debugging assistants — never to generate a feature the team didn't understand and couldn't explain. Concretely:

1. **Understanding RFCs** — simplifying dense HTTP/1.0 and HTTP/1.1 specification language while designing the `Request` and `Response` classes.
2. **I/O multiplexing** — clarifying how `poll()` and non-blocking file descriptors (`fcntl()`) fit together in the event loop at the core of `src/core/Server.cpp`.
3. **Troubleshooting** — help interpreting C++98 compiler errors and diagnosing memory leaks reported by testing tools during `ConfigParser` development.
4. **Post-implementation audit** — after the mandatory part was functionally complete, an AI-assisted line-by-line review against the official subject and grading rubric was used to find and fix bugs that would fail the evaluation live (virtual host matching by port, CGI timeout and crash handling, Path Traversal, multipart upload corruption, HTTP conformance gaps, and others) — each fix was reviewed, tested live against a running server, and understood by the team before being committed.
