### Initial testing setup steps

Compile && start
```bash
make re
 ./ircserv 6667 abc
```

Start weechat
```bash
make start
```

Add server in weechat
```weechat
/server add irc docker.host.internal/6667 -notls
/connect irc -password=abc
```
