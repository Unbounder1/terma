#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "algorithms.h"

BKTreeNode* createNode(const char *word) {
    BKTreeNode *node = (BKTreeNode*)malloc(sizeof(BKTreeNode));
    node->word = strdup(word); // Store a copy of the word
    for (int i = 0; i < 30; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void free_list(List *node) {
    if (node == NULL) {
        return; // Base case: stop when the node is NULL
    }

    // Recursively free the rest of the list first
    free_list(node->next);

    // Free the current node's memory
    free(node->word);  // Assuming word was dynamically allocated
    free(node);        // Free the node itself
}

List* create_node(const char *word) {
    List* new_node = (List *)malloc(sizeof(List));
    if (new_node == NULL){
        perror("Mem Alloc Error");
    }

    new_node->word = strdup(word); 
    new_node->prev = NULL;
    new_node->next = NULL;

    return new_node;
}

void insert_list(List **head, const char *word, int distance) {
    List *new_node = create_node(word);  // Create a new node with the word
    
    // If the list is empty, make the new node the head
    if (*head == NULL) {
        *head = new_node;
        return;
    }

    List *current = *head;

    // If the new node should be inserted at the head
    if (current->distance > distance) {
        new_node->next = current;
        current->prev = new_node;
        *head = new_node;
        return;
    }

    // Traverse the list to find the correct position
    while (current != NULL && current->distance <= distance) {
        if (current->next == NULL || current->next->distance > distance) {
            new_node->next = current->next;
            if (current->next != NULL) {
                current->next->prev = new_node;
            }
            current->next = new_node;
            new_node->prev = current;
            return;
        }
        current = current->next;
    }
}

void insert_bk(BKTreeNode *root, const char *word) {
    if (root == NULL) {
        fprintf(stderr, "Error: root is NULL\n");
        return;
    }
    int distance = levenshtein_distance(root->word, word);
    
    if (distance < 0 || distance >= 30) {
        fprintf(stderr, "Error: distance is out of bounds\n");
        return;
    }

    if (root->children[distance] != NULL) {
        insert_bk(root->children[distance], word);
    } else {
        root->children[distance] = createNode(word);
    }
}

void query(BKTreeNode *root, const char *word, int threshold, List *output) {
    int distance = levenshtein_distance(root->word, word);
    
    // If the distance is within the threshold, print the word
    if (distance <= threshold) {
        insert_list(&output, root->word, distance);
    }

    // Explore the children within the range of [distance - threshold, distance + threshold]
    for (int i = fmax(0, distance - threshold); i <= distance + threshold; i++) {
        if (root->children[i] != NULL) {
            query(root->children[i], word, threshold, output);
        }
    }
}

// Function to find the minimum of three values
int min(int x, int y, int z) {
    if (x < y && x < z) return x;
    if (y < x && y < z) return y;
    return z;
}

// Function to calculate the Levenshtein distance between two words
int levenshtein_distance(const char *word1, const char *word2) {
    int len1 = strlen(word1);
    int len2 = strlen(word2);

    // Create a 2D array to store distances
    int **dist = (int **)malloc((len1 + 1) * sizeof(int *));
    for (int i = 0; i <= len1; i++) {
        dist[i] = (int *)malloc((len2 + 1) * sizeof(int));
    }

    // Initialize the first row and column
    for (int i = 0; i <= len1; i++) {
        dist[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        dist[0][j] = j;
    }

    // Fill the rest of the distance matrix
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            if (word1[i - 1] == word2[j - 1]) {
                dist[i][j] = dist[i - 1][j - 1];  // No change if characters are the same
            } else {
                dist[i][j] = min(dist[i - 1][j] + 1,      // Deletion
                                 dist[i][j - 1] + 1,      // Insertion
                                 dist[i - 1][j - 1] + 1); // Substitution
            }
        }
    }

    // The Levenshtein distance is the value in the bottom-right corner of the array
    int result = dist[len1][len2];

    // Free the allocated memory
    for (int i = 0; i <= len1; i++) {
        free(dist[i]);
    }
    free(dist);

    return result;
}

#ifdef __MAIN__
int main() {
    char word1[] = "kitten";
    char word2[] = "sitting";

    int distance = levenshtein_distance(root->word, word);;
    printf("The Levenshtein distance between %s and %s is: %d\n", word1, word2, distance);

    return 0;
}
#endif