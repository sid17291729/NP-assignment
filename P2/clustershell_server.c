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
#include<errno.h>
#define MAXLEN 100
#define MAXARGLEN 50
#define MAX_MSG_LEN 1000
char read_buf[MAX_MSG_LEN];
char ACK[10];
char *arg_list[MAXARGLEN];
char *node_ip[MAXLEN];
int socket_fd[MAXLEN];
int active_connections[MAXLEN];
struct sockaddr_in node_address[MAXLEN];
int MAIN_FD;

void get_arguments(char *input);
void parse_ip(FILE *fp);
char * has_pipe(int len,char *cmd);
int cd_exec(char *input);

int main(int argc,char *argv[])
{ 
  int my_id=atoi(argv[1]);
  int Status;
  FILE *fp=fopen(argv[3],"r");
  
  parse_ip(fp);

  int listen_fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  struct sockaddr_in server_addr,cli_addr,main_addr;
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(1024+my_id);
  inet_pton(AF_INET,argv[2],&server_addr.sin_addr);

  bind(listen_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
  listen(listen_fd,2);
  
  int cli_fd;
  
  printf("Server Running \n\n");
  socklen_t clen=sizeof(cli_addr);
  int reciever_pid;
  MAIN_FD=0;
  while(1)//creator process
  {
    printf("creator process waiting for connection\n");
    memset((char*)&cli_addr,0,clen);
    cli_fd=accept(listen_fd,(struct sockaddr *)&cli_addr,&clen);
    printf("connected\n");
    if(!MAIN_FD)
    {
      MAIN_FD=cli_fd;
      main_addr=cli_addr;
      printf("connected to the main node\n");
    }
    printf("\n");
    reciever_pid=fork();
    
    if(reciever_pid==0)
      break;
    else
    { if(MAIN_FD!=cli_fd)
        close(cli_fd);
    }
  }
  
  if(MAIN_FD==cli_fd)
  {  
    while(1)//principal process
    { printf("principal process waiting for commands from the main node\n");
      int r=read(MAIN_FD,read_buf,MAX_MSG_LEN);
      read_buf[r]='\0';
      printf("recieved command=%s from main node\n",read_buf);
      if(strcmp(read_buf,"exit")==0)
       { close(MAIN_FD);
         exit(0);
        }
      
      int fd_set=MAIN_FD;
      char *pipe_pos=NULL;
      pipe_pos=has_pipe(strlen(read_buf),read_buf);
      char *cmd_send=NULL;
      int reciever_node=0;
      if(pipe_pos)
      {
        int i=0;
        while(pipe_pos[i]!='.')
        ++i;
        pipe_pos[i]='\0';
        reciever_node=atoi(pipe_pos+2);
        fd_set=socket_fd[reciever_node];
        cmd_send=pipe_pos+i+1;
        
        *pipe_pos='\0';
        int execute_pid=fork();
        if(execute_pid==0)
        {  printf("connecting with node %d\n",reciever_node);
           printf("...\n");
          int s=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
          fd_set=s;
          s=connect(s,(struct sockaddr *)&node_address[reciever_node],sizeof(node_address[reciever_node]));
          if(s!=0)
          {
           printf("unable to connect to node %d\n",reciever_node);
           printf("errnor no.=%d\n",errno);
           printf("Error description is : %s\n",strerror(errno));
           exit(0);
          }
          printf("connected with node %d\n",reciever_node);
          r=read(fd_set,ACK,2);
          ACK[r]='\0';
          write(fd_set,cmd_send,strlen(cmd_send));
          r=read(fd_set,ACK,2);
          ACK[r]='\0';
          printf("executing %s\n",read_buf);
          close(1);
          dup(fd_set);
          
          get_arguments(read_buf);
        
          execvp(arg_list[0],arg_list);
          printf("error in execvp\n");
       
          exit(0);
        }
        else
        { 
          
          waitpid(execute_pid,&Status,0);
          
          printf("command executed and connection\n\n");
          
        }
        continue;

      }
      
      if(cd_exec(read_buf))
       { printf("executing cd command\n\n");
         write(MAIN_FD,"directory changed",18);
         continue;
       }

      int execute_pid=fork();
      if(execute_pid==0)
      { printf("executing %s\n",read_buf);
        close(1);
        dup(MAIN_FD);
        get_arguments(read_buf);
        
        execvp(arg_list[0],arg_list);
        printf("error in execvp\n");
        write(MAIN_FD,"Error_in execvp",50);
      }
      else
      {
        waitpid(execute_pid,&Status,0);
        printf("Command executed\n\n");
      }
      
    }
    
  }
  else//side process
  { printf("side process connected\n");
    ACK[0]='1';
    ACK[1]='\0';
    char cmd[MAXARGLEN];
    write(cli_fd,ACK,1);
    int cmd_len=read(cli_fd,cmd,MAXARGLEN);
    cmd[cmd_len]='\0';
    printf("Command recieved=%s\n",cmd);
    
    
   
    write(cli_fd,ACK,1);
    int msg_len=read(cli_fd,read_buf,MAX_MSG_LEN);
    read_buf[msg_len]='\0';
    printf("data recieved\n");
    
    int p[2];
    pipe(p);
    write(p[1],read_buf,msg_len);


    char *pipe_pos=NULL;
    pipe_pos=has_pipe(strlen(cmd),cmd);
    char *cmd_send=NULL;
    int reciever_node=0;
    
    if(pipe_pos)
    {
      int i=0;
      while(pipe_pos[i]!='.')
       ++i;
      
      pipe_pos[i]='\0';
      reciever_node=atoi(pipe_pos+2);
      int fd_set=socket_fd[reciever_node];
      cmd_send=pipe_pos+i+1;

      int execute_pid=fork();
      if(execute_pid==0)
      { close(cli_fd);
        printf("connecting with node %d\n\n",reciever_node);
        printf("...\n");
        int s=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        fd_set=s;
        s=connect(s,(struct sockaddr *)&node_address[reciever_node],sizeof(node_address[reciever_node]));
        if(s!=0)
        {
         printf("unable to connect to node %d\n",reciever_node);
           printf("errnor no.=%d\n",errno);
           printf("Error description is : %s\n",strerror(errno));
           exit(0);
         
        }
        else
        printf("connected with node %d\n",reciever_node);
        int r=read(fd_set,ACK,2);
        ACK[r]='\0';
        write(fd_set,cmd_send,strlen(cmd_send));
        r=read(fd_set,ACK,2);
        ACK[r]='\0';
        printf("executing %s\n",read_buf);
        close(1);
        dup(fd_set);
         
        get_arguments(read_buf);
        
        execvp(arg_list[0],arg_list);
        printf("error in execvp\n");
       
        exit(0);

      }
      else
        {
          waitpid(execute_pid,&Status,0);
          
          printf("command executed, data sent and connection closed\n\n");
          
        }
        close(cli_fd);
        exit(0);
    }

    
    int execute_pd=fork();
    if(execute_pd==0)
    {  printf("executing command %s\n",cmd);
      close(0);
      dup(p[0]);
      close(p[1]);
      close(1);
      dup(MAIN_FD);
      get_arguments(cmd);
      
      
      execvp(arg_list[0],arg_list);
      printf("execvp error\n");
      exit(0);
    }
    close(p[1]);
    close(p[0]);
    waitpid(execute_pd,&Status,0);
    printf("command executed and data sent\n");
    close(cli_fd);
    exit(0);
    
  } 
}
int cd_exec(char *input)
{ int ret=0;
  if(input[0]=='c'&&input[1]=='d')
  { ret=1;
      int i=2;
      while(input[i]==' ')
      ++i;
      chdir(input+i);
  }
  
  return ret;
}
char * has_pipe(int len,char *cmd)
{
  for(int i=0;i<len;++i)
    {if(cmd[i]=='|'&&cmd[i+1]=='n'&&cmd[i+2]>='0'&&cmd[i+2]<='9')
      return cmd+i;
    }
  return NULL;
}
void get_arguments(char *input)
{
  int ptr=2;
  arg_list[0]="sh";
  arg_list[1]="-c";
  
  arg_list[ptr++]=input;
  arg_list[ptr]=NULL;
}
void parse_ip(FILE *fp)
{    
    while(fgets(read_buf,MAXLEN,fp)>0)
    {   
        int l=strlen(read_buf);
        
        if(read_buf[l-1]=='\n'||read_buf[l-1]=='\r')
        {
            read_buf[l-1]='\0';
            --l;
            if(read_buf[l-1]=='\n'||read_buf[l-1]=='\r')
            {
                read_buf[l-1]='\0';
                --l;
            }
        }
        int i=l-1;
        while(read_buf[i]!=' ')
        --i;
        read_buf[i]='\0';
        ++i;
        
      int index=atoi(read_buf);
      node_ip[index]=(char *)malloc(sizeof(char)*(l+1));
      strcpy(node_ip[index],read_buf+i);
      
      node_address[index].sin_family=AF_INET;
      inet_pton(AF_INET,node_ip[index],&node_address[index].sin_addr);
      node_address[index].sin_port=htons(1024+i);
    }
    
}