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
char*set_mode(char*,int*);

int main()
{
    fprintf(stdout,"Shell Running\n");
    char*input;
    int Status;
    while(1)
    {
        input = get_input();
        int p=fork();
        if(p==0)
        break;
        else
        {waitpid(p,&Status,0);
        printf("process pid=%d, return status=%d\n",p,Status);}   
        printf("\n");
    }
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
    switch(mode){
        case NONE:   
            
            pd=fork();
            if(pd==0){
                execvp(arg_list[0],arg_list);
                
                exit(0);
            }
            int st;
            waitpid(pd,&st,0);
            break;
        
        case PIPE:
            printf("");
            int pw[2];
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
            {   int st;
                waitpid(pd,&st,0);
                close(0);
                dup(pw[0]);
                close(pw[1]);
                execute(nex_tok);
                


            }
            break;
        
        case REDIRECT_OUT:
            printf("redirect OUT\n");
            break;

        case REDIRECT_APPEND:
            printf("APPEND\n");
            break;

        case REDIRECT_IN:
            printf("REDIRECT IN\n");
            break;

        case PIPE_DOUBLE:
            printf("DOUBLE\n");
            break;

        case PIPE_TRIPLE:
            printf("TRIPLE\n");
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
    if(c=='|'||c=='<'||c=='>') return 1;
    return 0;
}