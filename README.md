# Startup

## Testing

Show ports in use

```bash
lsof -i -P -n | grep LISTEN
```

## Compile && start

```bash
make re
 ./ircserv 6667 abc
```

## Start weechat

```bash
make start
```

# Commands

### Connect

```weechat
/server add irc  host.docker.internal/6667 -notls
/connect irc -password=abc
```

```weechat
/nick nickname
```

```weechat
/join #channel
```

```weechat
/kick nickname
/kick nickname reason
/kick #channel nickname
/kick #channel nickname reason
```

```weechat
/invite nickname #channel
/invite nickname
```

##### Invite Only

```weechat
/mode #channel +i
/mode #channel -i
```

##### Channel Key

```weechat
/mode #channel +k key
/mode #channel -k
```

##### Operator Privileges

```weechat
/mode #channel +o nickname
/mode #channel -o nickname
```

##### User Limit

```weechat
/mode #channel +l limit
/mode #channel -l
```
