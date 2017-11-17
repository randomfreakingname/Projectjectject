#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct{
  char code[20];
  char params[2][30];
} command;


#define MAXLINE 4096
#define SERV_PORT 3000

int sockfd;
char serverResponse[MAXLINE];
command cmd;
char username[30];

void showMenuLogin();
void showLoginForm();
void makeLoginCommand(char*command,char *username, char *password);
void makeSignupCommand(char*command,char *username, char *password);
void showSignupForm();
void showMenuFunction();
command convertReponseToCommand(char *response);

int main(){
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERV_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))!= 0){
		printf("Loi: Ket noi den server that bai\n");
		exit(1);
	}
    showMenuLogin();
    close(sockfd);
}

void showMenuLogin(){
    int choice;
    char c;
    while (1){
        choice = 0;
        printf("----Welcome-----\n");
        printf("Please login or signup to use our service\n");
        printf("1.Login\n");
        printf("2.Signup\n");
        printf("3.Exit\n");
        printf("Your choice:");
        while (choice == 0) {
            if(scanf("%d",&choice) < 1){
                choice = 0;
            }
            if(choice < 1 || choice > 3){
                choice = 0;
                printf("Invalid choice!\n");
                printf("Enter again:");
            }
            while((c = getchar())!='\n');
        }

        switch (choice) {
            case 1:
                showLoginForm();
                if (recv(sockfd, serverResponse, MAXLINE, 0) == 0){
                    perror("The server terminated prematurely");
                    exit(4);
                }
                cmd = convertReponseToCommand(serverResponse);
                if (atoi(cmd.code) == 200) {
                    showMenuFunction();
                }
                else {
                    printf("%s\n",cmd.params[0]);
                }
                break;
            case 2:
                showSignupForm();
                break;
            default:
                exit(0);
        }
    }
}

void showLoginForm(){
    char password[30];
    char command[100];
    printf("Username:\n");
    gets(username);
    printf("Password:\n");
    gets(password);
    makeLoginCommand(command,username, password);
    send (sockfd,command,sizeof(command),0);
}

void makeLoginCommand(char *command, char *username, char *password){
    strcpy(command,"LOGIN|");
    strcat(command,username);
    strcat(command,"|");
    strcat(command,password);
    strcat(command,"|");
}


void showSignupForm(){
    char password[30];
    char command[100];
    printf("Username:\n");
    gets(username);
    printf("Password:\n");
    gets(password);
    makeSignupCommand(command,username, password);
    send (sockfd,command,sizeof(command),0);
}

void makeSignupCommand(char *command, char *username, char *password){
    strcpy(command,"SIGNUP|");
    strcat(command,username);
    strcat(command,"|");
    strcat(command,password);
    strcat(command,"|");
}

command convertReponseToCommand(char *response){
	char copy[100];
	strcpy(copy,response);
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


void showMenuFunction(){
    int choice;
    char c;
    while (1){
        choice = 0;
        printf("Hello %s\n", username);
        printf("Here is your function:\n");
        printf("1.Create new folder\n");
        printf("2.See your folder\n");
        printf("3.Upload file\n");
        printf("4.Delete file\n");
        printf("5.Logout\n");
        printf("Your choice:");
        while (choice == 0) {
            if(scanf("%d",&choice) < 1){
                choice = 0;
            }
            if(choice < 1 || choice > 5){
                choice = 0;
                printf("Invalid choice!\n");
                printf("Enter again:");
            }
            while((c = getchar())!='\n');
        }

        switch (choice) {
            case 1:
                printf("code for create new folder here\n");
                break;
            case 2:
                printf("code for see folder here\n");
                break;
            case 3:
                printf("code for upfile here\n");
                break;
            case 4:
                printf("code for delete file here\n");
                break;
            case 5:
                printf("code for logout here\n");
                break;
        }
        if (choice == 5){
            break;
        }
    }
}
