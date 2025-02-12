NICK=irccat42
SERVER=localhost
PORT=6667
CHAN="#irccat"

{
  # join channel and say hi
  echo "before starting the loop"
  cat << IRC
NICK $NICK
USER irccat 8 x : irccat
JOIN $CHAN
PRIVMSG $CHAN :Greetings!
IRC

  # forward messages from STDIN to the chan, indefinitely
  while read line ; do
    printf "PRIVMSG $CHAN :%s\r\n" "$line"
  done

  # close connection
  printf "QUIT\r\n"
} | nc $SERVER $PORT | while read line ; do
  case "$line" in
    *PRIVMSG\ $CHAN\ :*) echo "$line" | cut -d: -f3- ;;
    #*) echo "[IGNORE] $line" >&2 ;;
  esac
done