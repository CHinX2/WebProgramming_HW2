all: execli exesrv 

execli: client.c
	gcc -o chatcli client.c -pthread

exesrv: server.c
	gcc -o chatsrv server.c -pthread

clear: chatcli chatsrv
	rm chatcli chatsrv
