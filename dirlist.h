#ifndef DIRLIST_H
#define DIRLIST_H

int read_directories(const char *filename, char *directories[], int max_dirs);
void free_directories(char *directories[], int count);

#endif
