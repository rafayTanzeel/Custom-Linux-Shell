#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>

typedef struct process
{
  struct process *next;
  char **argv;
  pid_t pid;
} process;


typedef struct job
{
  struct job *next;
  process *phead;
  process *ptail;
  pid_t pgid;
} job;

job *first_job=NULL;
job *last_job=NULL;

void clearShellScreen();

int sizeFromCommands(const char *Arr, char *delim);

void parseCommand(char **store, char *command, char *delim);

void add_process(job *j, int no_Of_Proc);

void add_Job(job *j, char *in);

void specialCommand(char *store);


void exec_process(job *j, char *arg, int fg);


int main(){
    clearShellScreen();
    while (tcgetpgrp (STDIN_FILENO) != getpgrp ())
        kill (getpgrp (), SIGTTIN);
    signal (SIGINT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    setpgrp();
    tcsetpgrp (STDIN_FILENO, getpid());

    while(1){
    printf("Shell>> ");
    char *buffer=(char *)malloc(sizeof(char)*100);
    fgets(buffer,100,stdin);
    if(strstr(buffer,"exit")!=NULL || strstr(buffer,"cd")!=NULL){
        specialCommand(buffer);
    }
    else {
    add_Job(first_job, buffer);
    }
}
    return 0;
}

void parseCommand(char **store, char *command, char *delim){
        char *token;
        char *saveptr;
        token=strtok_r(command,delim,&saveptr);
    int index=0;
        while(token!=NULL){
        store[index]=token;
        token=strtok_r(NULL,delim,&saveptr);
        index++;
            }
}


void exec_process(job *j, char *arg, int fg){
    process *p;
    j->pgid=getpid();
    if(fg){
        tcsetpgrp(STDIN_FILENO,j->pgid);
    }
    signal (SIGINT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    int fd[2];

    if(pipe(fd)==-1){
        error("Can't create the pipe");
    }
    int loop=0;
    for(p=j->phead; p; p=p->next, loop++){
     if(loop!=0){
        free(j->phead);
        j->phead=p;   
    }
        char delim[]={' ','\t','\n','&'};
         char delim2[]={'|','\n'};
         char *token;
         char *saveptr;
        if(loop==0){
        token=strtok_r(arg,delim2,&saveptr);
            }
        else{
        token=strtok_r(NULL,delim2,&saveptr);
            }
   
        pid_t pid=fork();

            if(pid<0){
               fprintf(stderr,"Fork Failed");
               exit(1);
            }
            else if(pid==0){
            p->pid=getpid();

        if(loop%2 || loop==1){
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        }
        else{
        close(fd[1]);
            dup2(fd[0],STDIN_FILENO);
        }
        
            size_t commandSize=sizeFromCommands(token,delim);
            char **Command=(char **)malloc(sizeof(char *)*commandSize);
  
            parseCommand(Command,token,delim);

            p->argv=Command;
      
            execvp(p->argv[0], p->argv);

            }
            else{
	wait(NULL);
               }
}
j->phead=NULL;
j->ptail=NULL;
}


void clearShellScreen(){

pid_t pid=fork();

    if(pid<0){
       fprintf(stderr,"Fork Failed");
       exit(1);
    }
    else if(pid==0){
        execlp("clear", "clear", NULL);
    }
    else{
           wait(NULL);
       }

}

int sizeFromCommands(const char *Arr, char *delim){
int size=0;
char *buff=(char *)malloc(sizeof(Arr));
strcpy(buff,Arr);
        char *token;
        char *saveptr;
        token=strtok_r(buff,delim,&saveptr);
        while(token!=NULL){
        size++;
        token=strtok_r(NULL," ",&saveptr);
            }
free(buff);
return size;
}


void add_Job(job *j, char *in){
if(j==NULL && last_job==NULL){
        j=(job *)malloc(sizeof(job));
        last_job=j;
        j->next=NULL;
    }
    else{
        job *z=(job *)malloc(sizeof(job));
        z->next=j;
        j=z;
    }
    char delim[]={'|','\n'};
    size_t processSize=sizeFromCommands(in,delim);
    add_process(j,processSize);
    if(in[strlen(in)-2]=='&'){
    exec_process(j, in, 0);
    }
    else{
    char delim[]={'&','\n'};
         char *token;
         char *saveptr;
        token=strtok_r(in,delim,&saveptr);
    exec_process(j, token, 1);
}
}

void add_process(job *j, int no_Of_Proc){
    int i=0;
       while(i<no_Of_Proc){
        if(j->phead==NULL && j->ptail==NULL){
            j->phead=(process *)malloc(sizeof(process));
            j->ptail=j->phead;
        j->phead->next=NULL;
                    }
        else{
            process *z=(process *)malloc(sizeof(process));
            z->next=j->phead;
            j->phead=z;
                }
                i++;
        }
}

void specialCommand(char *store){
    char delim[]={' ','\t','\n'};
      size_t commandSize=sizeFromCommands(store,delim);
    char **Command=malloc(sizeof(char *)*commandSize);
    parseCommand(Command, store, delim);

    if(strncmp(Command[0],"exit",4)==0){
        exit(0);
    }
    else if(strncmp(Command[0],"cd",2)==0){
        chdir(Command[1]);
    }
}
