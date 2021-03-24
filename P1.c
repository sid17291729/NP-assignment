#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<ctype.h>

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MAXLEN 1024
#define MAXARGLEN 100
enum MODE{NONE, PIPE, REDIRECT_OUT, REDIRECT_IN, REDIRECT_APPEND, PIPE_DOUBLE,PIPE_TRIPLE};

char read_buf[MAXLEN];
char *arg_list[MAXARGLEN];

int execute(char*input);
char*get_input();
int isDelim(char c);

int main()
{
    fprintf(stdout,"Shell Running\n");
    char*input;
    while(1)
    {
        input = get_input();
        execute(input);
        printf("\n");
    }
}


int execute(char*input){
    int Status;
    pid_t p;
    int mode=NONE;
    int arg_ptr=0;
    char *nex_tok=input;
    //fix parsing
    while(*nex_tok){
        arg_list[arg_ptr++]=nex_tok;
        while(*nex_tok && !isDelim(*nex_tok)){
            nex_tok++;
        }
        while(isspace(*nex_tok)){
            *nex_tok = '\0';
            nex_tok++;
        }
        if(*nex_tok=='|'){
            mode = PIPE;
            nex_tok++;
            if(*nex_tok=='|'){
                mode = PIPE_DOUBLE;
                nex_tok++;
                if(*nex_tok=='|'){
                    mode = PIPE_TRIPLE;
                    nex_tok++;
                }
            }
            break;
        }
    }
    arg_list[arg_ptr]=NULL;
    
    switch(mode){
        case NONE:    
            p=fork();
            if(p==0){
                execvp(arg_list[0],arg_list);
                printf("ERROR\n");
            }
            break;
        
        default: printf("ERROR. THIS REDIRECTION IS NOT SUPPORTED YET\n");
    }
    
    waitpid(p,&Status,0);
    printf("process pid=%d, return status=%d\n",p,Status);
    return p;
}

char*get_input(){
    fputs(ANSI_COLOR_GREEN"MyShell$ "ANSI_COLOR_RESET,stdout);
    fflush(stdout);
    fgets(read_buf,MAXLEN,stdin);
    int ptr_end = strlen(read_buf)-1;
    int ptr_strt = 0;
    while(isspace(read_buf[ptr_end])) read_buf[ptr_end--]='\0';
    while(isspace(read_buf[ptr_strt])) ptr_strt++;
    return read_buf+ptr_strt;
}

int isDelim(char c){
    if(isspace(c)||c=='|'||c=='<'||c=='>') return 1;
    return 0;
}