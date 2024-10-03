#include "config.h"

void get_Path(){

    FILE *file = fopen("terma.conf", "w");

    if (file == NULL) {
        perror("Error opening file");
    }

    char *path = getenv("PATH");
    char *path_copy = strdup(path);
    char *delim = ":";
    char *token = strtok(path_copy, delim);

    while (token != NULL){

        DIR *d;
        struct dirent *dir;
        d = opendir(token);

        if (d) {
            // Loop through the directory entries
            while ((dir = readdir(d)) != NULL) {
                fprintf(file, "%s\n", dir->d_name);
            }

            closedir(d);
        } else {
            printf("%s is not a valid path\n", token);
        }

        token = strtok(NULL, delim);

    }

    free (path_copy);
    fclose(file);
}

#ifdef __MAIN__
int main() {
    get_Path();
    return 0;
}
#endif