void create_hard_link(const char *src, const char *dst);
void copy_symlink(const char *src, const char *dst);
void copy_directory(const char *src, const char *dst);

// recursive function to copy files and directories
// preserve symlinks (dont create an actual copy of the file)
// create hard links instead of copying files
// maintain file permissions

// scan a dir:
// - if regular file, create hard link (this way we preserve the inode, which preserves the file permissions)
// - if symlink, copy symlink
// - if directory, run this function recursively, to preserve the structure

// 2 arguments: source_directory, backup_directory
// the source_directory must exist
// the backup_directory must not exist

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#define PATH_MAX 4096
void create_hard_link(const char *src, const char *dst) {
    if (link(src, dst) == -1) {
        perror("link");
        exit(EXIT_FAILURE);
    }
}

void copy_symlink(const char *src, const char *dst) {
    char target[PATH_MAX];
    ssize_t len = readlink(src, target, sizeof(target) - 1);
    if (len == -1) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    target[len] = '\0';
    if (symlink(target, dst) == -1) {
        perror("symlink");
        exit(EXIT_FAILURE);
    }
}

void copy_directory(const char *src, const char *dst) {
    struct stat st;
    if (lstat(src, &st) == -1) {
        perror("lstat");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(st.st_mode)) {
        if (mkdir(dst, st.st_mode) == -1) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }

        DIR *dir = opendir(src);
        if (!dir) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            // construct the full path for source and destination
            char src_path[PATH_MAX];
            char dst_path[PATH_MAX];
            snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

            // check the type of the file
            if (lstat(src_path, &st) == -1) {
                perror("lstat");
                exit(EXIT_FAILURE);
            }

            if (S_ISREG(st.st_mode)) {
                create_hard_link(src_path, dst_path);
            } else if (S_ISLNK(st.st_mode)) {
                copy_symlink(src_path, dst_path);
            } else if (S_ISDIR(st.st_mode)) {
                copy_directory(src_path, dst_path);
            }
        }
        closedir(dir);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_directory> <backup_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    struct stat st;
    lstat(src, &st); // if this fails it falls to the next check
    if (!S_ISDIR(st.st_mode)) {
        perror("src dir");
        return EXIT_FAILURE;
    }

    // make sure backup directory does not exist
    if (lstat(dst, &st) != -1 && S_ISDIR(st.st_mode)) {
        perror("backup dir");
        return EXIT_FAILURE;
    }

    copy_directory(src, dst);

    return EXIT_SUCCESS;
}