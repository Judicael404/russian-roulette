#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirlist.h"
#include "constants.h"

int read_directories(const char *filename, char *directories[], int max_dirs)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    char buffer[MAX_LEN];
    int count = 0;

    while (fgets(buffer, MAX_LEN, file) != NULL && count < max_dirs)
    {
        // Strip newline
        buffer[strcspn(buffer, "\n")] = '\0';

        directories[count] = malloc(strlen(buffer) + 1);
        if (directories[count] == NULL)
        {
            perror("Memory allocation failed");
            fclose(file);
            return -1;
        }

        strcpy(directories[count], buffer);
        count++;
    }

    fclose(file);
    return count;
}

void free_directories(char *directories[], int count)
{
    for (int i = 0; i < count; ++i)
    {
        free(directories[i]);
    }
}