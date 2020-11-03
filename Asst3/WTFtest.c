#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>


void writeTo(int,char*);
pid_t serverInit(char*);
int forkExec(char*[]);
int clientInit(char*);

pid_t serverPID;
pid_t clientPID;

void stopSig(int signum) {
    (void) signal(SIGINT,SIG_DFL);
    kill(serverPID,SIGINT);
    exit(0);
}


int main(int argc,char* argv[]) {
    
    if (argc != 2) {
        printf("Please a port number to start the WTFtest executable\n");
        exit(0);
    }
    
    (void) signal(SIGINT,stopSig);
    int stat;
    serverPID = serverInit(argv[1]);
    if (clientPID != 0) {
        printf("Client could not be initialized... try again.\n");
        kill(serverPID,SIGINT);
        return -1;
    }
    chdir("./client");
    clientPID = clientInit(argv[1]);
    char* initialList[] = {"WTF","create","intializedestroy",NULL};
    forkExec(initialList);
    printf("\n");
    char* destroyList[] = {"WTF","destroy","initializedestroy",NULL};
    forkExec(destroyList);
    printf("\n");
    
    
    char* argList[] = {"WTF","create","TestProject",NULL};
    forkExec(argList);
    printf("\n");
    //free(argList);


    int fd2 = open("TestProject/test1.txt",O_WRONLY | O_CREAT | O_TRUNC,00600);
    writeTo(fd2,"Hello there writing this stuff to a file to see what happens hopefully it don't break but lets see what it does\n");
    close(fd2);
    char* argList2[] = {"WTF","add","TestProject","TestProject/test1.txt",NULL};
    forkExec(argList2);
    printf("\n");

    
    int fd3 = open("TestProject/test2.txt",O_WRONLY | O_CREAT | O_TRUNC,00600);
    writeTo(fd3,"gonna get removed peace out\n");
    close(fd3);
    char* argList3[] = {"WTF","add","TestProject","TestProject/test2.txt",NULL};
    forkExec(argList3);
    printf("\n");
    
    
    int fd4 = open("TestProject/test3.txt",O_WRONLY | O_CREAT | O_TRUNC,00600);
    writeTo(fd4,"not gonna get modified yea\n");
    char* argList4[] = {"WTF","add","TestProject","TestProject/test3.txt",NULL};
    forkExec(argList4);
    printf("\n");
    close(fd4);

    
    char* argList5[] = {"WTF","remove","TestProject","TestProject/test2.txt",NULL};
    forkExec(argList5);
    printf("\n");
    

    char* argList6[] = {"WTF","commit","TestProject",NULL};
    forkExec(argList6);
    printf("\n");
    
    
    char* argList7[] = {"WTF", "push","TestProject",NULL};
    forkExec(argList7);
    printf("\n");
    
    
    char* argList8[] = {"WTF","currentversion","TestProject",NULL};
    forkExec(argList8);
    printf("\n");

    
    system("rm -r TestProject");
    printf("Removed TestProject...\n");
    char* argList9[] = {"WTF","checkout","TestProject",NULL};
    forkExec(argList9);
    printf("\n");

    
    char* argList10[] = {"WTF","update","TestProject",NULL};
    forkExec(argList10);
    printf("\n");
    
    
    char* argList11[] = {"WTF", "upgrade","TestProject",NULL};
    forkExec(argList11);
    printf("\n");


    char* argList12[] = {"WTF","history","TestProject",NULL};
    forkExec(argList12);
    printf("\n");
    
    
    char* argList13[] = {"WTF","rollback","TestProject","1",NULL};
    forkExec(argList13);
    printf("\n");

    printf("Testing complete\n");
    kill(serverPID,SIGINT);
    //waitpid(serverPID,&stat,0);
    return 0;
}
    
pid_t serverInit(char* port) {

    char* argList[] = {"WTFserver",port,NULL};
    pid_t pid = fork();
    if (pid == -1) {
        printf("Fork failed... try again");
        exit(0);
    } else if (pid == 0) {
        chdir("./server");
        execv(argList[0],argList);    
        exit(0);
    } else {
        return pid;
    }

}

int clientInit(char* port) {
    
    char* argList[] = {"WTF","configure","127.0.0.1",port,NULL};
    return forkExec(argList);

}

int forkExec(char* argList[]) {
    pid_t pid;
    int stat;
    pid = fork();
    if (pid == -1) {
        printf("Fork failed ... try again\n");
        exit(0);
    } else if (pid == 0) {
        execv(argList[0],argList);
        exit(0);
    } else {
        if (waitpid(pid,&stat,0) > 0) {
            if (WIFEXITED(stat) && !WEXITSTATUS(stat)) {
                printf("Successfully executed command\n");
            } else if ( WIFEXITED(stat) && WEXITSTATUS(stat)) {
                if (WEXITSTATUS(stat) == 127) {
                    printf("Exec failed, try again...\n");
                    return 1;
                } else {
                    printf("Program cleaned up correctly but returned a status that wasn't zero...\n");
                    return 1;    
                }
            } else {
                printf("Program didn't terminate properly\n");
                return 1;
            
            }
        } else {
            printf("Wait failed... try again\n");
            return 1;
        }
    }
    return 0;
}

void writeTo(int fd, char* word) {

    int bytesWritten = 0;
    int bytestoWrite = strlen(word);
    while (bytesWritten < bytestoWrite) {
        bytesWritten = write(fd,word,bytestoWrite - bytesWritten);
    }
    close(fd);

}

















