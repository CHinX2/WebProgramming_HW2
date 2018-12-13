// Web Programming HW#2
// 405410034 Chinxy Yang
//---
// Target:
// (1) 查看有哪些使用者正在server線上					- done
// (2) 送出訊息給所有連到同一server的client				- done
// (3) 指定傳送訊息給正在線上的某個使用者				  - done
// (4) 也可以傳送檔案給另一個使用者，對方可決定是否接收		- done
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

struct nameset clients[10];

int listenfd;
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

// send file to the specific user
void filetotar(char *filename, int curr, char *tarname)
{
	int i;
	char buf[512];
	char agree[3];
	char ask[50];
	sentsuccess = 0;

	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		if(strcmp(clients[i].usrname, tarname) == 0)
		{
			send(clients[i].fd,"File transfer agree? (y/n) ",strlen("File transfer agree? (y/n) "),0);
			recv(clients[i].fd,agree,3,0);
			if(strncmp("y",agree,1)==0)
			{
				send(curr, "getfile", strlen("getfile"),0);
				recv(curr,ask,50,0);
				printf("send %s\n",filename);
				
				send(curr, filename, strlen(filename),0);
				recv(curr,buf,512,0);
				if(send(clients[i].fd,buf,strlen(buf),0) < 0)
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

// send msg to the specific user
void sendtotar(char *msg, int curr, char *tarname)
{
	int i;
	sentsuccess = 0;

	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++)
	{
		if(strcmp(clients[i].usrname, tarname) == 0)
		{
			if(send(clients[i].fd,msg,strlen(msg),0) < 0) 
			{
				perror("sending failure");
				continue;
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
	char tarname[50]={'\0'};
	char* tar;
	int retval;
	int len;
	int i;
	int j;

	while(1)
	{
		if((len = recv(cl.sockno,msg,512,0)) > 0)
		{
			msg[len] = '\0';

			if(strncmp("<quit>",msg,6)==0)
			{
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
						close(cl.sockno);
					}
				}
				n--; //max sock num -1	
				pthread_mutex_unlock(&mutex);
			}
			else if(strstr(msg,"<show>")!=NULL)
			{
				showusr(cl.sockno);
			}
			else if((tar=strstr(msg, "to<"))!=NULL)
			{
				//get target name
				char *tmp = tar+3;
				int k=0;
				while(*tmp!='>')
				{
					tarname[k] = *tmp;
					tmp++;
					k++;
				}
				tmp++;
				*tar='\0';
				//get new msg
				char newmsg[500] = "[*]";
				strcat(newmsg, msg); 
				strcat(newmsg, tmp); 
				sendtotar(newmsg, cl.sockno, tarname);
			}
			else if((tar=strstr(msg, "file<"))!=NULL)
			{
				//get target name
				char *tmp = tar+5;
				int k=0;
				while(*tmp!='>')
				{
					tarname[k] = *tmp;
					tmp++;
					k++;
				}
				tmp++;
				*tar='\0';
				
				filetotar(tmp, cl.sockno, tarname);
			}
			else sendtoall(msg,cl.sockno);
			memset(msg,'\0',sizeof(msg));
		}
	}
}

void quit(void)
{
	char msg[10];
    while(1)
    {
        scanf("%s",msg);
        if(strcmp("quit",msg)==0)
        {
            printf("Byebye...\n");
            close(listenfd);
            exit(0);
        }
    }
}

int main(int argc,char *argv[])
{
	struct sockaddr_in master_addr,client_addr;
	int master_sock;
	int client_sock;
	socklen_t client_addr_size;
	int portno = 9999;
	pthread_t thread;
	char msg[512];
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
	if(master_sock < 0)
	{
		printf("Socket created fail");
		exit(1);
	}
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
	//pending connection limit : 10
	if(listen(master_sock,10) != 0) 
	{
		perror("listening unsuccessful");
		exit(1);
	}

	printf("Waiting for connections...\n");
	pthread_create(&thread,NULL,(void*)(&quit),NULL);

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
		recv(client_sock,clients[n].usrname,100,0);

		//inform user of socket number
		printf("New connected : usrname\t%s\t, socket fd\t%d\t, ip\t%s\n", clients[n].usrname, client_sock, ip);
		cl.sockno = client_sock;
		strcpy(cl.ip, ip);
		clients[n].fd = client_sock; //add new socket to socket list
		n++; //max sock num +1

		pthread_create(&thread, NULL, recvmg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}