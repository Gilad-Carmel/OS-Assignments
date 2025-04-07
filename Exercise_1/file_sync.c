#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>


#define MAX_PATH_LENGTH 1024
#define MAX_FILES_IN_DIRECTORY 100
#define MAX_FILE_NAME_LENGTH 256

int directory_exists(const char *path)
{
    struct stat path_stat;
    if ((stat(path, &path_stat) != 0))
    {
        return 0;
    }
    return S_ISDIR(path_stat.st_mode);
}

int run_execvp(char *const argv[])
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0)
    {
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(1);
    }
    else
    {
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("Command failed with exit status %d\n", WEXITSTATUS(status));
            return -1;
        }

    }
}

int file_exists(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }
    close(fd);
    return 1;
}



void create_directory(const char *path)
{
    char *argv[4] = { "mkdir", "-p", (char *)path, NULL };
    run_execvp(argv);
    printf("Created destination directory '%s'.\n", path);
}

void copy_file(const char *file1, const char *file2)
{
    char *argv[4] = { "cp", file1, file2, NULL };
    run_execvp(argv);
    printf("Copied: %s -> %s\n", file1, file2);
}

int newer_file(const char *file1, const char *file2)
{
    struct stat file_stat1, file_stat2;
    if (stat(file1, &file_stat1) != 0)
    {
        perror("stat failed");
        exit(1);
    }
    if (stat(file2, &file_stat2) != 0)
    {
        perror("stat failed");
        exit(1);
    }
    if (file_stat1.st_mtime > file_stat2.st_mtime)
    {
        return 1;
    }
    else if (file_stat1.st_mtime < file_stat2.st_mtime)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int str_cmp(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}
int file_diff(const char *file1, const char *file2) {
    int pipefd[2];  
    if (pipe(pipefd) == -1) {  
        perror("pipe failed");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {  
        close(pipefd[0]);  
        dup2(pipefd[1], STDOUT_FILENO);  
        close(pipefd[1]);  

        execlp("diff", "diff", file1, file2, (char *)NULL);
        perror("execlp failed");
        exit(1);  
    }

    
    close(pipefd[1]);  

    char buffer[MAX_PATH_LENGTH];  
    int differences_found = 0;

    differences_found = (read(pipefd[0], buffer, 1) > 0);

    close(pipefd[0]);  

    int status;
    waitpid(pid, &status, 0);
    
    return differences_found ? 1 : 0;
}


void dir_sync(const char *source, const char* dest) {
    struct dirent *entry;
    char source_file_path[MAX_PATH_LENGTH];
    char dest_file_path[MAX_PATH_LENGTH];
    char *filenames[MAX_FILES_IN_DIRECTORY];
    int file_count = 0;
    DIR *dir;
    struct stat file_stat;

    printf("Synchronizing from %s to %s\n", source, dest);
    
    dir = opendir(source);
    if (!dir) {
        perror("opendir failed");
        exit(1);
    }
    

    while (((entry = readdir(dir)) != NULL && file_count < MAX_FILES_IN_DIRECTORY))
    {
        snprintf(source_file_path, MAX_PATH_LENGTH, "%s/%s", source, entry->d_name);
        stat(source_file_path, &file_stat);
        if (S_ISREG(file_stat.st_mode))
        {
            filenames[file_count] = malloc(MAX_FILE_NAME_LENGTH);
            strcpy(filenames[file_count], entry->d_name);
            file_count++;
        }
    }
    
    closedir(dir);
    
    qsort(filenames, file_count, sizeof(char *), str_cmp);

    for (int i = 0; i < file_count; i++) {
        snprintf(source_file_path, MAX_PATH_LENGTH, "%s/%s", source, filenames[i]);
        snprintf(dest_file_path, MAX_PATH_LENGTH, "%s/%s", dest, filenames[i]);

        if (!file_exists(dest_file_path))
        {
            // file does not exist in destination, copy it
            printf("New file found: %s\n", filenames[i]);
            copy_file(source_file_path, dest_file_path);
        }
        else
        {
            // file exists in destination, check if it is different
            if (file_diff(source_file_path, dest_file_path) != 0)
            {
                // files are different, check which one is newer
                if (newer_file(source_file_path, dest_file_path) > 0)
                {
                    // source file is newer, copy it to destination
                    printf("File %s is newer in source. Updating...\n", filenames[i]);
                    copy_file(source_file_path, dest_file_path); 
                }
                else
                {
                    // destination file is newer, copy it to source
                    printf("File %s is newer in destination. Skipping...\n", filenames[i]);
                }
            }
            else
            {
                printf("File %s is identical. Skipping...\n", filenames[i]);
            }
        }

    }
    // free allocated memory
    for (int i = 0; i < file_count; i++) {
        free(filenames[i]);
    }

    printf("Synchronization complete.\n");
}

int main(int argc, char const *argv[])
{
    // print current working directory
    char cwd[MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd failed");
        exit(1);
    }

    // validate arguments
    if (argc != 3)
    {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        exit(1);
    }
   
    // parse arguments
    char source_directory[MAX_PATH_LENGTH];
    char destination_directory[MAX_PATH_LENGTH];
    strcpy(source_directory, argv[1]);
    strcpy(destination_directory, argv[2]);

    // check if source directory exists
    if (!directory_exists(source_directory))
    {
        printf("Error: Source directory '%s' does not exist.\n", source_directory);
        exit(1);
    }

    // create destination directory if it doesn't exist
    if (!directory_exists(destination_directory))
    {
        create_directory(destination_directory);
    }

    // concatenate cwd to source and destination directories
    char source_path[MAX_PATH_LENGTH];
    char dest_path[MAX_PATH_LENGTH];
    if (source_directory[0] != '/')
    {
        snprintf(source_path, sizeof(source_path), "%s/%s", cwd, source_directory);
    }
    else
    {
        snprintf(source_path, sizeof(source_path), "%s", source_directory);
    }
    
    if (destination_directory[0] != '/')
    {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", cwd, destination_directory);
    }
    else
    {
        snprintf(dest_path, sizeof(dest_path), "%s", destination_directory);
    }

    // sync files
    dir_sync(source_path, dest_path);
    

}