#ifndef COMPARE_H
#define COMPARE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the BKTreeNode structure
typedef struct BKTreeNode {
    char *word;                
    int distance;           
    struct BKTreeNode *children[10];
} BKTreeNode;

// Define the Doubly linked list structure
typedef struct List {
    char *word;
    int distance;                           
    struct List *next;
    struct List *prev;
} List;

// Function declarations

List* create_node(const char *word);

void insert_list(List **head, const char *word, int distance);

void free_list(List *node);

// Create a new BKTreeNode
BKTreeNode* createNode(const char *word);

// Insert a word into the BK-tree
void insert_bk(BKTreeNode *root, const char *word);

// Query the BK-tree for words within a certain Levenshtein distance
void query(BKTreeNode *root, const char *word, int threshold, List *output);

// Function to calculate the Levenshtein distance between two strings
int levenshtein_distance(const char *word1, const char *word2);

// Helper function to find the minimum of three values
int min(int x, int y, int z);

#endif // COMPARE_H