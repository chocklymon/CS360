#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

/**
 * Learning Activity:
 * Using stat
 */
int main(int argc, char **argv)
{
    char *directory;
    struct stat filestat;

    // Command Line Arguments //
    if (argc < 2) {
        printf("usage: %s <dir>\n", argv[0]);
        return 1;
    }
    directory = argv[1];

    // Using stat
    if (stat(directory, &filestat)) {
        perror("Problem reading directory");
        return 2;
    }
    if (S_ISREG(filestat.st_mode)) {
        printf("%s is a regular file\n", directory);
        printf("file size = %d\n", filestat.st_size);

        // Read the file
        FILE *fp = fopen(directory, "r");
        char *buffer = malloc((filestat.st_size + 1) * sizeof(char));
        if (fread(buffer, filestat.st_mode, 1, fp)) {
            perror("Problem reading file");
            return 2;
        }
        printf("------\n%s\n", buffer);
        free(buffer);
        fclose(fp);
    }
    if (S_ISDIR(filestat.st_mode)) {
        printf("%s is a directory\n", directory);

        // Using dirent to print all the files in the directory
        DIR *dirp;
        struct dirent *dp;

        dirp = opendir(directory);
        while ((dp = readdir(dirp)) != NULL) {
            printf("name %s\n", dp->d_name);
        }
        closedir(dirp);
    }

    return 0;
}
