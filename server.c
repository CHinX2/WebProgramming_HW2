// Web Programming HW#2
// 405410034 Chinxy Yang
//---
// Target:
// (1) 查看有哪些使用者正在server線上					- done
// (2) 送出訊息給所有連到同一server的client				- done
// (3) 指定傳送訊息給正在線上的某個使用者				  - done
// (4) 也可以傳送檔案給另一個使用者，對方可決定是否接收		- fail
//---
// Last edited: 2018/12/11

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
int sentsuccess = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// show the online user name
void showusr(int curr)
{
	int i;
	char msg[500]="\0";

	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		sprintf(msg, " - %s\n",clients[i].usrname);
		if(send(curr,msg,strlen(msg),0) < 0)
		{
			perror("sending failure");
			continue;
		}
	}
	pthread_mutex_unlock(&mutex);
}

// sent msg to all the user
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

// send msg to the specific user
void sendtotar(char *msg, int curr, char *tarname)
{
	int i;
	char filetrans_kw[7] = "<file>";
	char filename[100] = "\0";
	char revbuf[512];
	sentsuccess = 0;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		if(strcmp(clients[i].usrname, tarname) == 0)
		{
			char* check = strstr(msg, filetrans_kw);
			if(check!=NULL)
			{
				//receive file from client
				strcpy(filename,check+6);
				filename[strlen(filename)-1]='\0';
				FILE *fp = fopen(filename, "r");
				if(fp == NULL)
					printf("File %s Cannot be opened file on server.\n", filename);
				else{
					bzero(revbuf, 512); 
					int fr_block_sz = 0;
					while((fr_block_sz = fread(revbuf, sizeof(char), 512, fp))>0)
					{
						if(send(clients[i].fd, revbuf, fr_block_sz, 0) < 0)
						{
							perror("File write failed on server.\n");
							exit(1);
						}
						bzero(revbuf, 512);
					}
					printf("Ok sent to client!\n");
					close(clients[i].fd);
					printf("Ok received from client!\n");
					fclose(fp);
				}
			}
			else{
				if(send(clients[i].fd,msg,strlen(msg),0) < 0) 
				{
					perror("sending failure");
					continue;
				}
			}
			sentsuccess++;
		}
	}
	pthread_mutex_unlock(&mutex);
	//can't find the target user, print error msg
	if(sentsuccess == 0 ) write(curr,"[Warning : User not found]\n",strlen("[Warning : User not found]\n"));
}

void *recvmg(void *sock)
{
	struct client_info cl = *((struct client_info *)sock);
	char msg[500];
	char sendtotar_kw[4] = "to<";
	char showusr_kw[7] = "<show>";
	char tarname[50]={'\0'};
	int len;
	int i;
	int j;
	while((len = recv(cl.sockno,msg,500,0)) > 0)
	{
		msg[len] = '\0';

		char* check = strstr(msg, showusr_kw); //check keyword "<show>"
		if(check != NULL)
		{
			showusr(cl.sockno);
		}
		else{
			check = strstr(msg, sendtotar_kw); //check keyword "to<"
			if(check != NULL)
			{
				//get target name
				char *tmp = check+3;
				int k=0;
				while(*tmp!='>')
				{
					tarname[k] = *tmp;
					tmp++;
					k++;
				}
				tmp++;
				*check = '\0';
				//get new msg
				char newmsg[500] = "[*]";
				strcat(newmsg, msg); 
				strcat(newmsg, tmp); 
				sendtotar(newmsg, cl.sockno, tarname);
			}
			else sendtoall(msg,cl.sockno);
			memset(msg,'\0',sizeof(msg));
		}
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

		//get username
		recv(client_sock,clients[n].usrname,50,0);

		//inform user of socket number
		printf("New connected : usrname\t%s\t, socket fd\t%d\t, ip\t%s\n", clients[n].usrname, client_sock, ip);
		cl.sockno = client_sock;
		strcpy(cl.ip, ip);
		clients[n].fd = client_sock; //add new socket to socket list
		n++; //max sock num +1

		pthread_create(&recvt, NULL, recvmg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}