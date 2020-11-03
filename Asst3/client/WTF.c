#include <errno.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <openssl/sha.h>
#define SA struct sockaddr
int addFile(char*,char*); 
void checkout(char*,char*,char*);
char* combineString(char*,char*);
int compareString(char*,char*);
int commit(char*,char*);
char* compHash(char*);
void configure(char*,char*);
char* copyString(char*,char*);
int create(char*);
char* createCom(char*);
int extractInfo(char*);
void extractMan(char*);
void func(int,char*,char*,char*,int);
void listDirectories(char*);
char* liveHash(char*);
void makeDirectories(char*);
char* readConf(int);
char* readManifest(int);
int removeFile(char*,char*);
char* readServerman(char*);
void stopSig(int);
char* substring(char*,int,int);
int tableComphash(char*);
void tableInit(int);
void tableInsert(char*,char*,char*,char*);
int tableSearch(char*);
int update(char*,char*,char*);
void updateManifest(char*,char*,char*);
char* upgrade(char*,int);
int upgradeParse(char*);
void writeTo(int,char*);

typedef unsigned char byte;

int sockfd, connfd; 

typedef struct _hashNode {
    char* version;
    char* code;
    char* filepath;
    char* shacode; //sha256 hashcode
    struct _hashNode* next;
}hashNode;

typedef struct _hashTable {
    int size;
    hashNode** table;
}hashTable;

hashTable* table;

int hashSize = 0;

char* directories = "";

void stopSig(int signum) { 
    printf("\nStopping connection to server\n");
    (void) signal(SIGINT,SIG_DFL);
    close(sockfd);
    exit(0);
}

int main(int argc, char** argv) 
{     

    (void) signal(SIGINT,stopSig);
    if (argc < 3 || argc > 4) {
        printf("Error: Invalid number of arguments\n");
        exit(0);
    }
    
    if (compareString("configure\0",argv[1]) == 0) {
        configure(argv[2],argv[3]);
        printf("Successfully created .configure file\n");
        return 0;
    } else if (compareString("add\0",argv[1]) == 0) {
        tableInit(100);
        addFile(argv[2],argv[3]);
        return 0;
    } else if (compareString("remove\0",argv[1]) == 0) {
        tableInit(100);
        removeFile(argv[2],argv[3]);
        return 0;
    }
    int conf = open("./.configure",O_RDONLY);
    if (conf == -1) {
        printf("Fatal Error:No configure file present\n");
        exit(0);
    }

    char* confInfo = readConf(conf);
    int split = extractInfo(confInfo);
    char* ip = substring(confInfo,0,split);
    int port = atoi(substring(confInfo,split+1,-1));
    //printf("IP: %s, PORT: %d\n",ip,port);
    struct sockaddr_in servaddr, cli; 
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(ip); 
    servaddr.sin_port = htons(port); 

    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 

    // function for chat 
    if (argc == 3 || argc == 4) {
        func(sockfd,argv[1],argv[2],argv[3],-1);
    }
    // close the socket
    close(sockfd); 
}
//file name takes in the path rather than the blank file name
int addFile(char* projName, char* filename) {
    DIR *d;
    struct dirent *dir;
    if (!(d = opendir(projName))) {
        printf("%s is not a valid project name\n",projName);
        return -1;
    }


    int fd = open(filename,O_RDONLY);
    if (fd == -1) {
        printf("%s is not a valid file name\n",filename);
        return -1;
    }
    char* manFile = combineString(projName,"/.Manifest\0");
    int manFD = open(manFile,O_RDONLY);
    char* num = readManifest(manFD);
    close(manFD);    
    if (tableSearch(filename) != -1) {
        int manFD2 = open(manFile,O_WRONLY | O_CREAT | O_TRUNC,00600);
        num = combineString(num,"\n\0");
        writeTo(manFD2,num);
        int i;
        for (i = 0; i < table->size; i++) {
            //printf("hi\n");
            hashNode* temp = table->table[i];
            while (temp) {
                if (compareString(temp->filepath,filename) == 0) {
                    writeTo(manFD2,temp->version);
                    writeTo(manFD2," ");
                    writeTo(manFD2,"!MD\0");
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->filepath);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->shacode);
                    writeTo(manFD2,"\n");
                    temp = temp->next;
                } else {
                    writeTo(manFD2,temp->version);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->code);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->filepath);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->shacode);
                    writeTo(manFD2,"\n");
                    temp = temp->next;
                }
            }
        }
        close(manFD2);
        printf("Modified .Manifest entry for %s\n",filename);
        return 0; //warning
    }

    int manFD2 = open(manFile,O_RDWR);
    //memset(buffer,'\0',256);
    //read(manFD2,buffer,2);
    int t = lseek(manFD2,0,SEEK_END);
    
    char* hashedStuff = "";

    char* toHash = readConf(fd);
    hashedStuff = combineString(hashedStuff,compHash(toHash));
    
    char* letter = combineString(num," !AD \0");
    char* result = combineString(letter,filename);
    result = combineString(result, " \0");
    result = combineString(result,hashedStuff);
    
    writeTo(manFD2,result);
    write(manFD2,"\n",1);
    printf("Successfully added %s to .Manifest of %s.\n",filename,projName);
    close(fd);
    return 1; //success
}
 
void checkout(char* projname,char* message,char* manText) {
    projname = combineString(projname,"/.Manifest\0");
    int mFD = open(projname, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    writeTo(mFD,manText);
    close(mFD);
    message = substring(message,12,-1);
    //printf("%s\n",message);
    int length = strlen(message);
    int i = 0;
    while (message[i] != ':') {
        i++;
    }
    int num = atoi(substring(message,0,i));
    int start = i+1;
    int end = i+1;
    //printf("%d\n",num);
    int k;
    for (k = 0; k < num; k++) {
        int nameLength = 0;
        int contentLength = 0;
        while (message[end] != ':') {
            end++;
        }
        nameLength = atoi(substring(message,start,end));
        start = end+1;
        end+= 1;
        while (message[end] != ':') {
            end++;
        }
        contentLength = atoi(substring(message,start,end));
        start = end + 1;
        end = start + nameLength;
        char* fileName = substring(message,start,end);
        start = end;
        end = start + contentLength;
        char* fileContent = substring(message,start,end);
        //printf("%s and %s\n",fileName,fileContent);
        int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 00600);
        writeTo(fd,fileContent);
        close(fd);
        start = end;
    }
}

char* combineString(char* str1, char* str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    char* result = (char*)malloc((len1+len2)*sizeof(char) + 1);
    memset(result,'\0',(len1+len2+1));
    int i;
    int j = 0;
    for ( i = 0; i < len1; i++) {
        result[i] = str1[i];
        j++;
    }
    for ( i = 0; i < len2; i++) {
        result[j] = str2[i];
        j++;
    }
    return result;
}

int commit(char* manBuff, char* project) {
    char* clientMan = combineString(project,"/.Manifest\0");
    int manFD = open(clientMan,O_RDONLY);
    tableInit(100);
    char* servVer = readServerman(manBuff);//stored in hash table    
    char* clientManstuff = readConf(manFD);
    close(manFD);
    int i = 0;
    while (clientManstuff[i] != '\n') {
        i++;    
    }
    char* clientVer = substring(clientManstuff,0,i);
    int serv = atoi (servVer);
    int clien = atoi(clientVer);
    if (serv != clien) {
        return -1;//versions dont match
    }
    char* comFile = combineString(project,"/.Commit\0");
    int mitFD = open(comFile, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    int counter = 0;
    int start = i+1;
    char* version = "";//0
    char* code = "";//1
    char* filepath = "";//2
    char* shacode = "";//3
    bool vers = true;
    for (i = i+1; i < strlen(clientManstuff); i++) {
        if (clientManstuff[i] == ' ') {
                if (counter == 0) {    
                    version = substring(clientManstuff,start,i);
                } else if (counter == 1) {
                    code = substring(clientManstuff,start,i);
                } else {
                    filepath = substring(clientManstuff,start,i);    
                }
            
            counter++;
            start = i+1;
        }
        if (clientManstuff[i] == '\n') {
            shacode = substring(clientManstuff,start,i);
            start = i+1;
            counter = 0;    
            int t = tableSearch(filepath);
            hashNode* temp;
            if (t != -1) {
                temp = table->table[t];
                while (temp != NULL && (hashSize != 0 && hashSize != -1)) {
                    if (compareString(temp->filepath,filepath) == 0) {
                        break;
                    }
                    temp = temp->next;
                }
            } else {
                temp = (hashNode*)malloc(sizeof(hashNode));
                temp->version = clientVer;
                temp->code = "BAD";
                temp->filepath = "NULL";
                temp->shacode = "NULL";
            }
            if (hashSize != 0 && hashSize != -1) {
                if (compareString(temp->version,version) != 0 && (hashSize != -1 && hashSize != 0)) {
                    printf("Versions don't match in one or more files, update before committing again\n");
                    remove(comFile);
                    return -1;
                }
            }
             if ((hashSize == -1 || hashSize == 0) || compareString(temp->code,code) != 0) {
                if (compareString(code,"!MD\0") == 0) {
                    int fd = open(filepath,O_RDONLY);
                    char* toCode = readConf(fd);
                    char* hashedStuff = "";
                    hashedStuff = combineString(hashedStuff,compHash(toCode));
                    writeTo(mitFD,version);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,code);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,filepath);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,hashedStuff);
                    writeTo(mitFD,"\n");
                    char* stdoutMsg = combineString("M \0", filepath);
                    writeTo(1,stdoutMsg);
                    writeTo(1,"\n\0");
                    free(stdoutMsg);
                } else if (compareString(code,"!UT") != 0) {
                    writeTo(mitFD,version);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,code);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,filepath);
                    writeTo(mitFD," \0");
                    writeTo(mitFD,shacode);
                    writeTo(mitFD,"\n");
                    char* stdoutMsg = "";
                    if (compareString(code,"!AD") == 0) {
                        stdoutMsg = combineString("A \0",filepath);
                    } else {
                        stdoutMsg = combineString("D \0",filepath);
                    }
                    writeTo(1,stdoutMsg);
                    writeTo(1,"\n\0");
                    free(stdoutMsg);
                }
            }
            
        }
    }
    close(mitFD);
    return 1;//success
}

int compareString(char* str1, char* str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    int shorter = 0;
    int len = len1;
    if (len1 > len2) {
        shorter = 1;//first one is shorter
        len = len1;
    } else if (len2 < len1) {
        shorter = 2;//second one is shorter
        len = len2;
    } else {
        shorter = 0;//equal length
        len = len1;
    }

    int i;
    for ( i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return ((int)str1[i] - (int)str2[i]);//negative if str1 is lesser, positive if str1 is greater
        }    
    }
    if (len1 < len2) {
        return -1;
    } else if (len2 < len1) {
        return 1;
    }
    return 0;//equal strings
}

char* compHash(char* fileContent) {
    int DataLen = strlen(fileContent);
    byte digest[SHA256_DIGEST_LENGTH];
    int i;
    SHA256(fileContent,DataLen,digest);
    unsigned char* hash = malloc(SHA256_DIGEST_LENGTH * 2);
    for (i=0; i<SHA256_DIGEST_LENGTH; i++)
        //printf("%02x",digest[i]);
        sprintf((char*)(hash+(i*2)),"%02x", digest[i]);
    return hash;
}

void configure(char* ip, char* port) {
    int fd;
    fd = open ("./.configure", O_WRONLY | O_CREAT | O_TRUNC,00600);
    writeTo(fd,ip);
    writeTo(fd,"\n\0");
    writeTo(fd,port);
    close(fd);
}

char* copyString(char* to, char* from) {
    int length = strlen(from);
    to = (char*)malloc(length * sizeof(char) + 1);
    memset(to,'\0',(length+1));
    int i;
    for ( i = 0; i < length; i++) {
        to[i] = from[i];
    }
    return to;
}

int create(char* projectName) {
    struct stat st = {0};

    if (stat(projectName,&st) == -1) {
        mkdir(projectName,0700);
        char* histFile = combineString(projectName,"/.History\0");
        mkdir(histFile,0700);
        char* opsFile = combineString(projectName,"/.Operations\0");
        int opsFD = open(opsFile, O_WRONLY | O_CREAT | O_TRUNC,00600);
        close(opsFD);
        char* manFile = combineString(projectName,"/.Manifest\0");
        int manFD = open(manFile,O_WRONLY | O_CREAT | O_TRUNC, 00600);
        writeTo(manFD,"1\n\0");
        close(manFD);
        return 1;
    } else {
        printf("Project name already exists on client\n");
        return -1;
    }
}

char* createCom(char* comText) {
    char* message = "sendfile:\0";
    int fileCounter = 0;
    int size = strlen(comText);
    int i;
    for (i = 0; i < size; i++) {
        if (comText[i] == '\n') {
            fileCounter++;
        }
    }
    
    char num[256];
    memset(num,'\0',256);
    sprintf(num,"%d:",fileCounter);
    message = combineString(message,num);
    int counter = 0;
    int start = 0;
    char* code;
    for (i = 0; i < size; i++) {
        if (comText[i] == ' ') {
            
            if (counter == 1) {
                code = substring(comText,start,i);
                message = combineString(message,code);
                message = combineString(message,":\0");
            } else if (counter == 2) {
                char* filePath = substring(comText,start,i);
                memset(num,'\0',256);
                sprintf(num,"%d:",strlen(filePath));
                message = combineString(message,num);
                if (compareString(code,"!RM") == 0) {
                    memset(num,'\0',256);
                    sprintf(num,"%d:",strlen("DELETE\n\0"));
                    message = combineString(message,num);
                    message = combineString(message,filePath);
                    message = combineString(message,"DELETE\n\0");
                } else {    
                    int fpd = open(filePath,O_RDONLY);
                    char* fileContent = readConf(fpd);    
                    memset(num,'\0',256);
                    sprintf(num,"%d:",strlen(fileContent));
                    message = combineString(message,num);
                    
                    message = combineString(message,filePath);
                    message = combineString(message,fileContent);
                    free(fileContent);
                    close(fpd);
                }
            }
            start = i+1;
            counter++;
        }
        if (comText[i] == '\n') {
            counter = 0;
            start = i+1;
        }    
    }
    //printf("%s\n",message);
    return message;
}

int extractInfo(char* word) {
    int counter = 0;
    while (word[counter] != '\n') {
        counter++;
    }
    return counter;
}

void extractMan(char* manWord) {
    char** manInfo = (char**)malloc(4 * sizeof(char*));
    int i;
    int count = 0;
    int start = 0;
    for (i = 0; i < strlen(manWord); i++) {
        if (manWord[i] == ' ') {
            manInfo[count] = substring(manWord,start,i);
            start = i + 1;
            count++;        
        }
        if (count == 3) {
            manInfo[count] = substring(manWord,start,-1);
        }
    }
    tableInsert(manInfo[0],manInfo[1],manInfo[2],manInfo[3]);
    //printf("%s, %s, %s, %s\n",manInfo[0],manInfo[1],manInfo[2],manInfo[3]);
}

void func(int sockfd,char* action, char* projname,char* fname,int version) 
{ 
    //char buff[80]; 
    char buff[256];
    bzero(buff,sizeof(buff));
    if (compareString("create",action) == 0) {
        char* total = combineString(action," \0");
        total = combineString(total,projname); 
        write(sockfd,total,strlen(total));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Project already exists on server\n\0") == 0) {
            printf("Project already exists on server, need to clone the project or pick a new name\n");
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        } else {
            int cr = create(projname);
            printf("%s",buff);
            return;
        }
    } else if (compareString("destroy", action) == 0) {
        char* total = combineString(action," \0");
        total = combineString(total,projname); 
        write(sockfd,total,strlen(total));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Destroy failed. Project does not exist on server\n\0") == 0) {
            printf("Destroy failed. Project does not exit on server\n");
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
        }else {
            printf("Successfully destroyed project on server...\n");
        }
        return;
    } else if (compareString("currentversion",action) == 0) {
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        char* message = "";
        int bytesRead = 0;
        bzero(buff,sizeof(buff));
        bytesRead = read(sockfd,buff,(sizeof(buff)));
        message = combineString(message,buff);
        if (compareString(message,"Project does not exist on server\n") == 0) {
            printf("%s",message);
            return;
        } else if (compareString(message,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        int length = atoi(message);
        write(sockfd,"I got your message\n",19);
        char buffer2[length+1];
        bzero(buffer2,sizeof(buffer2));
        bytesRead = read(sockfd,buffer2,length);
        char* m2 = "";
        m2 = combineString(m2,buffer2);
        printf("%s",m2);
    } else if (compareString("checkout",action) == 0) {
        DIR* d;
        struct dirent* dir;
        if ((d = opendir(projname))) {
            write(sockfd,"Error",5);
            printf("Project already exists on client, cannot clone\n");
            closedir(d);
            return;
        }
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if(compareString(buff,"Project does not exist on server\n") == 0) {
            printf("%s\n",buff);
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        mkdir(projname,0700);
        if (compareString(buff,"empty") != 0) {
            //directories
            int length2 = atoi(buff);
            write(sockfd,"I got your message\n",19);
            char buff4[length2 + 1];
            bzero(buff4,sizeof(buff4));
            read(sockfd,buff4,sizeof(buff4));
            //printf("%s\n",buff4);
            makeDirectories(buff4);
        }
        
        //manifest
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        int length = atoi(buff);
        write(sockfd,"I got your message\n",19);
        char buff3[length + 1];
        bzero(buff3,sizeof(buff3));
        read(sockfd,buff3,sizeof(buff3));
         //printf("%s\n",buff3);
        
        //file paths
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        length = atoi(buff);
        write(sockfd,"I got your message\n",19);
        //printf("Len: %d\n",length);
        char buff2[length + 1];
        bzero(buff2,sizeof(buff2));
        read(sockfd,buff2,length);
        checkout(projname,buff2,buff3);
        printf("Successfully cloned %s.\n",projname);
    } else if (compareString("commit",action) == 0) {
        DIR *d;
        struct dirent *dir;
        if (!(d = opendir(projname))) {
            write(sockfd,"Error\0",6);
            printf("%s does not exist on client\n",projname);
            return;
        } 
        char* flictFile = combineString(projname,"/.Conflict\0");
        int conFD = open(flictFile,O_RDONLY);
        if (conFD != -1) {
            write(sockfd,"Error\0",6);
            printf("%s contains a .Conflict file, resolve conflicts (update) before committing\n");
            return;
        }
        char* updFile = combineString(projname,"/.Update\0");
        int updFD = open(updFile,O_RDONLY);
        if (updFD != -1) {
            int t = lseek(updFD,0,SEEK_END);
            if (t != 0) {
                write(sockfd,"Error\0",6);
                printf("%s contains a nonempty .Update file, update before committing\n");
                return;
            }
        }
    
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));//Read manifest length from server
        if (compareString(buff,"Error") == 0) {
            printf("%s does not exist on server\n",projname);
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        int length = atoi(buff);
        bzero(buff,sizeof(buff));
        write(sockfd,"I got your message\n",19);
        char manBuff[length+1];
        memset(manBuff,'\0',length+1);
        read(sockfd,manBuff,length);
        //printf("%s\n",manBuff);
        //Manifest acquired, compare version now
        int success = commit(manBuff,projname);//-1 on error
        //printf("%d\n",success);
        if (success == -1) {
            printf("Versions in one or more files don't match, synch with the repository\n");
            write(sockfd,"Error",5);
            return;
        } else {
            char* mitFile = combineString(projname,"/.Commit\0");
            int mitFD = open(mitFile,O_RDONLY);
            char* toSend = readConf(mitFD);
            //printf("toSend: %s\n",toSend);
            char sendLen[256];
            memset(sendLen,'\0',256);
            sprintf(sendLen,"%d",strlen(toSend));
            write(sockfd,sendLen,sizeof(sendLen));
            bzero(buff,sizeof(buff));
            read(sockfd,buff,sizeof(buff));
            write(sockfd,toSend,strlen(toSend));
            printf("Successfully sent over .Commit file to server\n");
            close(mitFD);
            return;
        }
    } else if (compareString("push",action) == 0) {
        char* comFile = combineString(projname,"/.Commit\0");
        int comFD = open(comFile,O_RDONLY);
        if (comFD == -1) {
            printf("No .Commit present for %s or project doesn't exist on client, commit before you push\n",projname);
            write(sockfd,"Error",5);
            return;
        }
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString("Project does not exist on server\n",buff) == 0) {
            printf("%s\n",buff);
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        char* comText = readConf(comFD);
        char length[256];
        memset(length,'\0',256);
        sprintf(length,"%d",strlen(comText));
        write(sockfd,length,sizeof(length));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        write(sockfd,comText,strlen(comText));
        //printf("COMTEXT: %s\n",comText);
        printf("Successfully sent commit over\n");
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Success") != 0) {
            printf("Commit not present on server, commmit before you push\n");
            close(comFD);
            return;
        }
        /*
        char* sysMessage = combineString("rsync -Rr \0",projname);
        char* dest = combineString(projname,"/.History/\0");
        dest = combineString(dest,num);
        mkdir(dest,0700);
        sysMessage = combineString(sysMessage," \0");
        sysMessage = combineString(sysMessage,dest);
        system(sysMessage);    
        */
        tableInit(100);
        int manFD = open(combineString(projname,"/.Manifest\0"),O_RDONLY);
        char* num = readManifest(manFD);
        listDirectories(projname);
        close(manFD);
        //printf("strlen: %d\n",strlen(directories));
        if (strlen(directories) <= 1) {
            write(sockfd,"empty",5);
        } else {
            char lenbufferthing[256];
            memset(lenbufferthing,'\0',256);
            sprintf(lenbufferthing,"%d",strlen(directories));
            write(sockfd,lenbufferthing,sizeof(lenbufferthing));    
            bzero(buff,sizeof(buff));
            read(sockfd,buff,sizeof(buff));
            write(sockfd,directories,strlen(directories));
        }
        bzero(buff,sizeof(buff));

    
        lseek(comFD,0,SEEK_SET);
        char* sendFile = createCom(comText);
        //printf("sendfile: %s",sendFile);
        memset(length,'\0',256);
        sprintf(length,"%d",strlen(sendFile));
        write(sockfd,length,sizeof(length));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        write(sockfd,sendFile,strlen(sendFile));//write commit message over
        close(manFD);
        //clean up manifest
        updateManifest(comText,projname,num);
        int newMan = open(combineString(projname,"/.Manifest\0"),O_RDONLY);
        char* newMantext = readConf(newMan);
        //printf("%s\n",newMantext);        
        
        memset(length,'\0',256);
        sprintf(length,"%d",strlen(newMantext));
        write(sockfd,length,sizeof(length));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        write(sockfd,newMantext,strlen(newMantext));
        printf("Successful push\n");
        remove(comFile);
        close(newMan);
        close(comFD);
    } else if (compareString(action,"update") == 0) {
        DIR *d;
        struct dirent dir;
        if (!(d = opendir(projname))) {
            printf("%s does not exist on client\n",projname);
            write(sockfd,"Error",5);
            closedir(d);
            return;
        }
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Error") == 0) {
            printf("Project does not exist on server\n");
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        int length = atoi(buff);
        char manbuff[length+1];//server manifest
        memset(manbuff,'\0',length+1);
        write(sockfd,"Success",7);
        read(sockfd,manbuff,length);
        //printf("manbuff: %s\n",manbuff);
        tableInit(100);
        int clientFD = open(combineString(projname,"/.Manifest\0"),O_RDONLY);
        char* ver = readManifest(clientFD);
        int success = update(projname,manbuff,ver);
        if (success == 2) {
            printf(".Conflict file created, fix conflicts before updating\n");
        } else if (success == 0) {
            printf(".Update file successfully created with list of required updates\n");
        }
        return;
            
    } else if (compareString(action,"upgrade") == 0) {
        DIR *d;
        struct dirent dir;
        if (!(d = opendir(projname))) {
            printf("%s does not exist on client\n",projname);
            write(sockfd,"Error",5);
            return;
        } 
        int flictFD = open(combineString(projname,"/.Conflict"),O_RDONLY);
        if (flictFD != -1) {
            printf("Resolve conflicts first and then update again\n");
            write(sockfd,"Error",5);
            close(flictFD);
            return;
        }
        close(flictFD);
        int updFile = open(combineString(projname,"/.Update"),O_RDONLY);
        if (updFile == -1) {
            printf("Call update first then upgrade\n");
            write(sockfd,"Error",5);
            close(updFile);
            return;
        } else if (lseek(updFile,0,SEEK_END) <= 1) {
            printf("Up to date\n");
            write(sockfd,"Error",5);
            close(updFile);
            return;
        }
        lseek(updFile,0,SEEK_SET);
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Error\0") == 0) {
            printf("Project does not exist on server\n");
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        char* toUpgrade = upgrade(projname,updFile);
        bzero(buff,sizeof(buff));
        sprintf(buff,"%d",strlen(toUpgrade));
        write(sockfd,buff,sizeof(buff));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        
        write(sockfd,toUpgrade,strlen(toUpgrade));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        bzero(buff,sizeof(buff));
        
        read(sockfd,buff,sizeof(buff));
        int len = atoi(buff);
        write(sockfd,"Success",7);
        char sendFile[len + 1];
        memset(sendFile,'\0',len+1);
        read(sockfd,sendFile,len);//sendfile
        
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        
        if (compareString(buff,"empty") != 0) {
            int dirlen = atoi(buff);
            char dirbuff[dirlen+1];
            memset(dirbuff,'\0',dirlen+1);
            write(sockfd,"Success",7);
            read(sockfd,dirbuff,dirlen);
            makeDirectories(dirbuff);
        }
        write(sockfd,"Success",7);
        int upgSuccess = upgradeParse(sendFile);
        
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        int newMan = atoi(buff);
        //printf("newMan: %d\n",newMan);
        write(sockfd,"Success",7);
        char manBuff[newMan+1];
        memset(manBuff,'\0',newMan+1);
        read(sockfd,manBuff,newMan);
        int nmanFD = open(combineString(projname,"/.Manifest\0"),O_WRONLY | O_CREAT | O_TRUNC,00600);
        writeTo(nmanFD,manBuff);
        close(newMan);

        printf("Successfuly upgrade\n");
        
    } else if (compareString(action,"history") == 0) {
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Error") == 0) {
            printf("Project does not exist on server\n");
            return;
        } else if (compareString(buff,"Empty") == 0) {
            printf("Project does not have a history\n");
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        int len = atoi(buff);
        char opsLen[len + 1];
        memset(opsLen,'\0',len+1);
        write(sockfd,"Success",7);
        read(sockfd,opsLen,len);
        printf("%s",opsLen);
            
    } else if (compareString(action,"rollback") == 0) {
        char* total = combineString(action," \0");
        total = combineString(total,projname);
        total = combineString(total," \0");
        total = combineString(total,fname);
        //printf("%s\n",total);
        write(sockfd,total,strlen(total));
        bzero(buff,sizeof(buff));
        read(sockfd,buff,sizeof(buff));
        if (compareString(buff,"Error") == 0) {
            printf("Project does not exist or you requested an invalid rollback\n");
            return;
        } else if (compareString(buff,"locked") == 0) {
            printf("Connection failed, try again\n");
            return;
        }
        printf("Successfully reverted to version %s on server\n",fname);    
    } else {
        write(sockfd,"Error",5);
        printf("Invalid inputs, look at documentation and try inputting a command again\n");
    }
    close(sockfd);
}

void listDirectories(char* path) {
    DIR *d;
    struct dirent *dir;
    if (!(d = opendir(path))) {
        return;
    }
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (compareString(dir->d_name,".") == 0 || compareString(dir->d_name,"..") == 0 || compareString(dir->d_name,".History") == 0) {
                continue;    
            }
            char* temp = combineString(path,"/");
            temp = combineString(temp,dir->d_name);
            directories = combineString(directories,temp);
            directories = combineString(directories,"\n\0");
            listDirectories(temp);
        }    
    }
    closedir(d);
}

char* liveHash(char* filePath) {
    int fd = open(filePath,O_RDONLY);
    if (fd == -1) {
        return "DELETE";
    }
    char* fileText = readConf(fd);
    char* hashedStuff = "";
    hashedStuff = combineString(hashedStuff,compHash(fileText));
    close(fd);
    return hashedStuff;
}

void makeDirectories(char* dirs) {
    int len = strlen(dirs);
    int i;
    int start = 0;
    for (i = 0; i < len; i++) {
        if (dirs[i] == '\n') {
            mkdir(substring(dirs,start,i),0700);
            start = i+1;    
        }    
    }
}
 
char* readConf(int conFD) {
    int status = 1;
    int bytesRead = 0;
    char* confInfo = "";
    while (status > 0) {
        char buffer[101];
        memset(buffer,'\0',101);
        int readIn = 0;
        do {
            status = read(conFD,buffer,100 - readIn);
            if (status == 0) {
                break;
            }
            readIn += status;
        }while (readIn < 100);
        confInfo = combineString(confInfo,buffer);
    }
    return confInfo;
}

char* readManifest(int manFD) {
    int status = 1;
    int bytesRead = 0;
    char* holder;
    char* numRet;
    bool moreStuff = false;
    bool first = true;
    
    while (status > 0) {
        char buffer[101];
        memset(buffer,'\0',101);
        int readIn = 0;
        do {
            status = read(manFD,buffer,100 - readIn);
            if (status == 0) {
                break;
            }
            readIn+= status;
        }while(readIn < 100);
        int end = 0;
        int start = 0;
        while (end < 100) {
            char* temp;
            if (buffer[end] == '\n') {
                temp = substring(buffer,start,end);
                if (first == true) {
                    numRet = copyString(numRet,temp);
                    first = false;
                } else if (moreStuff == true) {
                    holder = combineString(holder,temp);
                    moreStuff = false;
                    extractMan(holder);
                } else {
                    extractMan(temp);
                }
                start = end + 1;
            }
            if (end == 99) {
                if (moreStuff == true) {
                    holder = combineString(holder,buffer);
                } else {
                    holder = substring(buffer,start, -1);    
                }
                moreStuff = true;
            }
            if (buffer[end] == '\0') {
                break;    
            }
            end++;
        }
    }
    return numRet;
}

int removeFile(char* projName, char* filename) {
    DIR *d;
    struct dirent *dir;
    if (!(d = opendir(projName))) {
        printf("%s is not a valid project name\n",projName);
        return -1;
    }


    int fd = open(filename,O_RDONLY);
    if (fd == -1) {
        printf("%s is not a valid file name\n",filename);
        return -1;
    }
    char* manFile = combineString(projName,"/.Manifest\0");
    int manFD = open(manFile,O_RDONLY);
    char* num = readManifest(manFD);
    close(manFD);
    
    int indexVal = tableSearch(filename);
    if ( indexVal == -1) {
        printf("Warning: File not present in .Manifest, add before you remove.\n",filename);
        return 0; //warning
    } else {
        remove(filename);
        int manFD2 = open(manFile,O_WRONLY | O_CREAT | O_TRUNC,00600);
        num = combineString(num,"\n\0");
        writeTo(manFD2,num);
        int i;
        for (i = 0; i < table->size; i++) {
            //printf("hi\n");
            hashNode* temp = table->table[i];
            while (temp) {
                if (compareString(temp->filepath,filename) == 0) {
                    writeTo(manFD2,temp->version);
                    writeTo(manFD2," ");
                    writeTo(manFD2,"!RM\0");
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->filepath);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->shacode);
                    writeTo(manFD2,"\n");
                    temp = temp->next;
                } else {
                    writeTo(manFD2,temp->version);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->code);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->filepath);
                    writeTo(manFD2," ");
                    writeTo(manFD2,temp->shacode);
                    writeTo(manFD2,"\n");
                    temp = temp->next;
                }
            }
        }
        close(manFD2);
        
    }
    close(fd);
    printf("Successfully removed %s from %s and updated .Manifest\n",filename,projName);
    return 1;
}

char* readServerman(char* manText) {
    int i = 0;
    while (i < strlen(manText) && manText[i] != '\n') {
        i++;
    }
    if ((i+1) == strlen(manText)) {
        char* version = manText;
        hashSize = -1;
        return version;
    }
    char* version = substring(manText,0,i);
    int length = strlen(manText);
    int start = i+1;
    for (i = i+1;i < length; i++) {
        if (manText[i] == '\n') {
            extractMan(substring(manText,start,i));
            start = i+1;
        }
    }

    return version;
}

char* substring(char* str, int start, int end) {
    char* result;
    if (end == -1) {
        int length = strlen(str);
        result = (char*)malloc((length-start)*sizeof(char) + 1);
        memset(result,'\0',(length-start + 1));
        int i;
        int j = 0;
        for ( i = start; i < length; i++) {
            result[j] = str[i];
            j++;
        }
    } else {
        result = (char*)malloc((end-start)*sizeof(char) + 1);
        memset(result,'\0',(end-start + 1));
        int i;
        int j = 0;
        for ( i = start; i < end; i++) {
            result[j] = str[i];
            j++;
        }    
    }
    return result;
}

int tableComphash(char* filepath) {
    int len = strlen(filepath);
    int code = 0;
    int i;
    for (i = 0; i < len; i++) {
        code += (filepath[0] - 0);
    }
    return (code % table->size);
}

void tableInit(int size) {
    table = (hashTable*)malloc(sizeof(hashTable));
    table->size = size;
    table->table = (hashNode**)malloc(size * sizeof(hashNode*));
    int i;
    for (i = 0; i < size; i++) {
        table->table[i] = NULL;
    }
}

void tableInsert(char* version, char* code, char* filepath, char* shacode) {
    int index = -1;
    index = tableComphash(filepath);
    if (index == -1) {
        printf("Error in hashInsert\n");
        exit(0);
    }
    hashNode* temp = table->table[index];
    hashNode* toInsert = (hashNode*)malloc(sizeof(hashNode));
    hashNode* temp2 = temp;
    while (temp2) {
        temp2 = temp2->next;
    }
    toInsert->version = version;
    toInsert->code = code;
    toInsert->filepath = filepath;
    toInsert->shacode = shacode;
    toInsert->next = temp;
    table->table[index] = toInsert;
    hashSize++;
}

int tableSearch(char* filepath) {
    int index = -1;
    index = tableComphash(filepath);
    if (index == -1) {
        printf("Error in hashInsert\n");
        exit(0);
    }
    hashNode* temp = table->table[index];
    hashNode* temp2 = temp;
    while (temp2) {
        if (compareString(temp2->filepath,filepath) == 0) {
            return index;
        }
        temp2 = temp2->next;
    }
    return -1;    
}

int update(char* project, char* serverMan, char* clientVer) {
    int sevLength = strlen(serverMan);
    //printf("hello:%s:hello\n",serverMan);
    int i = 0;
    int start = 0;
    int counter = 0;
    char* version = "";
    char* code = "";
    char* filepath = "";
    char* shacode = "";
    int flictFD = 0;
    int updateCounter = 0;
    int success = -1;// -1 for fail, 0 for partial, 1 for success
    bool conflict = false;
    while (serverMan[i] != '\n' && i < sevLength) {
        i++;    
    }
    start = i+1;
    char* sversion = substring(serverMan,0,i);
    if ((i+1) == strlen(serverMan)) {
        printf("No updates necessary, server manifest is empty\n");
        return 1;//empty server manifest
    }
    if (compareString(sversion,clientVer) == 0) {
        write(1,"Up to Date\n\0",12);
        int updFile = open(combineString(project,"/.Update"),O_WRONLY | O_CREAT | O_TRUNC,00600);
        close(updFile);
        remove(combineString(project,"/.Conflict"));
        return 1;//full success
    } else {
        int updFile = open(combineString(project,"/.Update"),O_WRONLY | O_CREAT | O_TRUNC, 00600);
        //printf("HELLO\n");
        for (i = i+1; i < sevLength; i++) {
            //printf("i: %d\n sevlength: %d\n",i,sevLength);
            if (serverMan[i] == ' ' || serverMan[i] == '\n') {
                if (counter == 0) {
                    //printf("%s\n",version);
                    version = substring(serverMan,start,i);
                } else if (counter == 1) {
                    code = substring(serverMan,start,i);
                    //printf("%s\n",version);
                } else if (counter == 2) {
                    filepath = substring(serverMan,start,i);
                    //printf("%s\n",version);
                } else if (counter == 3) {
                    shacode = substring(serverMan,start,i);
                    //printf("%s\n",version);
                }
                counter++;
                start = i+1;
            }
            if (counter == 4) {
                //all data stored
                if (tableSearch(filepath) != -1) {//found
                //printf("HELLOi%s\n",filepath);
                    char* freshHash = liveHash(filepath);
                    //printf("wat\n");
                    int index = tableComphash(filepath);
                    hashNode* temp = table->table[index];
                    //printf("temp: %s.%d\n",temp->filepath,index);
                    while (temp != NULL) {
                        if (compareString(temp->filepath,filepath) == 0) {
                            break;
                        }
                        //printf("gonext\n");    
                        temp = temp->next;
                    }
                    //printf("swagmoney\n");
                    if (compareString(freshHash,temp->shacode) != 0 && compareString(freshHash,shacode) != 0) {
                        if (conflict == false) {
                            flictFD = open(combineString(project,"/.Conflict"),O_WRONLY | O_CREAT | O_TRUNC,00600);
                            remove(combineString(project,"/.Update\0"));
                            conflict = true;
                            printf("conflict created\n");
                            return 2;
                        }
                        char* message = combineString(version," \0");
                        char* stdMes = combineString("C \0",filepath);
                        message = combineString(message,"!CF \0");
                        message = combineString(message,filepath);
                        message = combineString(message," \0");
                        message = combineString(message,freshHash);
                        message = combineString(message,"\n\0");
                        writeTo(flictFD,message);
                        writeTo(1,stdMes);
                        counter = 0;
                    } else if (compareString(freshHash,temp->shacode) == 0 && compareString(freshHash,shacode) != 0) {
                        char* message = combineString(version," \0");
                        char* stdMes = "";
                        if (compareString(shacode,"DELETE\0") == 0) {
                            message = combineString(message,"!RM\0");
                            stdMes = combineString(stdMes,"D \0");
                        } else {
                            message = combineString(message,"!MD\0");
                            stdMes = combineString(stdMes,"M \0");
                        }
                        message = combineString(message," \0");
                        message = combineString(message,filepath);
                        message = combineString(message," \0");
                        message = combineString(message,shacode);
                        message = combineString(message,"\n\0");
                        stdMes = combineString(stdMes,filepath);
                        writeTo(updFile,message);
                        writeTo(1,stdMes);
                        updateCounter++;
                        counter = 0;    
                    }
                    
                    
                } else {
                    //printf("inhere maybe\n");
                    char* message = combineString(version," \0");
                    char* stdMes = combineString("A \0",filepath);
                    message = combineString(message,code);
                    message = combineString(message," \0");
                    message = combineString(message,filepath);
                    message = combineString(message," \0");
                    message = combineString(message,shacode);
                    message = combineString(message,"\n\0");
                    writeTo(updFile,message);
                    writeTo(1,stdMes);
                    updateCounter++;    
                    counter = 0;    
                }        
            }
        }    
    }
    return 0;//we did it
}


void updateManifest(char* commit,char* project,char* num) {
    //printf("%s\n",commit);
    int length = strlen(commit);
    int mVers = atoi(num) + 1;
    char newNum[256];
    char finalVer[256];
    memset(finalVer,'\0',256);
    sprintf(finalVer,"%d ",mVers);
    memset(newNum,'\0',256);
    sprintf(newNum,"%d\n",mVers);
    char* manFile = combineString(project,"/.Manifest\0");
    int manFD = open(manFile,O_WRONLY | O_CREAT | O_TRUNC,00600);
    writeTo(manFD,newNum);
    int i;
    int counter = 0;
    int version = 0;
    char* code;
    char* filepath;
    char* shacode;
    int start = 0;
    for (i = 0; i < length; i++) {
        if (commit[i] == ' ' || commit[i] == '\n') {
            if (counter == 0) {
                version = atoi(substring(commit,start,i));
                //printf("%d\n",version);
            } else if (counter == 1) {
                code = substring(commit,start,i);
            } else if (counter == 2) {
                filepath = substring(commit,start,i);
            } else if (counter == 3) {
                shacode = substring(commit,start,i);
            }
            start = i+1;
            counter++;
        }    
        if (counter == 4) {
            int index = tableSearch(filepath);
            hashNode* temp = table->table[index];
            while (temp != NULL) {
                if (compareString(temp->filepath,filepath) == 0) {
                    temp->code = "!DL\0";
                    break;
                }
                temp = temp->next;
            }
            if (compareString(code,"!AD\0") == 0 || compareString(code,"!MD\0") == 0) {
                writeTo(manFD,finalVer);
                writeTo(manFD,"!UT\0");
                writeTo(manFD," \0");
                writeTo(manFD,filepath);
                writeTo(manFD," \0");
                writeTo(manFD,shacode);
                writeTo(manFD,"\n\0");
            }
            if (compareString(code,"!RM\0") == 0) {
                writeTo(manFD,finalVer);
                writeTo(manFD,"!UT\0");
                writeTo(manFD," \0");
                writeTo(manFD,filepath);
                writeTo(manFD," \0");
                writeTo(manFD,"DELETE\0");
                writeTo(manFD,"\n\0");
            }
            start = i+1;
            counter = 0;
                
        }
    }
        
    for (i = 0; i < table->size; i++) {
        hashNode* temp2 = table->table[i];
        while (temp2 != NULL) {
            if (compareString(temp2->code,"!DL\0") != 0) {
                writeTo(manFD,temp2->version);
                writeTo(manFD," \0");
                writeTo(manFD,"!UT\0");
                writeTo(manFD," \0");
                writeTo(manFD,temp2->filepath);
                writeTo(manFD," \0");
                writeTo(manFD,temp2->shacode);
                writeTo(manFD,"\n\0");
            }
            temp2 = temp2->next;
        }    
    }
}

char* upgrade(char* project,int updateFD) {
    char* ret = "";
    int t = lseek(updateFD,0,SEEK_END);
    char* updfile = combineString(project,"/.Update\0");
    if (t <= 1) {
        ret = combineString(ret,"Up to date\n");
        //printf("EMPTY REMOVE THING\n");
        remove(updfile);
        return ret;    
    }
    lseek(updateFD,0,SEEK_SET);
    char* updText = readConf(updateFD);
    //printf("updText: %s\n",updText);
    int i;
    int counter = 0;
    int start = 0;
    char* version = "";
    char* code = "";
    char* filepath = "";
    char* shacode = "";
    for (i = 0; i < strlen(updText); i++) {
        if (updText[i] == ' ') {
            if (counter == 0) {
                version = substring(updText,start,i);
            } else if (counter == 1) {
                code = substring(updText,start,i);
            } else if (counter == 2) {
                filepath = substring(updText,start,i);
            }
            //printf("WORD: %s\n",substring(updText,start,i));
            counter++;
            start = i+1;
        } else if (updText[i] == '\n' && counter == 3) {
            shacode = substring(updText,start,i);
            start = i+1;
            counter = 0;
            if (compareString(code,"!RM\0") == 0) {
                remove(filepath);    
            } else if (compareString(code,"!AD\0") == 0 || compareString(code,"!MD\0") == 0) {
                ret = combineString(ret,filepath);
                ret = combineString(ret," \0");
            }
        }    
    }
    remove(updfile);
    return ret;
}

int upgradeParse(char* sendFile) {
    sendFile = substring(sendFile,9,-1);
    int length = strlen(sendFile);
    int i;
    int start = 0;
    int counter = 0;
    int nameLen = 0;
    int byteLen = 0;
    for ( i = 0; i < length; i++) {
        if (sendFile[i] == ':')    {
            if (counter == 0) {
                nameLen = atoi(substring(sendFile,start,i));
            } else if (counter == 1) {
                byteLen = atoi(substring(sendFile,start,i));
            }
            counter++;
            start = i+1;    
        } else if (sendFile[i] != ':' && counter == 2) {
            char* fName = substring(sendFile,start,start+nameLen);
            char* fBytes = substring(sendFile,(start+nameLen),(start+nameLen+byteLen));
            counter = 0;
            start = start + nameLen+byteLen;
            i = start;
            int fd = open(fName,O_WRONLY | O_CREAT | O_TRUNC,00600);
            writeTo(fd,fBytes);
            close(fd);
            //printf("fName: %s, fBytes: %s\n",fName,fBytes);
        }
    }
    return 1;
}

void writeTo(int fd, char* word) {
    int bytesWritten = 0;
    int bytestoWrite = strlen(word);
    while (bytesWritten < bytestoWrite) {
        bytesWritten = write(fd,word,bytestoWrite - bytesWritten);
    }
}




