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



typedef struct{
  char code[20];
  char params[2][30];
} command;

void sig_chld(int singno);
command convertRequestToCommand(char *request);
void processCommand(command cmd);

int main(){
	
	conn=mysql_init(NULL);
	
	
	if (!(mysql_real_connect(conn,host,user,pass,dbname,port,unix_socket,flag)))
	{
		printf("\nError: %s [%d]\n",mysql_error(conn), mysql_errno(conn));
		exit(1);
	}

	printf("Connection Successful\n");
	pid_t pid;
    int listenSock, connectSock, n;
    char request[MAXLINE];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clilen;
	command cmd;
    if((listenSock = socket(AF_INET,SOCK_STREAM,0)) < 0){
		printf("Loi tao socket\n");
        exit(1);
	}

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERV_PORT);

    if(bind(listenSock,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
		printf("Loi bind\n");
        exit(2);
	}
    listen(listenSock,LISTENQ);
    clilen = sizeof(clientAddr);

    while (1) {
        connectSock = accept (listenSock, (struct sockaddr *) &clientAddr, &clilen);
        if((pid=fork()) == 0){
			close(listenSock);
			while ((n = recv(connectSock, request, MAXLINE,0)) > 0)  {
 				cmd = convertRequestToCommand(request);
				processCommand(cmd);
			}
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
	while(token != NULL){
		token = strtok(NULL, "|");
		if(token != NULL){
			strcpy(cmd.params[i++],token);
		}
	}

	return cmd;
}

void processCommand(command cmd){
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[1000];
	if(strcmp(cmd.code,"LOGIN") == 0){
	    int i;
	    sprintf(query, "Select count(*),username,password FROM user WHERE username ='%s'",cmd.params[0]);
	    if (mysql_query(conn, query)) {
	        mysql_close(conn);
	        return;
	    }
	    result = mysql_store_result(conn);
	    if(result == NULL) {
	        mysql_close(conn);
	        return;
	    }
	    while ((row = mysql_fetch_row(result))) {
	    	if (atoi(row[0])<1) {
	    		
	    	} else {
	    		if (strcmp(cmd.params[1],row[2])==0)
	    		{
	    			printf("good\n");
	    		} else {
	    			printf("not good\n");
	    		}
	    	}
	    }
	    
	}
	else if(strcmp(cmd.code,"SIGNUP") == 0){
		if (isUserExisted(cmd.params[0])==1){
			printf("lmao who cares haha xd\n");
			return;
		}
	    sprintf(query, "insert into user(username,password) values ('%s','%s')",cmd.params[0],cmd.params[1]);
	    if (mysql_query(conn, query)) {
	        mysql_close(conn);
	        return;
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
        return;
    }
    result = mysql_store_result(conn);
    if(result == NULL) {
        mysql_close(conn);
        return;
    }
    while ((row = mysql_fetch_row(result))) {
    	if (atoi(row[0])<1) {
    		return 0;
    	} else {
    		return 1;
    	}
    }
}