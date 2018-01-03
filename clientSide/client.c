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
        char params[2][256];
} command;


#define MAXLINE 1024
#define SERV_PORT 3000

int sockfd;
command cmd;
char username[30];
char currentPath[256];
char currentContent[256];

int togglePrivacy();
void showMenuLogin();
void showLoginForm();
void searchAccessableFiles();
void makeCommand(char* command, char* code, char* param1, char* param2);
int makeFolderForm(char *messageHeader);
void showSignupForm();
void showMenuFunction();
void getResponse();
void updateCurrentContent();
command convertReponseToCommand(char *response);
int sharePrivilege();
int deletePrivilege();
int getFileSize(FILE *f){
        int size;
        fseek(f, 0L, SEEK_END);
        size=ftell(f);
        fseek(f, 0L, SEEK_SET);
        return size;
}

char* getFileName(const char *filename) {
        char *dot = strrchr(filename, '/');
        if(!dot || dot == filename) return "";
        return dot + 1;
}

int main(int argc, char **argv){
        sockfd = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in serverAddr;

        if (argv[1]==NULL){
          printf("Error: Server address is missing\n");
          exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERV_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

        if(connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))!= 0) {
                printf("Error: Error in connecting to server\n");
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
        printf("Username: ");
        gets(username);
        printf("Password: ");
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
                updateCurrentContent();
                choice = 0;
                printf("\n\n");
                printf("---------MENU----------\n");
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
                printf("6. Delete folder or file\n");
                printf("7. Back\n");
                printf("8. Search\n");
                printf("9. Download file by path\n");
                printf("10. Share privilege\n");
                printf("11. Delete privilege\n");
                printf("12. Logout\n");
                printf("Your choice: ");
                while (choice == 0) {
                        if(scanf("%d",&choice) < 1) {
                                choice = 0;
                        }
                        if(choice < 1 || choice > 12) {
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
                                printf("Error: %s\n",cmd.params[0]);
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
                                printf("Error: %s\n",cmd.params[0]);
                        }
                        break;
                case 3: {
                        char fileNameOrPath[100];
                        char fileName[20];
                        char fileSizeStr[12];
                        char buf[MAXLINE];
                        char command[100];
                        char path[256];
                        int totalSent = 0;
                        int sent;
                        int read;
                        int n;
                        FILE *fptr;
                        printf("Enter file name or file path: ");
                        scanf("%s", fileNameOrPath);
                        printf("Raw file name/file path: %s\n", fileNameOrPath);
                        strcpy(fileName, getFileName(fileNameOrPath));
                        if (strcmp(fileName, "") == 0){
                          strcpy(fileName, fileNameOrPath);
                        }
                        printf("Processed file name: %s\n", fileName);
                        fptr = fopen(fileNameOrPath, "rb");
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
                                bzero(buf, sizeof(buf));
                                n = recv(sockfd, buf, MAXLINE, 0);
                                buf[n] = '\0';
                        }
                        printf("Server is ready. Begin uploading...\n");
                        while(totalSent < fileSizeNo) {
                                read = fread(buf, 1, MAXLINE, fptr);
                                sent = send(sockfd, buf, read, 0);
                                totalSent += sent;
                                printf("Sent: %d byte(s)\tTotal: %d byte(s)\tRemaining: %d\n", sent, totalSent, fileSizeNo - totalSent);
                                bzero(buf, sizeof(buf));
                                while(strcmp(buf, "READY") != 0) {
                                        bzero(buf, sizeof(buf));
                                        n = recv(sockfd, buf, MAXLINE, 0);
                                        buf[n] = '\0';
                                }
                                bzero(buf, sizeof(buf));
                        }
                        fclose(fptr);
                        if (totalSent != fileSizeNo){
                          printf("File uploading unsuccessful\n");
                        }
                        else if (totalSent == fileSizeNo){
                          printf("File uploading successful\n");
                        }
                        getResponse();
                } break;
                case 4: {
                        int fileSize;
                        int received;
                        int totalReceived = 0;
                        char fileName[200];
                        char buf[MAXLINE];
                        char command[100];
                        char path[256];
                        int n;
                        printf("Enter file name: ");
                        scanf("%s", fileName);
                        if(strstr(currentContent, fileName) == NULL) {
                          printf("File not found\n");
                          break;
                        }
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
                        send(sockfd, "READY", 5, 0);
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
                                send(sockfd, "READY", 5, 0);
                        }
                        fclose(fptr);
                        if (totalReceived != fileSize){
                          printf("File downloading unsuccessful\n");
                        }
                        else if (totalReceived == fileSize){
                          printf("File downloading successful\n");
                        }
                        getResponse();
                } break;
                case 5:
                        if(togglePrivacy()) {
                                getResponse();
                                if (atoi(cmd.code) == 201) {
                                        printf("Changed from public to private successfully\n");
                                } else if (atoi(cmd.code) == 202) {
                                        printf("Changed from private to public successfully\n");
                                }
                        }
                        break;
                case 6:
                        if(makeFolderForm("DELETEFOLDER")==1){
                                getResponse();
                                if (atoi(cmd.code) == 201) {
                                        printf("Delete successfully\n");
                                        strcpy(currentContent,cmd.params[0]);
                                }
                                else {
                                        printf("Error: %s\n",cmd.params[0]);
                                }
                        }
                        break;
                case 7:
                        if(makeFolderForm("BACKFOLDER")){
                                getResponse();
                                strcpy(currentContent,cmd.params[1]);
                                strcpy(currentPath, cmd.params[0]);
                        }

                        break;
                case 8:
                        searchAccessableFiles();
                        getResponse();
                        if (atoi(cmd.code) == 201) {
                                printf("Result:\n%s",cmd.params[0]);
                        } else if (atoi(cmd.code) == 401) {
                                printf("No such file.\n");
                        }
                        break;
                case 9: {
                        int fileSize;
                        int received;
                        int totalReceived = 0;
                        char filePath[100];
                        char fileName[20];
                        char buf[MAXLINE];
                        char command[100];
                        printf("Enter file path: ");
                        scanf("%s", filePath);
                        printf("Raw file path: %s\n", filePath);
                        strcpy(fileName, getFileName(filePath));
                        if (strcmp(fileName, "") == 0){
                          strcpy(fileName, filePath);
                        }
                        printf("Processed file name: %s\n", fileName);
                        makeCommand(command,"SEARCH",fileName,NULL);
                        send (sockfd,command,sizeof(command),0);
                        getResponse();
                        if (atoi(cmd.code) != 201) {
                                printf("Error: No such file or unauthorized\n");
                                break;
                        }
                        FILE* fptr = fopen(fileName, "wb");
                        if (fptr == NULL) {
                                perror("Can't open file");
                                exit(0);
                        }
                        makeCommand(command, "DOWNLOADBYPATH", filePath, NULL);
                        send(sockfd, command, sizeof(command), 0);
                        recv(sockfd, buf, MAXLINE, 0);
                        fileSize = atoi(buf);
                        printf("File size: %d byte(s)\n", fileSize);
                        bzero(buf, sizeof(buf));
                        send(sockfd, "READY", 5, 0);
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
                                send(sockfd, "READY", 5, 0);
                        }
                        fclose(fptr);
                        if (totalReceived != fileSize){
                          printf("File downloading unsuccessful\n");
                        }
                        else if (totalReceived == fileSize){
                          printf("File downloading successful\n");
                        }
                        getResponse();
                } break;
                case 10:
                        if(sharePrivilege()) {
                                getResponse();
                                if (atoi(cmd.code) == 401) {
                                        printf("No such username.\n");
                                } else if (atoi(cmd.code) == 402) {
                                        printf("Can not share public file(s).\n");
                                } else if (atoi(cmd.code) == 201) {
                                        printf("Shared privilege succesfully.\n");
                                } else if (atoi(cmd.code) == 403) {
                                        printf("This file is already shared with this user\n");
                                }
                        }
                        break;
                case 11:
                        if(deletePrivilege()) {
                                getResponse();
                                if (atoi(cmd.code) == 401) {
                                        printf("No such username.\n");
                                } else if (atoi(cmd.code) == 402) {
                                        printf("This is public file.\n");
                                } else if (atoi(cmd.code) == 201) {
                                        printf("Delete privilege succesfully.\n");
                                } else if (atoi(cmd.code) == 403) {
                                        printf("This file is not shared with this user\n");
                                }
                        }
                        break;
                case 12:
                        printf("Bye\n");
                        break;
                }
                if (choice == 12) {
                        break;
                }
        }
}

void getResponse(){
        char serverResponse[MAXLINE];
        int n = recv(sockfd, serverResponse, MAXLINE, 0);
        if (n == 0) {
                perror("The server terminated prematurely");
                exit(4);
        }
        serverResponse[n] = '\0';
        cmd = convertReponseToCommand(serverResponse);
}

int makeFolderForm(char *messageHeader){
        char folderName[100];
        char command[100];
        char isSure[10];
        char temp[100];
        sprintf(temp, "folder/%s",username);
        if(strcmp(messageHeader,"BACKFOLDER")==0) {
                if(strcmp(currentPath,temp) == 0){
                        printf("%s\n","Cant back" );
                        return 0;
                }
                makeCommand(command,messageHeader, " ", currentPath);
                send (sockfd,command,sizeof(command),0);
                return 1;
        }
        if(strcmp(messageHeader,"DELETEFOLDER")==0) {
                printf("Enter folder or filename: ");
                gets(folderName);
                if(strstr(currentContent, folderName) != NULL){
                        printf("Are you sure?(Y/N): ");
                        gets(isSure);
                        if (strcmp(isSure,"Y") == 0){
                                makeCommand(command,messageHeader, folderName, currentPath);
                                send (sockfd,command,sizeof(command),0);
                                return 1;
                        }
                        return 0;
                }
                printf("%s\n","Not exist");
                return 0;

        }
        printf("Enter folder name: ");
        gets(folderName);
        makeCommand(command,messageHeader, folderName, currentPath);
        send (sockfd,command,sizeof(command),0);
        return 1;
}



int togglePrivacy() {
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
void searchAccessableFiles() {
        char enteredFileName[200];
        char command[200];
        printf("Enter filename: ");
        gets(enteredFileName);
        makeCommand(command,"SEARCH",enteredFileName,NULL);
        send (sockfd,command,sizeof(command),0);
}

void updateCurrentContent(){
        char command[200];
        makeCommand(command,"UPDATE",currentPath,NULL);
        send (sockfd,command,sizeof(command),0);
        getResponse();
        if (atoi(cmd.code) == 201) {
                strcpy(currentContent,cmd.params[0]);
        }
        else {
                printf("Error: %s\n",cmd.params[0]);
        }
}

int sharePrivilege() {
        char enteredFileName[200];
        char enteredUserName[200];
        char filePath[256];
        char command[200];
        printf("Enter filename: ");
        gets(enteredFileName);
        if(strstr(currentContent, enteredFileName) != NULL) {
                sprintf(filePath, "%s/%s",currentPath,enteredFileName);
                printf("Enter username: ");
                gets(enteredUserName);
                makeCommand(command,"SHARE",filePath,enteredUserName);
                send (sockfd,command,sizeof(command),0);
                return 1;
        } else {
                printf("Not exist\n");
                return 0;
        }
}



int deletePrivilege() {
        char enteredFileName[200];
        char enteredUserName[200];
        char filePath[256];
        char command[200];
        printf("Enter filename: ");
        gets(enteredFileName);
        if(strstr(currentContent, enteredFileName) != NULL) {
                sprintf(filePath, "%s/%s",currentPath,enteredFileName);
                printf("Enter username: ");
                gets(enteredUserName);
                makeCommand(command,"DELETESHARE",filePath,enteredUserName);
                send (sockfd,command,sizeof(command),0);
                return 1;
        } else {
                printf("Not exist\n");
                return 0;
        }
}
