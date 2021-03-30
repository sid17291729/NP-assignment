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
#define MAX_MSG_LEN 1000
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

char *node_ip[MAXLEN];
char read_buf[MAX_MSG_LEN];
int socket_fd[MAXLEN];

int active_conections[MAXLEN];
struct sockaddr_in node_address[MAXLEN];
void parse_ip(FILE* fp);
char *get_input();
int is_local(char *input);
void execute_local(char *input);
void connect_nodes();
void execute_node(char * input);
char * has_pipe(int len,char *cmd);
int cd_exec(char *input);
int main(int argc, char *argv[])
{
    
    char file_name[MAXLEN];
    
    FILE *fp=fopen(argv[1],"r");
    
    if(fp<0)
    {
        printf("error in opening file\n");
        exit(0);
    }
    
    parse_ip(fp);
    connect_nodes();
    char *input;
    int Status;
    
    while(1)
    {   for(int i=0;i<MAX_MSG_LEN;++i)
        read_buf[i]='\0';
        input=get_input();
        
        if(is_local(input))
        {   if(cd_exec(input))
            {   printf("cd executed\n");
                continue;
            }
            int p=fork();
            if(p==0)
             break;
            else
             waitpid(p,&Status,0);
        }
        else
        {
          int p=fork();
          if(p==0)
          {
              execute_node(input);
              exit(0);
          }  
          else
          {
              waitpid(p,&Status,0);
          }
        }

    }
    execute_local(input);
    
}

void execute_node(char * input)
{ 
    int i=0;
    int wait_node=0;
    if(has_pipe(strlen(input),input))
    {
        int l=strlen(input);
        
        for(int j=l-1;j>=0;--j)
        {
            if(input[j]=='|'&&input[j+1]=='n'&&input[j+2]>='0'&&input[j+2]<='9')
            {
                int k1=j+2;
                int k=k1;
                while(input[k]&&input[k]!='.')
                ++k;

                input[k]='\0';
                wait_node=atoi(input+k1);
                input[k]='.';
                break;
            }
        }
    }
    while(input[i]&&input[i]!='.')
    ++i;
    if(input[i-1]=='*')
    {   char cmd[MAXARGLEN];
        strcpy(cmd,input+i+1);
        for(int j=0;j<MAXLEN;++j)
        {
            if(active_conections[j])
            {
                int message_len=strlen(cmd);
                write(socket_fd[j],cmd,message_len);
                int r=read(socket_fd[j],read_buf,MAXLEN);
                read_buf[r]='\0';   
                printf("result from node %d =%s\n",j,read_buf);
            }
        }
        return ;
    }
    input[i]='\0';
    int node= atoi(input+1);
    if(wait_node==0)
        wait_node=node;
    if(active_conections[node]==0)
    {
        printf("connection to %d not active,restart to connect\n",node);
        exit(0);
    }
    ++i;
    
    int message_len=strlen(input+i);
    
    
    write(socket_fd[node],input+i,message_len);
    
    
    int r=read(socket_fd[wait_node],read_buf,MAXLEN);
    read_buf[r]='\0';   
    printf("result=%s\n",read_buf);
  
  

}
char * has_pipe(int len,char *cmd)
{
  for(int i=0;i<len;++i)
    {if(cmd[i]=='|'&&cmd[i+1]=='n'&&cmd[i+2]>='0'&&cmd[i+2]<='9')
      return cmd+i;
    }
  return NULL;
}
void connect_nodes()
{
  for(int i=1;i<MAXLEN;++i)
  {
      if(!node_ip[i])
      continue;
     int s=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
     socket_fd[i]=s;
     node_address[i].sin_family=AF_INET;
     inet_pton(AF_INET,node_ip[i],&node_address[i].sin_addr);
     node_address[i].sin_port=htons(1024+i);
     s=connect(socket_fd[i],(struct sockaddr *)&node_address[i],sizeof(node_address[i]));
    if(s==0)
    {
        active_conections[i]=1;
        printf("connected for node %d\n",i);   
    }
    else
        active_conections[i]=0;
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
void execute_local(char *input)
{   

    int Status;
    pid_t pd;
    
    int arg_ptr=2;
    char *nex_tok=input;
    char *arg_list[MAXLEN];
    arg_list[0]="sh";
    arg_list[1]="-c";
    
    arg_list[arg_ptr++]=input;
    arg_list[arg_ptr]=NULL;
    execvp(arg_list[0],arg_list);
    printf("error execvp\n");
    exit(0);
}
int is_local(char *input)
{
    if(input[0]=='n')
    {
        int i=1;
        if(input[1]=='*')
            return 0;
        while(input[i]>='0'&&input[i]<='9')
        ++i;

        if(input[i]=='.')
        return 0;
        
        else
        return 1;
    }
    return 1;
}
void print_node_ip_list()
{
    for(int i=0;i<MAXLEN;++i)
    {
        if(node_ip[i])
        printf("node=%d, ip=%s\n",i,node_ip[i]);
    }
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
    }
    print_node_ip_list();
}

char*get_input()
{
    fputs(ANSI_COLOR_GREEN"Cluster_Shell$ "ANSI_COLOR_RESET,stdout);
    fflush(stdout);
    fgets(read_buf,MAXLEN,stdin);
    
    
    int ptr_end = strlen(read_buf)-1;
    int ptr_strt = 0;
    while(isspace(read_buf[ptr_end])) read_buf[ptr_end--]='\0';
    while(isspace(read_buf[ptr_strt])) ptr_strt++;
    return read_buf+ptr_strt;
}
