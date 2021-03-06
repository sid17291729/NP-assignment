#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<ctype.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MAXLEN 1024
#define MAXARGLEN 100
enum MODE{NONE, PIPE, REDIRECT_OUT, REDIRECT_IN, REDIRECT_APPEND, PIPE_DOUBLE,PIPE_TRIPLE};

char read_buf[MAXLEN];
char *arg_list[MAXARGLEN];
char *cmd_list[MAXLEN];
int execute(char*input);
char*get_input();
int isDelim(char c);
char*set_mode(char*,int*);
int has_pipe(char *nex_tok);
int is_sc(char *cmd);
void handle_sc(char *cmd);
void sig_handle(int sig_no);
int is_bgprocess();
int main()
{   signal(SIGINT,sig_handle);
    fprintf(stdout,"Shell Running\n");
    char*input;
    int Status=0;
    int is_bgp=0;
    while(1)
    {
        input = get_input();
        is_bgp=is_bgprocess(input);
        if(strcmp(input,"exit")==0)
        exit(0);
        if(is_sc(input))
        {  
            handle_sc(input);
            continue;
        }
        int p=fork();
        if(p==0){
            setpgid(0,0);
            break;
        };
        
        if(!is_bgp){ 
            waitpid(p,&Status,0);
            tcsetpgrp(STDIN_FILENO,getpgid(0));
            printf("process pid=%d, return status=%d\n",p,Status);       
        }
        else{
            tcsetpgrp(STDIN_FILENO,getpgid(0));
        }
        
    }
    //printf("input=%s\n",input);
    execute(input);
}


int execute(char*input){
    int Status;
    pid_t pd;
    int mode=NONE;
    int arg_ptr=0;
    char *nex_tok=input;
    
    //fix parsing
    while(*nex_tok){
        arg_list[arg_ptr++]=nex_tok;
        while(*nex_tok && !isDelim(*nex_tok) && !isspace(*nex_tok)){
            nex_tok++;
        }
        while(isspace(*nex_tok)){
            *nex_tok = '\0';
            nex_tok++;
        }
        if(isDelim(*nex_tok)){
            nex_tok = set_mode(nex_tok, &mode);
            break;
        }
    }
    arg_list[arg_ptr]=NULL;
    //printf("pid=%d ,mode=%d\n",getpid(),mode);
    //printf("nex_tok=%c\n",*nex_tok);
    int pw[2];
    int st;
    int filename_fd;
    switch(mode){
        case NONE:   
            
            pd=fork();
            if(pd==0){
                
                execvp(arg_list[0],arg_list);
                exit(0);
            }
            waitpid(pd,&st,0);
            break;
        
        case PIPE:
            
            pipe(pw);
            
            pd=fork();
            if(pd==0)
            {    
                
                close(1);
                dup(pw[1]);
                close(pw[0]);
                execvp(arg_list[0],arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
                    
               
                
            }
            else
            {
                waitpid(pd,&st,0);
                close(0);
                dup(pw[0]);
                close(pw[1]);
                execute(nex_tok);
            }
            break;
        
        case REDIRECT_OUT:
            //open fd for filename
            while(isspace(*nex_tok)){
                *nex_tok = '\0';
                nex_tok++;
            }
            filename_fd = open(nex_tok, O_CREAT|O_TRUNC|O_WRONLY);
            if(filename_fd<0){ perror("FILE OPENING"); return -1; }
            pd = fork();
            if(pd==0){
                close(1);
                dup(filename_fd);
                execvp(arg_list[0], arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
            }
            else{
                waitpid(pd,&st,0);
                close(filename_fd);
            }
            break;

        case REDIRECT_APPEND:
            while(isspace(*nex_tok)){
                *nex_tok = '\0';
                nex_tok++;
            }
            filename_fd = open(nex_tok, O_CREAT|O_APPEND|O_WRONLY);
            if(filename_fd<0){ perror("FILE OPENING"); return -1; }
            pd = fork();
            if(pd==0){
                close(1);
                dup(filename_fd);
                execvp(arg_list[0], arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
            }
            else{
                waitpid(pd,&st,0);
                close(filename_fd);
            }
            break;

        case REDIRECT_IN:
            // need to handle file not exists error properly
            
            while(isspace(*nex_tok)){
                *nex_tok = '\0';
                nex_tok++;
            }
            int pp=has_pipe(nex_tok);
            if(pp)
            pipe(pw);
            

            char *temp_nex_tok=nex_tok;
            while(*temp_nex_tok&&!isspace(*temp_nex_tok))
            ++temp_nex_tok;
            *temp_nex_tok='\0';
           
            filename_fd = open(nex_tok, O_RDONLY);
            ++temp_nex_tok;
            //nex_tok=temp_nex_tok;
            
            pd = fork();
            if(pd==0){
                close(0);
                dup(filename_fd);
                if(pp)
                {
                    close(1);
                    dup(pw[1]);
                    close(pw[0]);
                    //printf("pid=%d pipe used\n",getpid());
                }
                execvp(arg_list[0], arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
            }
            else{
                if(pp)
                {
                    close(0);
                    dup(pw[0]);
                    close(pw[1]);
                }
                waitpid(pd,&st,0);
                close(filename_fd);
                if(pp)
               { while(*nex_tok!='|')
                    nex_tok++;
                    ++nex_tok;
                     
                    execute(nex_tok);
                }

            }
            break;

        case PIPE_DOUBLE:
            printf("DOUBLE\n");
            pipe(pw);
            int p1[2];
            int p2[2];
            pd=fork();
            if(pd==0)
            {
              close(1);
              dup(pw[1]);
              close(pw[0]);
              execvp(arg_list[0], arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
            }
            else
            {
                waitpid(pd,&Status,0);
                pipe(p1);
                pipe(p2);
                close(pw[1]);
                char c;
                while(read(pw[0],&c,1)>0)
                {    //printf("%c",c);
                    write(p1[1],&c,1);
                    write(p2[1],&c,1);
                }
                //printf("\n");
                close(pw[0]);
                close(p1[1]);
                close(p2[1]);
                char *temp_nex_tok=nex_tok;
                while(*temp_nex_tok!=',')
                ++temp_nex_tok;

                *temp_nex_tok='\0';
                ++temp_nex_tok;
                while(isspace(*temp_nex_tok))
                ++temp_nex_tok;
                int pd1=fork();
                if(pd1==0)
                {
                    close(0);
                    dup(p1[0]);
                    execute(nex_tok);
                }
                else
                {   
                    pd=fork();
                    if(pd==0)
                    {nex_tok=temp_nex_tok;
                    close(0);
                    dup(p2[0]);
                    execute(nex_tok);}
                    else
                    {waitpid(pd,&Status,0);
                    waitpid(pd1,&Status,0);
                    }
                }
                
            }
            
            
            break;

        case PIPE_TRIPLE:
            printf("TRIPLE\n");
            pipe(pw);
            // int p1[2];
            // int p2[2];
            int p3[2];
            pd=fork();
            if(pd==0)
            {
              close(1);
              dup(pw[1]);
              close(pw[0]);
              execvp(arg_list[0], arg_list);
                printf("error in executing %s\n",arg_list[0]);
                exit(0);
            }
            else
            {
                waitpid(pd,&Status,0);
                pipe(p1);
                pipe(p2);
                pipe(p3);
                close(pw[1]);
                char c;
                while(read(pw[0],&c,1)>0)
                {    //printf("%c",c);
                    write(p1[1],&c,1);
                    write(p2[1],&c,1);
                    write(p3[1],&c,1);
                }
                close(pw[0]);
                close(p1[1]);
                close(p2[1]);
                close(p3[1]);
                char *temp_nex_tok=nex_tok;
                while(*temp_nex_tok!=',')
                ++temp_nex_tok;
                *temp_nex_tok='\0';
                ++temp_nex_tok;

                char* nex_tok_c1;
                while(isspace(*temp_nex_tok))
                ++temp_nex_tok;

                nex_tok_c1=temp_nex_tok;
                while(*temp_nex_tok!=',')
                ++temp_nex_tok;
                *temp_nex_tok='\0';
                ++temp_nex_tok;

                 while(isspace(*temp_nex_tok))
                ++temp_nex_tok;
                char *nex_tok_c2=temp_nex_tok;

                int pd1=fork();
                if(pd1==0)
                {
                    close(0);
                    dup(p1[0]);
                    execute(nex_tok);
                }
                else
                {
                    int pd2=fork();

                    if(pd2==0)
                {
                    close(0);
                    dup(p2[0]);
                    execute(nex_tok_c1);
                }
                else
                {
                    pd=fork();
                    if(pd==0)
                    {
                     close(0);
                    dup(p3[0]);
                    execute(nex_tok_c2);   
                    }
                    else
                    {
                        waitpid(pd,&Status,0);
                        waitpid(pd1,&Status,0);
                        waitpid(pd2,&Status,0);
                    }
                }

                }

            }
            break;

        default: printf("ERROR. THIS REDIRECTION IS NOT SUPPORTED YET\n");
    }
    
    // if(pd){ 
    //     printf("pid=%d waiting for pd=%d\n",getpid(),pd);  
    //     waitpid(pd,&Status,0);
    //     printf("process pid=%d, return status=%d\n",pd,Status);
    // }
    // else
    // printf("not waiting pid=%d,pd=%d\n",getpid(),pd);
    return pd;
}

int has_pipe(char *nex_tok)
{    char *temp=nex_tok;
    while(*temp!='\0')
    {
        if(*temp=='|')
        return 1;
        //printf("temp=%c\n",*temp);
        temp++;
    }
    return 0;
}
char* set_mode(char*nex_tok, int*mode){
    if(*nex_tok=='|'){
        *mode = PIPE;
        *nex_tok = '\0';
        nex_tok++;
        if(*nex_tok=='|'){
            *mode = PIPE_DOUBLE;
            *nex_tok = '\0';
            nex_tok++;
            if(*nex_tok=='|'){
                *mode = PIPE_TRIPLE;
                *nex_tok = '\0';
                nex_tok++;
            }
        }
    }
    else if (*nex_tok=='>'){
        *mode = REDIRECT_OUT;
        *nex_tok = '\0';
        nex_tok++;
        if(*nex_tok=='>'){
            *mode = REDIRECT_APPEND;
            *nex_tok='\0';
            nex_tok++;
        }
    }
    else if(*nex_tok=='<'){
        *mode = REDIRECT_IN;
        *nex_tok='\0';
        nex_tok++;
    }

    while(isspace(*nex_tok)){
        *nex_tok = '\0';
        nex_tok++;
    }
    return nex_tok;
}
int not_char(char c)
{
    if(!(c>='a'&&c<='z'||c>='A'&&c<='Z'))
    return 1;
    return 0;
}
char*get_input(){
    fputs(ANSI_COLOR_GREEN"MyShell$ "ANSI_COLOR_RESET,stdout);
    fflush(stdout);
    fgets(read_buf,MAXLEN,stdin);
    read_buf[MAXLEN-1] = '\0';
    
    int ptr_end = strlen(read_buf)-1;
    int ptr_strt = 0;
    while(isspace(read_buf[ptr_end])) read_buf[ptr_end--]='\0';
    while(isspace(read_buf[ptr_strt])||not_char(read_buf[ptr_strt])) ptr_strt++;
    return read_buf+ptr_strt;
}

int isDelim(char c){
    if(c=='|'||c=='<'||c=='>') return 1;
    return 0;
}
void add_command(char * cmd,int ind){
 if(ind>=MAXLEN)
 {printf("Index out of bounds. max index value=%d\n",MAXLEN-1);
    return;
 }
 int l=strlen(cmd);
 cmd_list[ind]=(char*)malloc(sizeof(char)*(l+1));
 strcpy(cmd_list[ind],cmd);
 return;
}
void delete_command(int ind)
{
 if(ind>=MAXLEN)
 {printf("Index out of bounds. max index value=%d\n",MAXLEN-1);
    return;
 }
 cmd_list[ind]=NULL;
 return;   
}
int is_sc(char *cmd)
{   char *temp=cmd;
    if(temp[0]=='s'&&temp[1]=='c')
    return 1;

    return 0;
}
void handle_sc(char *cmd)
{   char *temp=cmd; 
    int n=strlen(cmd);
    while(*temp&&*temp!='-')
    ++temp;
    ++temp;
    int add=0;
    if(*temp=='i')
    add=1;
    
    temp+=2;
    char* index_string;
    index_string=temp;
    
    while(!isspace(*temp))
    ++temp;
    *temp='\0';
    ++temp;
    while(isspace(*temp))
    ++temp;
    int index=atoi(index_string);
    
    if(add)
    add_command(temp,index);
    else
    delete_command(index);

}
void sig_handle(int sig_no)
{   printf("\n");
    fflush(stdout);
    fflush(stdin);
    int ind;
    int Status;
    int pd=fork();
    if(pd==0)
    {
    while(1)
    {    fgets(read_buf,MAXLEN,stdin);
         fflush(stdin);
         ind=atoi(read_buf);
        if(ind==-1)
        exit(0);
        if(!cmd_list[ind])
        {
            printf("no command at the index\n");
            fflush(stdout);
        
        }
        else
            break; 
    }
    
    
        execute(cmd_list[ind]);
        exit(0);
    }
    else
    {waitpid(pd,&Status,0);
    fputs(ANSI_COLOR_GREEN"MyShell$ "ANSI_COLOR_RESET,stdout);
    fflush(stdout);
    
    
    fflush(stdin);
    return;
    }
}

int is_bgprocess(char *input){
    int len = strlen(input);
    len--;
    if(input[len]=='&'){
        input[len] = '\0';
        len--;
        while(isspace(input[len])){
            input[len]='\0';
        }
        return 1;
    }
    return 0;

}