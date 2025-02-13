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

## Add server in weechat

```bash
docker exec  -it weechat weechat
weechat
```

# Commands

## Server Commands

### Connect

```weechat
/server add irc  host.docker.internal/6667 -notls
/connect irc -password=abc
```

### Authentication

#### Valid

```weechat

```

#### Invalid

```weechat

```

### Nickname

-   The nickname must be unique. If not, gets extended by a number.

#### Valid

```weechat
/nick nickname
```

#### Invalid

```weechat
/nick nickname a
```

### Username

#### Valid

```weechat
/
```

#### Invalid

```weechat

```

### Join

-   Creates a channel if it does not exist.

#### Valid

```weechat
/join #channel
```

#### Invalid

```weechat
/join
/join #
/join #channel1 #channel2
```

### (Private) Message

#### Valid

```weechat
/msg toto hello you!
/msg #channel hello you!
```

#### Invalid

```weechat

```

## Operator Commands

### Kick

#### Valid

```weechat
/kick nickname
/kick nickname reason
/kick #channel nickname
/kick #channel nickname reason
```

#### Invalid

```weechat

```

### Invite

#### Valid

```weechat
/invite nickname #channel
/invite nickname
```

#### Invalid

```weechat

```

### Topic

#### Valid

```weechat

```

#### Invalid

```weechat

```

### Mode

#### Valid

##### Invite Only

```weechat
/mode #channel +i
/mode #channel -i
```

##### Topic Protection

```weechat
/mode #channel +t
/mode #channel -t
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

#### Invalid

```weechat

```
