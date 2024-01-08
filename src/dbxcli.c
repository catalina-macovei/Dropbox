#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

// pentru meniu
#define MAX_LENGTH 100  

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*  For disk usage     */

long calculateSize(const char *path) {
    struct stat st;     // stat - o structura care contine info despre file
    long size = 0;

    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode)) {      // flag - daca e regular file
            size = st.st_size;
        } else if (S_ISDIR(st.st_mode)) {   // flag - daca e director
            DIR *dir = opendir(path);
            if (dir != NULL) {
                struct dirent *entry;   // var struct dirent cu informatii caracteristice directorului
                while ((entry = readdir(dir)) != NULL) {    // citeste toate intrarile in directoare/subdirectoare
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // '.' dir curent,'..' parinte director
                        char filePath[1024];
                        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name); // verific filepath, base case
                        size += calculateSize(filePath);  // apel recursiv de calculare dimensiunii in baza filepathului construit
                    }
                }
                closedir(dir);
            }
        }
    }

    return size;
}

void du(const char *basePath) {
    printf("Disk usage:\n");
    long totalSize = calculateSize(basePath);
    printf("%ld\t%s\n", totalSize, basePath);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
/*  For search directory     */

#define MAX_FILENAME_LENGTH 256

// exemplu utilizare : ./dbxcli ../src list
int searchDirectory(const char *basePath, const char *item) {
    DIR *dir;
    // = opendir(basePath);
    if ((dir = opendir(basePath)) == NULL) {
        perror("opendir");
        return 1;
    }
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            char path[MAX_FILENAME_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);

            struct stat statbuf;
            if (stat(path, &statbuf) == 0) {
                if (( strstr(entry->d_name, item) != NULL || strcmp(entry->d_name, item) == 0)) { // S_ISREG(statbuf.st_mode) &&
                    printf("%s\n", path);
                } else if (S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    searchDirectory(path, item);
                }
            }
        }
        closedir(dir);
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
/*  Create directory     */

int createDirectory(const char *dirName) {
    if (mkdir(dirName, 0777) == -1) {       // 0777 permisiunile: read, write, and execute permisiuni pentru owner, group -> codul e in sis octal
        perror("mkdir");
        return 1;
    }

    printf("Directory '%s' created successfully.\n", dirName);
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      LIST DIRECTORY      */

int listDirectory(const char *dirname) {
    DIR *dp;
    struct dirent *dirp;

    if ((dp = opendir(dirname)) == NULL) {
        perror("opendir");
        return 1;
    }

    while ((dirp = readdir(dp)) != NULL) {
        printf("%s\n", dirp->d_name);
    }

    closedir(dp);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      UTILITIES      */

void display_menu() {
    printf("Welcome to DropBox!  *__* \n"); 
    printf("Commands:\n1.du - display disk usage\n2.mkdir - create a directory\n3.search - search for an item\n4.ls - list items\n0-EXIT\n");
}

char (*splitIntoCommands(char *input))[MAX_LENGTH] {        // poate fi refolosita pentru a face split la o linie de siruri de carctere

    char (*commands)[MAX_LENGTH] = malloc(MAX_LENGTH * sizeof(*commands));
    if (commands == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    char *token;
    int count = 0;

    token = strtok(input, " ");
    while (token != NULL && count < MAX_LENGTH - 1) {
        strcpy(commands[count], token);
        count++;
        token = strtok(NULL, " ");
    }
    commands[count][0] = '\0';  // adaug caract null la final

    return commands;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      MAIN      */

int main(int argc, char *argv[]) {

    display_menu();

    while (1) {
        char input[MAX_LENGTH];
        char (*commands)[MAX_LENGTH];

        printf("\nEnter a command $: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        commands = splitIntoCommands(input);
  
        char option = commands[0][0];

        if (option == '1') 
            {
                const char *path = malloc(strlen(commands[1]) + 1);

                if (path != NULL) {
                    strcpy(path, commands[1]);
                } else {

                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }

                if (strlen(path) < 2) {
                    printf("\nUsage du: <command_number> <file_name || directory_name>");
                } else {
                    du(path);
                }

                free(path);
            } 
            else if (option == '2')
            {
                const char *dirName = malloc(strlen(commands[1]) + 1);

                if (dirName != NULL) {
                    strcpy(dirName, commands[1]);
                }
                else 
                {
                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }
                if (strlen(dirName) < 2) {
                    printf("\nUsage mkdir: <command_number> <directory_name>");
                } else {
                    createDirectory(dirName);
                }

                free(dirName);
            }
            else if (option == '3')
            {
                const char *path = malloc(strlen(commands[1]) + 1);
                const char *item = malloc(strlen(commands[2]) + 1);

                if (path != NULL && item != NULL) {
                    strcpy(path, commands[1]);
                    strcpy(item, commands[2]);    
                } else {
                    //printf("Usage: %s <directory_path> <item>\n", argv[0]);
                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }
                if (strlen(path) < 2 || strlen(item) < 1) {
                    printf("\nUsage search: <command_number> <path> <search_text>");
                } else {
                    searchDirectory(path, item);
                }
                free(path);
                free(item);
            }
            else if (option == '4')
            {
                const char *name = malloc(strlen(commands[1]) + 1);

                if (name != NULL) {
                    strcpy(name, commands[1]);
                } else {
                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }
                if (strlen(name) < 2) {
                    printf("\nUsage ls: <command_number> <path>");
                } else {
                    listDirectory(name);
                }
                free(name);
            }else if (option == '0') {
                printf("\n!!! Exited from Dropbox\n");
                break;
            } else {
                printf("\n!!! Not a command. Try a number from the left of your desired command.\n");
            }

            free(commands);
    }
               
    exit(0); 
}