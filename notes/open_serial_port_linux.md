1. Set baud rate of deviceL: ```stty <BAUD_RATE> -F /dev/ttySx```
2. Open using socat: ```socat - /dev/ttySx,raw```

NOTE: Baud rate is 115200 in ```websocket-demo``` for esp8266