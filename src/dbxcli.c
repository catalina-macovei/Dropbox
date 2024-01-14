#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

// pentru meniu
#define MAX_LENGTH 100  

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*  For disk usage     */

long calculateSize(const char *path) {
    struct stat st;     // stat - o structura care contine info despre file
    long size = 0;

    if (stat(path, &st) == 0)

        if (S_ISREG(st.st_mode))       // flag - daca e regular file
            size = st.st_size;

        else if (S_ISDIR(st.st_mode)) {   // flag - daca e director
            DIR *dir = opendir(path);

            if (dir != NULL) {
                struct dirent *entry;   // var struct dirent cu informatii caracteristice directorului

                while ((entry = readdir(dir)) != NULL)    // citeste toate intrarile in directoare/subdirectoare
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // '.' dir curent,'..' parinte director
                        char filePath[1024];
                        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name); // verific filepath, base case
                        size += calculateSize(filePath);  // apel recursiv de calculare dimensiunii in baza filepathului construit
                    }
                
                closedir(dir);
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
            struct stat statbuf;

            char path[MAX_FILENAME_LENGTH];
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);

            
            if (stat(path, &statbuf) == 0) 
                if (( strstr(entry->d_name, item) != NULL || strcmp(entry->d_name, item) == 0)) // S_ISREG(statbuf.st_mode) &&
                    printf("%s\n", path);

                else if (S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                    searchDirectory(path, item);   
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
/*      COPY      */

void cp_file(char *source, char *destination)
{
    int source_fd;
    int dest_fd;

    //deschidem fisierul sursa
    if((source_fd = open(source, O_RDONLY)) < 0)
    {
        perror("open source");
        return 1;
    }

    //daca fiserul destinatie nu exista, il cream
    if((dest_fd = open(destination, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR) ) < 0)
    {
        perror("open/create destination");
        return 1;
    }

    //copiem continutul
    int n;
    char buf[8196];

    while ((n = read(source_fd, buf, sizeof(buf))) > 0 ) {
        write(dest_fd, buf, n);
    }

    //inchidem fisierele            
    close(source_fd);
    close(dest_fd);
}

void cp_dir(char *source, char *destination)
{
    //cream directorul nou
    mkdir(destination, S_IRWXU);

    //deschidem directorul sursa
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (source);

    //copiem fiecare entry
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL) 
        {
            char source_name[MAX_FILENAME_LENGTH];
            strcpy(source_name, "");
            strcat(source_name, source);
            strcat(source_name, "/");
            strcat(source_name, ep->d_name);

            char dest_name[MAX_FILENAME_LENGTH];
            strcpy(dest_name, "");
            strcat(dest_name, destination);
            strcat(dest_name, "/");
            strcat(dest_name, ep->d_name);

            //daca entry este director
            if(ep->d_type == 4 && strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..")) 
            {   
                cp_dir(source_name, dest_name);
            }
            //daca entry este regular file
            else if(ep->d_type == 8)
            {
                cp_file(source_name, dest_name);
            }
        }
                    
        closedir (dp);
    }
    else
    {
        perror ("open source directory");
        return 1;
    }
}

void cp(char (*paths)[MAX_LENGTH], char *dest_path, int sources_nr) 
{

    struct stat file_info_s;
    struct stat file_info_d;
	mode_t	m_dest;
    mode_t	m_source;

    //tipul primului parametru
    if(lstat(paths[0], &file_info_s) < 0)
	    {
			perror("lstat");
            return 1;
	    }
        
    m_source = file_info_s.st_mode;

    if(sources_nr == 1 && S_ISREG(m_source))
    {
        //dbxcli cp <source_file> <dest_file>
        cp_file(paths[0], dest_path);
    }
    else
    {
        //verificam daca destinatia exista si e director
        if(lstat(dest_path, &file_info_d) < 0)
	    {
			perror("lstat");
            return 1;
	    }

        m_dest = file_info_d.st_mode;

        if(!S_ISDIR(m_dest)) 
        {
            printf("%s is not a directory", dest_path);
            return 1;
        }

        //parcurgem parametrii
        for(int i = 0; i < sources_nr; i++)
	    {
		    if(lstat(paths[i], &file_info_s) < 0)
		    {
			    perror("lstat");
			    continue;
		    }

		    m_source = file_info_s.st_mode;

            //destination_pathname
            char dest_pathname[MAX_FILENAME_LENGTH];
            strcpy(dest_pathname, "");
            strcat(dest_pathname, dest_path);
            strcat(dest_pathname, "/");

            //obtinem numele fiserului/directorului din pathname
            char source_name[MAX_FILENAME_LENGTH];
            strcpy(source_name, "");
            strcat(source_name, paths[i]);
            char* token = strtok(source_name, "/");
            char* file_name;

            while (token != NULL) {
                file_name = token;
                token = strtok(NULL, "/");
            }

            strcat(dest_pathname, file_name);

		    if(S_ISREG(m_source)) 
            {
                cp_file(paths[i], dest_pathname);
            }
            else if(S_ISDIR(m_source))
            {
                cp_dir(paths[i], dest_pathname);
            }
            else
            {
                printf("%s is nor a regular file, nor a directory.", paths[i]);
                continue;
            }
 
        }              
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      REMOVE      */

void rm_file(char *pathname)
{
    if(remove(pathname) < 0)
    {
        printf("The removal of %s was unsuccessful\n", pathname);
        return 1;
    }
}

void rm_dir(char *pathname)
{
    char source_name[FILENAME_MAX];

    //deschidem directorul
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (pathname);


    //stergem fiecare entry
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL) 
        {
            strcpy(source_name, "");
            strcat(source_name, pathname);
            strcat(source_name, "/");
            strcat(source_name, ep->d_name);

            if(ep->d_type == 4)
            {
                if(strcmp(ep->d_name, ".") && strcmp(ep->d_name, ".."))
                    rm_dir(source_name);
            }
            else
            {
                rm_file(source_name);
            }
        }
        
        closedir (dp);
    }
    else
    {
        perror ("open source directory");
        return 1;
    }

    if(remove(pathname) < 0)
    {
        printf("The removal of %s was unsuccessful\n", pathname);
        return 1;
    }
}

void rm(char *path)
{

    struct stat file_info;
    mode_t	m_source;

    //tipul parametrului
    if(lstat(path, &file_info) < 0)
	{
		printf("%s doesn't exist\n", path);
        return 1;
	}
        
    m_source = file_info.st_mode;

    if(S_ISDIR(m_source))
    {
        rm_dir(path);
    }
    else
    {
        rm_file(path);
    }

}


//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      MOVE      */

void mv_file(char *source, char *destination)
{
    int source_fd;
    int dest_fd;

    //deschidem fisierul sursa
    if((source_fd = open(source, O_RDONLY)) < 0)
    {
        perror("open source");
        return 1;
    }

    char source_name[FILENAME_MAX];
    strcpy(source_name, "");
    strcat(source_name, source);

    char* token = strtok(source_name, "/");
    char* file_name;

    while (token != NULL) {
        file_name = token;
        token = strtok(NULL, "/");
    }

    char destination_name[FILENAME_MAX];
    strcpy(destination_name, "");
    strcat(destination_name, destination);
    strcat(destination_name, "/");
    strcat(destination_name, file_name);

    //cream fiserul destinatie
    if((dest_fd = open(destination_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR) ) < 0)
    {
        perror("open/create destination");
        return 1;
    }

    //copiem continutul
    int n;
    char buf[8196];

    while ((n = read(source_fd, buf, sizeof(buf))) > 0 ) {
        write(dest_fd, buf, n);
    }

    //inchidem fisierele            
    close(source_fd);
    close(dest_fd);

    //stergem fisierul sursa
    rm_file(source);
}

void mv_dir(char *source, char *destination)
{
    char source_name[FILENAME_MAX];
    strcpy(source_name, "");
    strcat(source_name, source);

    char* token = strtok(source_name, "/");
    char* dir_name;

    while (token != NULL) {
        dir_name = token;
        token = strtok(NULL, "/");
    }

    char destination_name[FILENAME_MAX];
    strcpy(destination_name, "");
    strcat(destination_name, destination);
    strcat(destination_name, "/");
    strcat(destination_name, dir_name);


    //cream directorul nou
    mkdir(destination_name, S_IRWXU);

    //deschidem directorul sursa
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (source);

    //copiem fiecare entry
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL) 
        {
            char entry_name[FILENAME_MAX];
            strcpy(entry_name, "");
            strcat(entry_name, source);
            strcat(entry_name, "/");
            strcat(entry_name, ep->d_name);

            //daca entry este director
            if(ep->d_type == 4) 
            {   
                if(strcmp(ep->d_name, ".") && strcmp(ep->d_name, ".."))
                {   
                    mv_dir(entry_name, destination_name);
                }
                
            }
            //daca entry este regular file
            else
            {
                mv_file(entry_name, destination_name);
            }
        }
                    
        closedir (dp);
    
        //stergem directorul 
        rm_dir(source);
    }
    else
    {
        perror ("open source directory");
        return 1;
    }
}

void mv(char* source, char *dest)
{

    struct stat file_info_s;
    struct stat file_info_d;
	mode_t	m_dest;
    mode_t	m_source;

    //tipul celui de al doilea parametru
    if(lstat(dest, &file_info_d) < 0)
	    {
			perror("lstat");
            return 1;
	    }
        
    m_dest = file_info_d.st_mode;

    //al doilea parametru trebuie sa existe si sa fie folder
    if(!S_ISDIR(m_dest))
    {
        printf("%s is not a directory\n", source);
        return 1;
    }

    //tipul primului parametru
    if(lstat(source, &file_info_s) < 0)
	{
		perror("lstat");
        return 1;
	}
        
    m_source = file_info_s.st_mode;


    if(S_ISDIR(m_source))
    {
        mv_dir(source, dest);
    }
    else
    {
        mv_file(source, dest);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*      UTILITIES      */

void display_menu() {
    printf("Welcome to DropBox!  *__* \n"); 
    printf("Commands:\n1.du - display disk usage\n2.mkdir - create a directory\n3.search - search for an item\n4.ls - list items\n5.cp - copy files or directories\n6.rm - remove a file or a directory\n7.mv - move files or directories\n0-EXIT\n");
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
            }
            else if(option == '5') {
                
                char (*paths)[MAX_LENGTH] = commands + 1;
                const char *dest_path = malloc(MAX_FILENAME_LENGTH);

                int counter = 0;

                while(paths[counter][0] != '\0') {
                    strcpy(dest_path, paths[counter]);
                    counter ++;
                }

                if(counter < 2) {
                    printf("\nUsage cp: <command_number> <source_file> <dest_file> || <source_file1> <source_file2>... <destination_dir> || <source_dir1> [...] <dest_dir>\n");

                } else {
                    cp(paths, dest_path, counter - 1);
                }

                free(dest_path);
            }
            else if(option == '6'){
                const char *name = malloc(strlen(commands[1]) + 1);

                if (name != NULL) {
                    strcpy(name, commands[1]);
                } else {
                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }
                if (strlen(name) < 2) {
                    printf("\nUsage rm: <command_number> <path>");
                } else {
                    rm(name);
                }
                free(name);
            }
            else if(option == '7'){

                const char *source = malloc(strlen(commands[1]) + 1);
                const char *dest = malloc(strlen(commands[2]) + 1);

                if (source != NULL && dest != NULL) {
                    strcpy(source, commands[1]);
                    strcpy(dest, commands[2]);    
                } else {
                    fprintf(stderr, "Malloc!\n");
                    exit(1);
                }
                if (strlen(source) < 2 || strlen(dest) < 1) {
                    printf("\nUsage mv: <command_number> <source_file || source_dir> <dest_dir>");
                } else {
                    mv(source, dest);
                }
                free(source);
                free(dest);
            }
            else if (option == '0') {
                printf("\n!!! Exited from Dropbox\n");
                break;
            } else {
                printf("\n!!! Not a command. Try a number from the left of your desired command.\n");
            }

            free(commands);
    }
               
    exit(0); 
}