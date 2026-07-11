# ft_irc — 42KL

> A fully functional IRC server written in C++98, built from scratch using TCP sockets and non-blocking I/O, compatible with real IRC clients.

## Overview

Internet Relay Chat (IRC) is one of the oldest real-time messaging protocols still in active use, defined in RFC 1459 (1993). Despite its age, understanding how IRC works teaches the fundamentals of network programming: socket creation, client multiplexing, message framing, stateful protocol parsing, and concurrent connection management — all without threads.

`ft_irc` implements a complete IRC server that handles multiple clients simultaneously, manages channels, enforces channel modes and permissions, and speaks the IRC protocol well enough to work with standard clients like irssi and WeeChat. No external networking libraries are used — the entire server is built on BSD socket APIs.

## The Challenge

Implement an IRC server that:
- Accepts TCP connections from multiple clients simultaneously
- Uses `poll()` for non-blocking I/O — no multi-threading, no `fork()`
- Handles incomplete messages (TCP is a stream protocol; a single `recv()` may contain partial or multiple commands)
- Implements the full IRC registration handshake: `PASS` → `NICK` → `USER` → welcome replies
- Supports channels with all four required modes
- Passes all commands correctly using a real IRC client as the validator

## Concepts Introduced

- TCP socket lifecycle: `socket()` → `bind()` → `listen()` → `accept()` → `send()`/`recv()` → `close()`
- `poll()` multiplexing: monitoring multiple file descriptors for readability/writability in a single loop without blocking
- Non-blocking I/O: why `O_NONBLOCK` is set and how `EAGAIN`/`EWOULDBLOCK` are handled
- IRC protocol (RFC 1459): message format (`:<prefix> <command> <params> :<trailing>\r\n`), numeric replies, registration flow
- Object-oriented C++98 design: three cooperating classes with clear responsibilities
- Command dispatch via a `std::map<std::string, CommandHandler>` of member function pointers
- Channel state management: membership, operator status, modes, and topic

## Learning Outcomes

After completing this project you will have:
- Built a functioning TCP server that handles real-world protocol traffic
- Understood how event-driven servers work (the same model used by Node.js, nginx, and Redis)
- Implemented a stateful text protocol parser that handles partial reads and concatenated messages
- Designed a clean OOP architecture in C++98 with encapsulation and no use of modern C++ features
- Gained practical experience debugging network issues using tools like `nc`, Wireshark, and actual IRC clients

## Architecture

```
Server
 ├── socketFd          TCP listening socket
 ├── pollfds[]         All active file descriptors (listen + clients)
 ├── clientsByFd       Map<fd, Client*>
 ├── clientsByNickname Map<nickname, Client*>
 ├── channels          Map<name, Channel*>
 └── commandMap        Map<string, CommandHandler>

Client
 ├── fd, nickname, username, realname
 ├── authenticated / registered flags
 └── write buffer (for partial sends)

Channel
 ├── members[], operators[], invitedNicknames[]
 └── modes: inviteMode, topicMode, keyMode, limitMode
```

**Event loop (simplified):**
```
while (running):
    poll(pollfds)
    if listening socket ready → acceptClient()
    for each client fd:
        if readable  → receiveClientData() → parseExecuteCommand()
        if writable  → handleClientWrite()  (flush pending output)
```

## Supported Commands

| Category | Commands |
|----------|---------|
| Registration | `PASS`, `NICK`, `USER` |
| Messaging | `PRIVMSG`, `NOTICE` |
| Channels | `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`, `MODE` |
| Server | `PING`/`PONG`, `QUIT`, `WHOIS` |

**Channel modes:**

| Flag | Meaning |
|------|---------|
| `+i` | Invite-only — only invited users can join |
| `+t` | Topic locked — only channel operators can change the topic |
| `+k <key>` | Password-protected channel |
| `+o <nick>` | Grant / revoke operator status |
| `+l <limit>` | Set maximum number of members |

## How to Build

```bash
make
make re
make fclean
```

## How to Deploy and Use

**Start the server:**
```bash
./ircserv <port> <password>

# Example
./ircserv 6667 mysecret
```

**Connect with irssi:**
```
/connect localhost 6667 mysecret
/nick mynick
/join #general
/msg #general Hello, world!
```

**Connect with WeeChat:**
```
/server add local localhost/6667 -password=mysecret
/connect local
```

**Quick test with netcat:**
```bash
nc localhost 6667
PASS mysecret
NICK testuser
USER testuser 0 * :Test User
JOIN #test
PRIVMSG #test :hello
```
