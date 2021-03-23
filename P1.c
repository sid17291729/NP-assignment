#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#define MAXLEN 100
int main()
{
    printf("shell running\n");
    char buf[MAXLEN];
    char *arg_list[MAXLEN];
    int Status;
    while(1)
    {
        fputs("prompt>",stdout);
        fflush(stdout);
        fgets(buf,MAXLEN,stdin);
        buf[strlen(buf)-1]='\0';
        char *c=strtok(buf," ");
        int arg_ptr=0;
        
        while(c)
        {
            arg_list[arg_ptr++]=c;
            c=strtok(NULL," ");

        }
        arg_list[arg_ptr]=NULL;
        printf("\n");
        pid_t p=fork();
        if(p==0)
        {
           execvp(arg_list[0],arg_list);
           printf("error\n");
        }
        waitpid(p,&Status,0);
        
        printf("\nprocess pid=%d, return status=%d\n\n",p,Status);
        

    }
}