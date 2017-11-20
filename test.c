#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
char* showContentFolder(char *folderPath){
    char *content = malloc(sizeof(char)*256);
    char temp[200];
    DIR  *d;
    struct dirent *dir;
    d = opendir(folderPath);
    if (d){
        while ((dir = readdir(d)) != NULL){
        if (dir->d_name[0] != '.'){
                strcat(content,dir->d_name);
                strcat(content,",");
            }
        }
        printf("%s\n",content );
        closedir(d);
    }
    return content;
}

void main(){
    showContentFolder("folder/huhu");
}
