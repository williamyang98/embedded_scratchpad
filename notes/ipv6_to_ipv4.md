- Useful if you want a home server to have its own unique IPv6 address but it doesn't natively support it
- You can setup a IPv4 to IPv6 tunnel for it

1. Using socat (Linux): ```socat TCP6-LISTEN:8001,fork TCP4:192.168.1.104:80```
2. Using netsh (Windows): ```netsh interface portproxy add v6tov4 listenaddress=* listenport=8001 connectaddress=192.168.1.104 connectport=80```