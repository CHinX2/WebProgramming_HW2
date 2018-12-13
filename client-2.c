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
	char acceptfile[3] = "\0";
	int len, len2;
	char buf[512];


	while((len = recv(their_sock,msg,500,0)) > 0) 
	{
		msg[len] = '\0';
		if(strcmp("File accept? (y/n) ", msg)==0)
		{
			fputs(msg,stdout);
			fgets(acceptfile,3,stdin);
			send(their_sock, acceptfile, 3, 0);
			if(strstr(acceptfile, "y")!=NULL)
			{
				FILE *fp = fopen("download.txt","w");
				bzero(buf, 512); 
				while((len2 = recv(their_sock,buf,512,0))>0)
				{
					buf[len] = '\0';
					fprintf(fp,"%s",buf);
					bzero(buf, 512); 
				}
				fclose(fp);
			}
			int c;
			while((c=getchar()!='\n')&&c!=EOF){}

		}
		else{
			fputs(msg,stdout);
		}
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
	printf("If you want to send a file,\nplease follow the format : \"to<[Usrname]><file>[filename]\"\n");
	
	pthread_create(&recvt,NULL,recvmg,&sockmsg);
	while(fgets(msg,500,stdin) > 0) 
	{
		
		filename = strstr(msg, filetrans_kw);
		
		if(filename!=NULL)
		{
			filename += 6;
			*(filename+strlen(filename)-1) = '\0';

			//printf("%s\n",filename);
			FILE *fp = fopen(filename, "r");
			if (fp == NULL) {
                    perror("open send file");
                    // exit(EXIT_FAILURE);
					fclose(fp);
                    int c;
					while((c=getchar()!='\n')&&c!=EOF){}
					break;
            }
			else{
				len = send(sockmsg,msg,strlen(msg),0);
				if(len < 0) 
				{
					perror("[message not sent]");
					exit(1);
				}
				char start[10];
				recv(sockmsg, start,10,0);
				if(strncmp("start",start,5)!=0)
				{
					printf("[File transfer deny]\n");
					fclose(fp);
				}
				else{
					bzero(buf, 512);
					int fr_block_sz = 0;
					while((fr_block_sz = fread(buf, sizeof(char), 512, fp))>0)
					{
						if(send(sockmsg, buf, 512, 0) < 0)
						{
							perror("File transfer fail\n");
							exit(1);
						}
						bzero(buf, 512);
					}
					printf("[transfer finish]\n");
					fclose(fp);
				}
			}

		}
		else if(strcmp("\n",msg)!=0){
			strcpy(res,username);
			strcat(res,":");
			strcat(res,msg);
			len = send(sockmsg,res,strlen(res),0);
			if(len < 0) 
			{
				perror("[message not sent]");
				exit(1);
			}
		}
		filename = NULL;
		memset(msg,'\0',sizeof(msg));
		memset(res,'\0',sizeof(res));
	}
	pthread_exit(NULL);
	close(sockmsg);

}