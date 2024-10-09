// =====================================
/*

Structs and functions for server-side handling.

*/
// =====================================

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>  // for directory functions
#include <limits.h>  // for PATH_MAX


struct tree_node {



};

struct file_tree { // Store files in 'dir' directory


};

// CSV File for client data
/*
    CSV file format:
        IP:PORT, Command
        IP:PORT, Command
        ...
*/
int create_clients_csv(char *file_path) {

    // Open File
    FILE *fptr = NULL;
    fptr = fopen(file_path, "w+");
    if (fptr == NULL) {
        return -1;
    }

    // 1st line of csv file
    fprintf(fptr, "IP, PORT, Command\n");

    // Close and return
    fclose(fptr);
    return 0;
}

int add_to_csv_entry(char *file_path, char* IP, int PORT,char *command) {

    // TO DO: Search client file and if not present, add line, else, append command to line
    // Open File
    FILE *fptr = NULL;
    fptr = fopen(file_path, "a+"); 
    if (fptr == NULL) {
        return -1;
    }

    // Write to end of file
    fprintf(fptr, "%s, %d, %s\n", IP, PORT, command);


    // Close and return
    fclose(fptr);
    return 0;
}

