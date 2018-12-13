#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void * send_msg(char *username, int socket_fd, struct sockaddr_in *address) {

  	char msg[512];
	FILE* fd;
  	char final_msg[612];
	
  	while (fgets(msg, 512, stdin) != NULL) {
      	memset(final_msg,'\0',strlen(final_msg)); // Clear final message buffer
     	if(strncmp("<quit>",msg,6) == 0)
		{
			strcpy(final_msg,msg);
			if(send(socket_fd, final_msg, strlen(final_msg)+1, 0)<0)
	  		{
				perror("[message not sent]");
				exit(1);
			}
			memset(msg,'\0',strlen(msg));
			exit(0);
		}
		
			strcat(final_msg, username);
			strcat(final_msg, " : ");
			strcat(final_msg, msg);

			if(send(socket_fd, final_msg, strlen(final_msg)+1, 0)<0)
			{
				perror("[message not sent]");
				exit(1);
			}
			memset(msg,'\0',strlen(msg));
		
    }
}


void *recvmg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[512];
	char buf[512];
	char acceptfile[3] = "\0";
	char filename[100] = "\0";
	int len, len2;
	FILE *fp, *fd;
	memset(msg,'\0',sizeof(msg));

	while((len = recv(their_sock,msg,512,0))>0)
	{
		if(strncmp("File transfer agree? (y/n) ",msg,27)==0)
		{
			fputs(msg,stdout);
			fgets(acceptfile, 3, stdin);
			send(their_sock, acceptfile,3,0);
			bzero(buf, 512);
			if(strncmp("y",acceptfile,1)==0)
			{
				fp = fopen("download.txt","w");
				len2 = recv(their_sock,buf,512,0);
				buf[len2]='\0';
				fprintf(fp,"%s",buf);
				bzero(buf, 512);
				fclose(fp);
			}
			
		}
		else if(strncmp("getfile",msg,7)==0)
		{
			send(their_sock,"getname",sizeof("getname"),0);
			len2 = recv(their_sock,filename,100,0);
			filename[len2-1]='\0';
			printf("send %s\n",filename);
			fd = fopen(filename,"r");
			if (fd == NULL) {
                    perror("open send file");
                    // exit(EXIT_FAILURE);
					//exit(1);
            }
			else{
				bzero(buf, 512);
				int fr_block_sz = 0;
				fgets(buf,512,fd);
				if(send(their_sock, buf, 512, 0) < 0)
				{
					perror("File transfer fail\n");
					exit(1);
				}
				bzero(buf, 512);
				printf("[transfer finish]\n");
				fclose(fd);
			}
		}
		else fputs(msg,stdout);
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
	pthread_t thread;
	char msg[512];
	char username[100];
	char ip[INET_ADDRSTRLEN];
	int len;
	char buf[512];
	char filetrans_kw[7] = "<file>";
	char *filename;

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
	printf("If you want to send a file,\nplease follow the format : \"file<[Usrname]>[filename]\"\n");
	printf("If you want to leave,\nplease follow the format : \"<quit>\"\n\n");
	
	pthread_create(&thread,NULL,recvmg,&sockmsg);
	send_msg(username,sockmsg,&client_addr);

}