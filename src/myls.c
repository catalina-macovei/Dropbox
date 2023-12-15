#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

int main(int argc, char *argv[], char* envp[])
{
    DIR *dp;
	struct dirent *dirp;

	if(argc != 2)
		printf("Usage: %s <dirname>\n", argv[0]), exit(0);

	if((dp = opendir(argv[1])) == NULL)
		perror("opendir"), exit(1);

	while((dirp = readdir(dp)) != NULL)
		printf("%s\n", dirp->d_name);
	
	closedir(dp);
	exit(0);
}
