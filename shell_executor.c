#include "shell_executor.h"
#include <stdio.h>
#include <stdlib.h>

// Function to execute remove directory command with console output
int execute_remove_directory(const char* directory)
{
    char command[1024];
    
    printf("\n--- Directory Removal ---\n");
    printf("Preparing to remove directory: %s\n", directory);
    printf("This action is irreversible!\n");
    
    // Build the command
    snprintf(command, sizeof(command), "rm -rf \"%s\"", directory);
    
    printf("Executing: %s\n", command);
    printf("Please wait...\n");
    
    // Execute the command
    int result = system(command);
    
    if (result == 0)
    {
        printf("Successfully removed directory: %s\n", directory);
    }
    else
    {
        printf("Failed to remove directory: %s\n", directory);
        printf("Error code: %d\n", result);
    }
    
    printf("--- Operation complete ---\n\n");
    
    return result;
}