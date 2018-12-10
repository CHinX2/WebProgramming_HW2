#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
void *recvmg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(their_sock,msg,500,0)) > 0) 
	{
		msg[len] = '\0';
		fputs(msg,stdout);
		memset(msg,'\0',sizeof(msg));
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in client_addr;
	int sockmsg;
	int their_sock;
	int their_addr_size;
	int portno;
	pthread_t sendt,recvt;
	char msg[500];
	char username[100];
	char res[600];
	char ip[INET_ADDRSTRLEN];
	int len;
	char buf[512];

	if(argc > 3) 
	{
		printf("command must be ./[exefile] [usrname] [port number]\n");
		exit(1);
	}
	portno = atoi(argv[2]);
	strcpy(username,argv[1]);
	sockmsg = socket(AF_INET,SOCK_STREAM,0);
	memset(client_addr.sin_zero,'\0',sizeof(client_addr.sin_zero));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(portno);
	client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sockmsg, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) 
	{
		perror("connection not esatablished");
		exit(1);
	}
	inet_ntop(AF_INET, (struct sockaddr *)&client_addr, ip, INET_ADDRSTRLEN);
	send(sockmsg, username, strlen(username),0); //send usrname to server
	
	printf("Connected to %s, start chatting...\n",ip);
	printf("If you want to send a message to the specific user,\nplease follow the format : \"to<[Usrname]>[YourMsg]\"\n");
	pthread_create(&recvt,NULL,recvmg,&sockmsg);
	while(fgets(msg,500,stdin) > 0) 
	{
		strcpy(res,username);
		strcat(res,":");
		strcat(res,msg);
		len = write(sockmsg,res,strlen(res));
		if(len < 0) 
		{
			perror("[message not sent]");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
		memset(res,'\0',sizeof(res));
	}
	pthread_join(recvt,NULL);
	close(sockmsg);

}