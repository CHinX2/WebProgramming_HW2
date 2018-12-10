#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct client_info {
	int sockno;
	char ip[INET_ADDRSTRLEN];
};

struct nameset {
	int fd;
	char usrname[50];
};

struct nameset clients[100];

int n = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void sendtoall(char *msg,int curr)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		if(clients[i].fd != curr)
		{
			if(send(clients[i].fd,msg,strlen(msg),0) < 0) 
			{
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}

void sendtotar(char *msg, int curr, char *tarname)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		if(clients[i].fd != curr && strcmp(clients[i].usrname,tarname)==0)
		{
			if(send(clients[i].fd,msg,strlen(msg),0) < 0)
			{
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}

void *recvmg(void *sock)
{
	struct client_info cl = *((struct client_info *)sock);
	char msg[500];
	char sendtotar_kw[4] = "to<";
	char tarname[50]={'\0'};
	int len;
	int i;
	int j;
	while((len = recv(cl.sockno,msg,500,0)) > 0)
	{
		msg[len] = '\0';

		char* check = strstr(msg, sendtotar_kw);
		if(check != NULL)
		{
			char *tmp = check+3;
			int k=0;
			while(*tmp!='>')
			{
				tarname[k] = *tmp;
				tmp++;
				k++;
			}
			tmp++;
			sendtotar(tmp, cl.sockno, tarname);
		}
		else sendtoall(msg,cl.sockno);
		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",cl.ip);
	for(i = 0; i < n; i++) 
	{
		if(clients[i].fd == cl.sockno) 
		{
			j = i;
			while(j < n-1)
			{
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--; //max sock num -1
	pthread_mutex_unlock(&mutex);
}
int main(int argc,char *argv[])
{
	struct sockaddr_in master_addr,client_addr;
	int master_sock;
	int client_sock;
	socklen_t client_addr_size;
	int portno;
	pthread_t sendt,recvt;
	char msg[500];
	int len;
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];
	
	if(argc > 2)
	{
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[1]); //port number 

	//creat a master socket
	master_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(master_addr.sin_zero,'\0',sizeof(master_addr.sin_zero));
	master_addr.sin_family = AF_INET;
	master_addr.sin_port = htons(portno);
	master_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//accept the incoming connection
	client_addr_size = sizeof(client_addr);

	//bind the master socket to port
	if(bind(master_sock, (struct sockaddr *)&master_addr, sizeof(master_addr)) != 0) 
	{
		perror("binding unsuccessful");
		exit(1);
	}

	printf("Listening on port %d ...\n",portno);
	//pending connection limit : 5
	if(listen(master_sock,5) != 0) 
	{
		perror("listening unsuccessful");
		exit(1);
	}


	printf("Waiting for connections...\n");

	while(1)
	{
		if((client_sock = accept(master_sock, (struct sockaddr *)&client_addr, &client_addr_size)) < 0) 
		{
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&client_addr, ip, INET_ADDRSTRLEN);
		recv(client_sock,clients[n].usrname,50,0);
		//inform user of socket number
		printf("New user connected : socket fd %d, ip %s, port %d\n", client_sock, ip, portno);
		cl.sockno = client_sock;
		strcpy(cl.ip, ip);
		clients[n].fd = client_sock; //add new socket to socket list
		n++; //max sock num +1
		pthread_create(&recvt, NULL, recvmg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}