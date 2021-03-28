#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<ctype.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#define MAXLEN 100
#define MAXARGLEN 50
char read_buf[MAXLEN];
char *arg_list[MAXARGLEN];
void get_arguments(char *input);
int main(int argc,char *argv[])
{
  int my_id=atoi(argv[1]);
  int Status;
  int listen_fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  struct sockaddr_in server_addr,cli_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(1024+my_id);
  inet_pton(AF_INET,argv[2],&server_addr.sin_addr);
  bind(listen_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
  listen(listen_fd,2);
  int cli_fd;
  
  printf("Server Running\n waiting for connection\n");
  socklen_t clen=sizeof(cli_addr);
  cli_fd=accept(listen_fd,(struct sockaddr *)&cli_addr,&clen);
  printf("Connected\n");  
    while(1)
    { 
      int r=read(cli_fd,read_buf,MAXLEN);
      read_buf[r]='\0';
      if(r>0)
      { int p[2];
        pipe(p); 
        int pd=fork();
        if(pd==0)
        {
          close(1);
          dup(p[1]);
          close(p[0]);
          get_arguments(read_buf);
          execvp(arg_list[0],arg_list);
          printf("error in execvp\n");
          printf("input=%s\n",read_buf);
          exit(0);

        }
        else
          { close(p[1]);
            waitpid(pd,&Status,0);
            r=read(p[0],read_buf,MAXLEN);
            //printf("read_buf=%s r=%d\n",read_buf,r);
            write(cli_fd,read_buf,r);
          }
      }
      else
      printf("unable to read\n");
    }
    
}
void get_arguments(char *input)
{
  int ptr=0;
  while(*input)
  { 
    while(*input&&isspace(*input))
    ++input;
    arg_list[ptr++]=input;
    while(*input&&!isspace(*input))
    {
      ++input;
    }
    if(!(*input))
    break;
    *input='\0';
    ++input;
  }
  arg_list[ptr]=NULL;
}

