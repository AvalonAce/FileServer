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
        Client Number, IP, PORT, Commands (LIST|DIFF|PULL|LEAVE|)
        Client Number, IP, PORT, Commands (LIST|LIST|DIFF|PULL|LEAVE|)
        ...
*/
int create_clients_csv(char *file_path) {

    // TO DO: Create file to store data for each client


    return 0;
}

int create_new_csv_entry(char *file_path, char *content) {
    
    // TO DO: Write 1 line to csv file for new client


    return 0;
}

int add_to_csv_entry(char *file_path, int client_number, char *command) {

    // TO DO: Add command to client's line in csv file at the end

    return 0;
}

