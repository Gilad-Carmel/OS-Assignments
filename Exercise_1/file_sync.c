#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_PATH_LENGTH 1024
#define MAX_FILES_IN_DIRECTORY 100
#define MAX_FILE_NAME_LENGTH 256

// chagne directory to source directory
int change_directory(const char *path)
{
    if (chdir(path) != 0)
    {
        perror("chdir() error");
        return 1;
    }
    return 0;
}
// get current working directory
int get_current_directory(){
    char cwd[MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working dir: %s\n", cwd);
        return 0;
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }
}



int main(int argc, char const *argv[])
{
    // validate arguments
    if (argc != 2)
    {
        printf("Usage: %s <source_directory> <destination_directory>\n", argv[0]);
        return 1;
    }
   
    // check if source directory exists

    // create destination directory if it doesn't exist
    
    // parse arguments
    char source_directory[MAX_PATH_LENGTH];
    char destination_directory[MAX_PATH_LENGTH];
    strcpy(source_directory, argv[1]);
    strcpy(destination_directory, argv[2]);

    // change to source directory
    if (change_directory(source_directory) != 0)
    {
        return 1;
    }
    // get current working directory
    if (get_current_directory() != 0)
    {
        return 1;
    }


    // open source directory
    
}