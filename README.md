# WinSock File Transfer
Two CLI apps, a client and server, made using WinSock API.
You can send a file from one PC to another.

### How to use:
Client side:

Arguments: `>winsockclient [IP address] -f [FILENAME]`

Example: `>winsockclient 192.168.1.1 -f image.png`

Server: All you have to do is run the executable. The server has multithreading so it can accept multiple files at once. If the server is on the same PC, instead of the IP address you need to use `localhost`

### Future improvements:
* Add additional functionallity
