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
char currentPath[256];
char currentContent[256];

void showMenuLogin();
void showLoginForm();

void makeCommand(char* command, char* code, char* param1, char* param2);
void makeFolderForm(char *messageHeader);
void showSignupForm();
void showMenuFunction();
void getResponse();
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
                        getResponse();
                        if (atoi(cmd.code) == 201) {
                                strcpy(currentPath,cmd.params[0]);
                                strcpy(currentContent,cmd.params[1]);
                                showMenuFunction();
                        }
                        else {
                                printf("%s\n",cmd.params[0]);
                        }
                        break;
                case 2:
                        showSignupForm();
                        getResponse();
                        if (atoi(cmd.code) == 200) {
                                printf("Sign up successfully\n");
                        }
                        else {
                                printf("%s\n",cmd.params[0]);
                        }
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
                printf("\n\n");
                printf("Hello, %s\n", username);
                printf("Your current folder: %s\n",currentPath);
                if(strcmp(currentContent,"empty") == 0) {
                        printf("This folder is empty\n");
                }
                else{
                        printf("Your content in this folder: %s\n",currentContent);
                }
                printf("What do you want to do:\n");
                printf("1. Create new folder\n");
                printf("2. Enter folder\n");
                printf("3. Upload file\n");
                printf("4. Download file\n");
                printf("5. Change privacy\n");
                printf("6. Delete folder\n");
                printf("7. Back\n");
                printf("8. Logout\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 8) {
                                choice = 0;
                                printf("Invalid choice!\n");
                                printf("Enter again:");
                        }
                        while((c = getchar())!='\n') ;
                }

                switch (choice) {
                case 1:
                        makeFolderForm("MAKEFOLDER");
                        getResponse();
                        if (atoi(cmd.code) == 201) {
                                printf("Create folder successfully\n");
                                strcpy(currentContent,cmd.params[0]);
                        }
                        else {
                                printf("Error :%s\n",cmd.params[0]);
                        }
                        break;
                case 2:
                        makeFolderForm("ENTERFOLDER");
                        getResponse();
                        if (atoi(cmd.code) == 201) {
                                printf("Enter folder successfully\n");
                                strcpy(currentContent,cmd.params[1]);
                                strcpy(currentPath, cmd.params[0]);
                        }
                        else {
                                printf("Error :%s\n",cmd.params[0]);
                        }
                        break;
                case 3: {
                        char fileName[20];
                        char fileSizeStr[12];
                        char buf[MAXLINE];
                        char command[100];
                        char path[256];
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
                                break;
                        }
                        makeCommand(command, "UPLOAD", currentPath, fileName); // makeCommand to send
                        
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
                                printf("\rSent: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d", sent, totalSent, fileSizeNo - totalSent);
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
                        char fileName[200];
                        char buf[MAXLINE];
                        char command[100];
                        char path[256];
                        printf("Enter file name: ");
                        scanf("%s", fileName);
                        printf("File name: %s\n", fileName);
                        FILE* fptr = fopen(fileName, "wb");
                        if (fptr == NULL) {
                                perror("Can't open file");
                                exit(0);
                        }
                        snprintf(path, sizeof(path), "%s/%s", currentPath, fileName);
                        makeCommand(command, "DOWNLOAD", path, NULL);
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
                        if(togglePrivacy()){
                                getResponse();
                                if (atoi(cmd.code) == 201) {
                                        printf("Changed from public to private successfully\n");
                                } else if (atoi(cmd.code) == 202) {
                                        printf("Changed from private to public successfully\n");
                                }
                        } 
                        break;
                case 6:
                        makeFolderForm("DELETEFOLDER");
                        getResponse();
                        if (atoi(cmd.code) == 201) {
                                printf("Delele folder successfully\n");
                                strcpy(currentContent,cmd.params[0]);
                        }
                        else {
                                printf("Error :%s\n",cmd.params[0]);
                        }
                        break;
                case 7:
                        makeFolderForm("BACKFOLDER");
                        getResponse();
                        strcpy(currentContent,cmd.params[1]);
                        strcpy(currentPath, cmd.params[0]);
                        break;
                case 8:
                        printf("Bye\n");
                        break;
                }

                if (choice == 8) {
                        break;
                }
        }
}

void getResponse(){
        if (recv(sockfd, serverResponse, MAXLINE, 0) == 0) {
                perror("The server terminated prematurely");
                exit(4);
        }
        cmd = convertReponseToCommand(serverResponse);
}

void makeFolderForm(char *messageHeader){
        char folderName[100];
        char command[100];
        if(strcmp(messageHeader,"BACKFOLDER")==0) {
                makeCommand(command,messageHeader, " ", currentPath);
                send (sockfd,command,sizeof(command),0);
                return;
        }
        printf("Enter folder name:\n");
        gets(folderName);
        makeCommand(command,messageHeader, folderName, currentPath);
        send (sockfd,command,sizeof(command),0);
}
int togglePrivacy() {
        // enter file name,makeCommand(TOGGLE,currentPath,fileName),client receive response
        // if currentContent include filename then makeCommand
        char enteredFileName[200];
        char command[200];
        printf("Enter filename:");
        gets(enteredFileName);
        if(strstr(currentContent, enteredFileName) != NULL) {   
                makeCommand(command,"TOGGLE",currentPath,enteredFileName);
                send (sockfd,command,sizeof(command),0);
                return 1;
        } else {
                printf("Not exist\n");
                return 0;
        }
}