# WinSock File Transfer
Two apps, one server and the other a client, made using WinSock API.
Made to directly send a file from one PC to another, its meant to be used by the client from the command line.


### How to use:
Server side: All you have to do is run it. 
The server app is designed to be multithreaded so it can accept multiple incoming files

Client side:

Arguments: `>winsockclient [IP address] -f [FILENAME]`

Example: `>winsockclient 192.168.1.1 -f image.png`

Server: All you have to do is run the executable. If the server is on the same PC, instead of the IP address you need to use `localhost`

### Future improvement:
* Add additional functionallity
