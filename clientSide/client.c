#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct {
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
void makeLoginCommand(char*command, char *username, char *password);
void makeSignupCommand(char*command, char *username, char *password);
void makeCommand(char* command, char* code, char* param1, char* param2);
void showSignupForm();
void showMenuFunction();
command convertReponseToCommand(char *response);

int getFileSize(FILE *f){
        int size;
        fseek(f, 0L, SEEK_END);
        size=ftell(f);
        fseek(f, 0L, SEEK_SET);
        return size;
}

int main(){
        sockfd = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in serverAddr;

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERV_PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if(connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))!= 0) {
                printf("Loi: Ket noi den server that bai\n");
                exit(1);
        }
        showMenuLogin();
        close(sockfd);
}

void showMenuLogin(){
        int choice;
        char c;
        while (1) {
                choice = 0;
                printf("-----Welcome-----\n");
                printf("Please login or signup to use our service\n");
                printf("1. Login\n");
                printf("2. Signup\n");
                printf("3. Exit\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 3) {
                                choice = 0;
                                printf("Invalid choice!\n");
                                printf("Enter again: ");
                        }
                        while((c = getchar())!='\n') ;
                }

                switch (choice) {
                case 1:
                        showLoginForm();
                        if (recv(sockfd, serverResponse, MAXLINE, 0) == 0) {
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
        printf("Username: ");
        gets(username);
        printf("Password: ");
        gets(password);
        makeCommand(command,"LOGIN", username, password);
        send (sockfd,command,sizeof(command),0);
}

void makeCommand(char* command, char* code, char* param1, char* param2){
        strcpy(command, code);
        strcat(command, "|");
        if (param1 != NULL) {
                strcat(command,param1);
                strcat(command,"|");
        }
        if (param2 != NULL) {
                strcat(command,param2);
                strcat(command,"|");
        }
}

void showSignupForm(){
        char password[30];
        char command[100];
        printf("Username:\n");
        gets(username);
        printf("Password:\n");
        gets(password);
        makeCommand(command,"SIGNUP", username, password);
        send (sockfd,command,sizeof(command),0);
}

command convertReponseToCommand(char *response){
        char copy[100];
        strcpy(copy,response);
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


void showMenuFunction(){
        int choice;
        char c;
        while (1) {
                choice = 0;
                printf("Hello, %s\n", username);
                printf("What do you want to do:\n");
                printf("1. Create new folder\n");
                printf("2. See your folder\n");
                printf("3. Upload file\n");
                printf("4. Download file\n");
                printf("5. Delete file\n");
                printf("6. Logout\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 6) {
                                choice = 0;
                                printf("Invalid choice!\n");
                                printf("Enter again:");
                        }
                        while((c = getchar())!='\n') ;
                }

                switch (choice) {
                case 1:
                        printf("code for create new folder here\n");
                        break;
                case 2:
                        printf("code for see folder here\n");
                        break;
                case 3: {
                        char fileName[20];
                        char fileSizeStr[12];
                        char buf[MAXLINE];
                        char command[100];
                        int totalSent = 0;
                        int sent;
                        int read;
                        FILE *fptr;
                        printf("Enter file name: ");
                        scanf("%s", fileName);
                        printf("File name: %s\n", fileName);
                        fptr = fopen(fileName, "rb");
                        if (fptr == NULL) {
                                perror("Can't open file");
                                exit(0);
                        }
                        makeCommand(command, "UPLOAD", fileName, NULL);
                        send(sockfd, command, sizeof(command), 0);
                        int fileSizeNo = getFileSize(fptr);
                        sprintf(fileSizeStr, "%d", fileSizeNo);
                        printf("File size : %d byte(s)\n", fileSizeNo);
                        send(sockfd, fileSizeStr, strlen(fileSizeStr), 0);
                        while(strcmp(buf, "READY") != 0) {
                                recv(sockfd, buf, MAXLINE, 0);
                        }
                        printf("Server is ready. Begin uploading...\n");
                        while(totalSent < fileSizeNo) {
                                read = fread(buf, 1, MAXLINE, fptr);
                                sent = send(sockfd, buf, read, 0);
                                totalSent += sent;
                                printf("Sent: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d\n", sent, totalSent, fileSizeNo - totalSent);
                                bzero(buf, sizeof(buf));
                                while(strcmp(buf, "READY") != 0) {
                                        recv(sockfd, buf, MAXLINE, 0);
                                }
                                bzero(buf, sizeof(buf));
                        }
                        fclose(fptr);
                        printf("File uploading successful\n");
                } break;
                case 4: {
                        int fileSize;
                        int received;
                        int totalReceived = 0;
                        char fileName[20];
                        char buf[MAXLINE];
                        char command[100];
                        printf("Enter file name: ");
                        scanf("%s", fileName);
                        printf("File name: %s\n", fileName);
                        FILE* fptr = fopen(fileName, "wb");
                        if (fptr == NULL) {
                                perror("Can't open file");
                                exit(0);
                        }
                        makeCommand(command, "DOWNLOAD", fileName, NULL);
                        send(sockfd, command, sizeof(command), 0);
                        recv(sockfd, buf, MAXLINE, 0);
                        fileSize = atoi(buf);
                        printf("File size: %d byte(s)\n", fileSize);
                        bzero(buf, sizeof(buf));
                        send(sockfd, "READY", MAXLINE, 0);
                        printf("Client ready to download\n");
                        while(totalReceived < fileSize) {
                                received = recv(sockfd, buf, MAXLINE, 0);
                                totalReceived += received;
                                printf("Received: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d byte(s)\n", received, totalReceived, fileSize - totalReceived);
                                if(received <= 0) {
                                        printf("Lost connection to server\n");
                                        break;
                                }
                                fwrite(buf, 1, received, fptr);
                                bzero(buf, sizeof(buf));
                                send(sockfd, "READY", MAXLINE, 0);
                        }
                        fclose(fptr);
                        printf("File downloading successful\n");
                } break;
                case 5:
                        printf("code for delete file here\n");
                        break;
                case 6:
                        printf("code for logout here\n");
                        break;
                }
                if (choice == 6) {
                        break;
                }
        }
}