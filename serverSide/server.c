#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <errno.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8 /*maximum number of client connections*/


static char *host = "localhost";
static char *user = "root";
static char *pass = "123456";
static char *dbname = "test";
unsigned int port=3306;
static char *unix_socket= NULL;
unsigned int flag=0;
MYSQL *conn;
int listenSock, connectSock, n;
pid_t pid;
char request[MAXLINE];
struct sockaddr_in serverAddr, clientAddr;
socklen_t clilen;

typedef struct {
        char username[30];
        int id;
        char path[256];
} User;


typedef struct {
        char code[20];
        char params[2][30];
} command;

User currentUser;

void sig_chld(int singno);
command convertRequestToCommand(char *request);
char* processCommand(command cmd);
int isUserExisted(char* username);
int makeNewFolder(char * folderName);
void resetCurrentUser();
char* showContentFolder(char *folderPath);
int deleteFolder(char *folderName);
int operateFolder(char *folderName, char *operation);
char *updatePath();
void backFolder(char *path);
int isFilePublic(char* filepath);
int getUserId(char* username);
int getFileId(char* filepath);
int deleteFile(char *fileName);
command cmd;

int getFileSize(FILE *f){
        int size;
        fseek(f, 0L, SEEK_END);
        size=ftell(f);
        fseek(f, 0L, SEEK_SET);
        return size;
}

int main(){
        conn=mysql_init(NULL);
        if (!(mysql_real_connect(conn,host,user,pass,dbname,port,unix_socket,flag)))
        {
                printf("\nError: %s [%d]\n",mysql_error(conn), mysql_errno(conn));
                exit(1);
        }
        currentUser.id = 0; //no current user

        if((listenSock = socket(AF_INET,SOCK_STREAM,0)) < 0) {
                printf("Loi tao socket\n");
                exit(1);
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(SERV_PORT);

        int enable = 1;
        if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");

        if(bind(listenSock,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
                printf("Loi bind\n");
                exit(2);
        }
        listen(listenSock,LISTENQ);
        clilen = sizeof(clientAddr);
        char* message;
        while (1) {
                connectSock = accept (listenSock, (struct sockaddr *) &clientAddr, &clilen);
                if((pid=fork()) == 0) {
                        close(listenSock);
                        while ((n = recv(connectSock, request, MAXLINE,0)) > 0)  {
                                cmd = convertRequestToCommand(request);
                                message = processCommand(cmd);
                                if (message != NULL) {
                                        send(connectSock, message, MAXLINE, 0);
                                }
                        }
                        resetCurrentUser();
                        close(connectSock);
                        exit(0);
                }
                signal(SIGCHLD,sig_chld);
                close(connectSock);
        }
}



void sig_chld(int singno){
        pid_t pid;
        int stat;
        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
                printf("child %d terminated\n", pid);
        return;
}

command convertRequestToCommand(char *request){
        char copy[100];
        strcpy(copy,request);
        command cmd;
        char *token;
        int i = 0;
        token = strtok(copy, "|");
        strcpy(cmd.code, token);
        while(token != NULL) {
                token = strtok(NULL, "|");
                if(token != NULL) {
                        strcpy(cmd.params[i++],token);
                }
        }
        return cmd;
}

char* processCommand(command cmd){
        MYSQL_RES *result;
        MYSQL_ROW row;
        char* message = malloc(MAXLINE*sizeof(char));
        char query[1000];
        char temp[100];
        if(strcmp(cmd.code,"LOGIN") == 0) {
                int i;
                sprintf(query, "Select count(*),username,password,id FROM user WHERE username ='%s'",cmd.params[0]);
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        strcpy(message,"401|Cant connect to database");
                        return message;
                }
                result = mysql_store_result(conn);
                if(result == NULL) {
                        mysql_close(conn);
                        strcpy(message,"401|Cant execute query");
                        return message;
                }
                row = mysql_fetch_row(result);
                if (atoi(row[0])<1) {
                        strcpy(message,"401|Cant find username");
                } else {
                        if (strcmp(cmd.params[1],row[2])==0)
                        {
                                strcpy(message,"201|");
                                strcpy(currentUser.username,cmd.params[0]);
                                currentUser.id = atoi(row[3]);
                                char temp[256] = "folder/";
                                strcat(temp,currentUser.username);
                                strcat(message,temp);
                                strcat(message,"|");
                                strcat(message,showContentFolder(temp));
                                strcat(message,"|");
                        } else {
                                strcpy(message,"401|Wrong password");
                        }
                }
                return message;
        }
        else if(strcmp(cmd.code,"SIGNUP") == 0) {
                if (isUserExisted(cmd.params[0])==1) {
                        strcpy(message,"401|username is already existed");
                        return message;
                }
                sprintf(query, "insert into user(username,password) values ('%s','%s')",cmd.params[0],cmd.params[1]);
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        strcpy(message,"401|Cant connect to database");
                        return message;
                }
                strcpy(message,"200|");
                char temp[100] = "folder/";
                strcat(temp,cmd.params[0]);
                makeNewFolder(temp);
                return message;
        }else if (strcmp(cmd.code, "UPLOAD") == 0) {
                int fileSize;
                int received;
                char path[MAXLINE];
                int totalReceived = 0;
                char buf[MAXLINE];
                snprintf(path, sizeof(path), "%s/%s", cmd.params[0], cmd.params[1]);
                path[strlen(path)]='\0';
                recv(connectSock, buf, MAXLINE, 0);
                fileSize = atoi(buf);
                printf("File size: %d byte(s)\n", fileSize);
                FILE* fptr = fopen(path, "wb");
                if (fptr == NULL) {
                        perror("Can't write file");
                        strcpy(message, "401|");
                        return message;
                }
                bzero(buf, sizeof(buf));
                send(connectSock, "READY", MAXLINE, 0);
                printf("Server ready to download\n");
                while(totalReceived < fileSize) {
                        received = recv(connectSock, buf, MAXLINE, 0);
                        totalReceived += received;
                        printf("Received: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d byte(s)\n", received, totalReceived, fileSize - totalReceived);
                        if(received <= 0) {
                                printf("Lost connection to client\n");
                                break;
                        }
                        fwrite(buf, 1, received, fptr);
                        bzero(buf, sizeof(buf));
                        send(connectSock, "READY", MAXLINE, 0);
                }
                fclose(fptr);
                printf("File downloading successful\n");
                char* fileName=cmd.params[1];
                int owner=currentUser.id;
                char* currentPath=cmd.params[0];

                sprintf(query, "insert into file(filename,owner,path,public) values ('%s',%d,'%s/%s',0)",fileName,currentUser.id,currentPath,fileName);
                mysql_query(conn, query);
                bzero(cmd.code, sizeof(cmd.code));
                bzero(cmd.params[0], sizeof(cmd.params[0]));
                bzero(cmd.params[1], sizeof(cmd.params[1]));
                strcpy(message, "201|");
                return message;
        }else if (strcmp(cmd.code, "DOWNLOAD") == 0) {
                printf("DOWNLOAD request from user %s\n", currentUser.username);
                char fileSizeStr[12];
                char buf[MAXLINE];
                int totalSent = 0;
                int sent;
                int read;
                FILE *fptr;
                printf("Filename: %s\n", cmd.params[0]);
                fptr = fopen(cmd.params[0], "rb");
                if (fptr == NULL) {
                        perror("Can't open file");
                        strcpy(message, "401|");
                        return message;
                }
                int fileSizeNo = getFileSize(fptr);
                sprintf(fileSizeStr, "%d", fileSizeNo);
                printf("File size : %d byte(s)\n", fileSizeNo);
                send(connectSock, fileSizeStr, strlen(fileSizeStr), 0);
                while(strcmp(buf, "READY") != 0) {
                        recv(connectSock, buf, MAXLINE, 0);
                }
                printf("Client is ready. Begin uploading...\n");
                while(totalSent < fileSizeNo) {
                        read = fread(buf, 1, MAXLINE, fptr);
                        sent = send(connectSock, buf, read, 0);
                        totalSent += sent;
                        printf("Sent: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d\n", sent, totalSent, fileSizeNo - totalSent);
                        bzero(buf, sizeof(buf));
                        while(strcmp(buf, "READY") != 0) {
                                recv(connectSock, buf, MAXLINE, 0);
                        }
                        bzero(buf, sizeof(buf));
                }
                fclose(fptr);
                printf("File uploading successful\n");
                bzero(cmd.code, sizeof(cmd.code));
                bzero(cmd.params[0], sizeof(cmd.params[0]));
                bzero(cmd.params[1], sizeof(cmd.params[1]));
                strcpy(message, "201|");
                return message;
        }else if (strcmp(cmd.code, "MAKEFOLDER") == 0) {
                strcpy(temp, cmd.params[1]);
                strcat(temp,"/");
                strcat(temp, cmd.params[0]);
                if( makeNewFolder(temp) == 1) {
                        strcpy(message,"201|");
                        strcat(message,showContentFolder(cmd.params[1]));
                        strcat(message,"|");
                }else{
                        strcpy(message,"401|Invalid folder name");
                }
                return message;
        }else if (strcmp(cmd.code, "DELETEFOLDER") == 0) {
                strcpy(temp, cmd.params[1]);
                strcat(temp,"/");
                strcat(temp, cmd.params[0]);
                if (deleteFile(temp) == 1) {
                        strcpy(message,"201|");
                        strcat(message,showContentFolder(cmd.params[1]));
                        strcat(message,"|");
                        return message;
                }
                if( deleteFolder(temp) == 1) {
                        strcpy(message,"201|");
                        strcat(message,showContentFolder(cmd.params[1]));
                        strcat(message,"|");
                }else{
                        strcpy(message,"401|Not exist");
                }
                return message;
        }else if (strcmp(cmd.code, "ENTERFOLDER") == 0) {
                strcpy(temp, cmd.params[1]);
                strcat(temp,"/");
                strcat(temp, cmd.params[0]);
                if( operateFolder(temp,"cd") == 1) {
                        strcpy(message,"201|");
                        char *newPath = updatePath();
                        strcat(message,newPath);
                        strcat(message,"|");
                        strcat(message,showContentFolder(newPath));
                        strcat(message,"|");
                }else{
                        strcpy(message,"401|Not a folder");
                }
                return message;
        }
        else if (strcmp(cmd.code, "BACKFOLDER") == 0) {
                backFolder(cmd.params[1]);
                strcpy(message,"201|");
                strcat(message,cmd.params[1]);
                strcat(message,"|");
                strcat(message,showContentFolder(cmd.params[1]));
                strcat(message,"|");
                return message;
        }else if (strcmp(cmd.code, "TOGGLE") == 0) {

                sprintf(query, "update file set public = !public where filename='%s' and path='%s/%s'",cmd.params[1],cmd.params[0],cmd.params[1]);
                mysql_query(conn, query);


                sprintf(query, "Select public FROM file WHERE filename='%s' and path='%s/%s'",cmd.params[1],cmd.params[0],cmd.params[1]);
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                }
                result = mysql_store_result(conn);
                if(result == NULL) {
                        mysql_close(conn);
                }
                row = mysql_fetch_row(result);
                if (atoi(row[0])==1) {
                        strcpy(message,"201|");
                } else {
                        strcpy(message,"202|");
                }

                return message;
        }else if (strcmp(cmd.code, "SEARCH") == 0) {
                sprintf(query, "select path from file left join privilege on file.id=privilege.fileId where (public=0 or owner=%d or userId=%d) and filename='%s'",currentUser.id,currentUser.id,cmd.params[0]);
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        return 0;
                }
                result = mysql_store_result(conn);
                if(mysql_num_rows(result)==0) {
                        strcpy(message,"401|");
                } else {
                        strcpy(message,"201|");
                        int i=1;
                        char numString[2];
                        while ((row = mysql_fetch_row(result))) {
                                sprintf(numString, "%d.", i);
                                strcat(message,numString);
                                strcat(message,row[0]);
                                strcat(message,"\n");
                                i++;
                        }
                        strcat(message,"|");
                }
                return message;
        }else if (strcmp(cmd.code, "UPDATE") == 0) {
                strcpy(message,"201|");
                strcat(message,showContentFolder(cmd.params[0]));
                strcat(message,"|");
                return message;
        }else if (strcmp(cmd.code, "DOWNLOADBYPATH") == 0) {
                sprintf(query, "select owner,public from file where path='%s'",cmd.params[0]);
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        return 0;
                }
                result = mysql_store_result(conn);
                if(mysql_num_rows(result)==0) {
                        strcpy(message,"401|");
                        return message;
                } else {
                        row = mysql_fetch_row(result);
                        if ((row[1] == 0 && currentUser.id != atoi(row[1]))) {
                                strcpy(message, "401|");
                                return message;
                        } else{
                                printf("DOWNLOADBYPATH request from user %s\n", currentUser.username);
                                char fileSizeStr[12];
                                char buf[MAXLINE];
                                int totalSent = 0;
                                int sent;
                                int read;
                                FILE *fptr;
                                printf("Filepath: %s\n", cmd.params[0]);
                                fptr = fopen(cmd.params[0], "rb");
                                if (fptr == NULL) {
                                        perror("Can't open file");
                                        strcpy(message, "401|");
                                        return message;
                                }
                                int fileSizeNo = getFileSize(fptr);
                                sprintf(fileSizeStr, "%d", fileSizeNo);
                                printf("File size : %d byte(s)\n", fileSizeNo);
                                send(connectSock, fileSizeStr, strlen(fileSizeStr), 0);
                                while(strcmp(buf, "READY") != 0) {
                                        recv(connectSock, buf, MAXLINE, 0);
                                }
                                printf("Client is ready. Begin uploading...\n");
                                while(totalSent < fileSizeNo) {
                                        read = fread(buf, 1, MAXLINE, fptr);
                                        sent = send(connectSock, buf, read, 0);
                                        totalSent += sent;
                                        printf("Sent: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d\n", sent, totalSent, fileSizeNo - totalSent);
                                        bzero(buf, sizeof(buf));
                                        while(strcmp(buf, "READY") != 0) {
                                                recv(connectSock, buf, MAXLINE, 0);
                                        }
                                        bzero(buf, sizeof(buf));
                                }
                                fclose(fptr);
                                printf("File uploading successful\n");
                                bzero(cmd.code, sizeof(cmd.code));
                                bzero(cmd.params[0], sizeof(cmd.params[0]));
                                bzero(cmd.params[1], sizeof(cmd.params[1]));
                                strcpy(message, "201|");
                                return message;
                        }
                }
        } else if (strcmp(cmd.code, "SHARE") == 0) {
                if (isUserExisted(cmd.params[1]))
                {
                        if (isFilePublic(cmd.params[0])) {
                                strcpy(message,"402|");
                        } else {
                                printf("(%d,%d)\n",getFileId(cmd.params[0]),getUserId(cmd.params[1]));
                                sprintf(query, "insert into privilege(fileId,userId) values(%d,%d)",getFileId(cmd.params[0]),getUserId(cmd.params[1]));
                                if (mysql_query(conn, query)) {
                                        mysql_close(conn);
                                        return 0;
                                }
                                strcpy(message,"201|");
                        }
                        return message;
                } else {
                        strcpy(message,"401|");
                        return message;
                }
        }
}

int isUserExisted(char* username) {
        MYSQL_RES *result;
        MYSQL_ROW row;
        char query[1000];
        int i;
        sprintf(query, "Select count(*) FROM user WHERE username ='%s'",username);
        if (mysql_query(conn, query)) {
                mysql_close(conn);
                return 0;
        }
        result = mysql_store_result(conn);
        if(result == NULL) {
                mysql_close(conn);
                return 0;
        }
        while ((row = mysql_fetch_row(result))) {
                if (atoi(row[0])<1) {
                        return 0;
                } else {
                        return 1;
                }
        }
}


int makeNewFolder(char * folderName){
        char command[256];
        sprintf(command,"mkdir %s",folderName);
        if(system(command) == 0 ) {
                return 1;
        }
        return 0;
}


void resetCurrentUser(){
        printf("%s\n","reseted" );
        currentUser.id = 0;
        strcpy(currentUser.username,"");
        strcpy(currentUser.username,"");
}

char* showContentFolder(char *folderPath){
        char *content = malloc(sizeof(char)*256);
        strcpy(content,"");
        char temp[200];
        DIR  *d;
        struct dirent *dir;
        d = opendir(folderPath);
        if (d) {
                while ((dir = readdir(d)) != NULL) {
                        if (dir->d_name[0] != '.') {
                                strcat(content,dir->d_name);
                                strcat(content,",");
                        }
                }
                closedir(d);
        }
        int x = (int)strlen(content);
        if (x == 0) {
                strcpy(content,"empty");
        }
        return content;
}

int deleteFolder(char *folderName){
        char command[256];
        sprintf(command,"rm -rf %s",folderName);
        char query[1000];
        sprintf(query, "DELETE FROM file WHERE path like '%s%%'",folderName);
        if(system(command) == 0 ) {
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        return 0;
                }
                return 1;
        }
        return 0;
}

int deleteFile(char *fileName){
        char command[256];
        sprintf(command,"rm -f %s",fileName);
        char query[1000];
        sprintf(query, "DELETE FROM file WHERE path ='%s'",fileName);
        if(system(command) == 0 ) {
                if (mysql_query(conn, query)) {
                        mysql_close(conn);
                        return 0;
                }
                return 1;
        }
        return 0;
}

int operateFolder(char *folderName, char *operation){
        char command[256];
        sprintf(command,"%s %s",operation, folderName);
        if(system(command) == 0 ) {
                return 1;
        }
        return 0;
}

char *updatePath(){
        char *path = malloc (sizeof (char) * 256);
        strcpy(path,cmd.params[1]);
        strcat(path,"/");
        strcat(path,cmd.params[0]);
        return path;
}

void backFolder(char *path){
        int i = strlen(path);
        while(1) {
                if(path[i] == '/') {
                        path[i] = '\0';
                        break;
                }
                i--;
        }
}

int isFilePublic(char* filepath) {
        MYSQL_RES *result;
        MYSQL_ROW row;
        char query[1000];
        sprintf(query, "select public from file where path='%s'",filepath);
        if (mysql_query(conn, query)) {
                mysql_close(conn);
                return 0;
        }
        result = mysql_store_result(conn);
        if(result == NULL) {
                mysql_close(conn);
                return 0;
        }
        while ((row = mysql_fetch_row(result))) {
                if (atoi(row[0])<1) {
                        return 1;
                } else {
                        return 0;
                }
        }
}

int getUserId(char* username){
        MYSQL_RES *result;
        MYSQL_ROW row;
        char query[1000];
        sprintf(query, "Select count(*),id FROM user WHERE username ='%s'",username);
        if (mysql_query(conn, query)) {
                mysql_close(conn);
                return 0;
        }
        result = mysql_store_result(conn);
        if(result == NULL) {
                mysql_close(conn);
                return 0;
        }
        while ((row = mysql_fetch_row(result))) {
                if (atoi(row[0])<1) {
                        return -1;
                } else {
                        return atoi(row[1]);
                }
        }
}

int getFileId(char* filepath){
        MYSQL_RES *result;
        MYSQL_ROW row;
        char query[1000];
        sprintf(query, "Select id FROM file WHERE path ='%s'",filepath);
        if (mysql_query(conn, query)) {
                mysql_close(conn);
                return 0;
        }
        result = mysql_store_result(conn);
        if(result == NULL) {
                mysql_close(conn);
                return 0;
        }
        while ((row = mysql_fetch_row(result))) {
                if (atoi(row[0])<1) {
                        return -1;
                } else {
                        return atoi(row[0]);
                }
        }
}
