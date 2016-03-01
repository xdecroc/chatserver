# Simple tcp chat server using posix sockets

binds to port 8888, uses c++11 threads and mutex

Usage: start chat server './cserver' in one terminal
open two or more terminals and connect via telnet
'telnet localhost 8888'.  (escape ctrl],quit)
Messages from one client will be sent to the other chat clients

to compile:
```bash
  g++ accept.cpp -o cserver -std=c++11 -pthread -g
```
