all: execli exesrv 

execli: client.c
	gcc -o clichat client.c -pthread

exesrv: server.c
	gcc -o srvchat server.c -pthread

clear: clichat srvchat
	rm clichat srvchat
