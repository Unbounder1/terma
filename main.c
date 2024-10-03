#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <pty.h>
#include <regex.h>
#include <ctype.h>

#include "algorithms.h"
#include "config.h"


int handle_output(char buffer[256], List* suggestionList, BKTreeNode* bktree, const char *word){
    regex_t regex;
    int reti;

    // check if command not found (no suggestions)
    reti = regcomp(&regex, "command not found", 0);

    if (!reti){
        query(bktree, word, 4, suggestionList);
        return 1;
    }
    return 0;

}

void handle_tab_autocomplete(List* suggestionList, char* cmdBuffer, BKTreeNode* bktree) {
    List *current = suggestionList;
    List *matched = NULL;
    int matches = 0;
    size_t len = strlen(cmdBuffer);

    // Traverse the suggestion list and check for matches
    while (current != NULL) {
        // If the word starts with the current buffer
        if (strncmp(current->word, cmdBuffer, len) == 0) {
            matched = current;  // Track the match
            matches++;
        }
        current = current->next;
    }

    if (matches == 1) {
        // If only one match, autocomplete it
        strcpy(cmdBuffer, matched->word);
        printf("\r%s", cmdBuffer);  // Print the completed command
        fflush(stdout);
    } else if (matches > 1) {
        // If multiple matches, list them
        printf("\nPossible completions:\n");
        current = suggestionList;
        while (current != NULL) {
            if (strncmp(current->word, cmdBuffer, len) == 0) {
                printf("%s ", current->word);
            }
            current = current->next;
        }
        printf("\n%s", cmdBuffer);  // Reprint the current buffer for the user
        fflush(stdout);
    }
}

// Function to handle the I/O between master PTY and user terminal
void interact_with_pty(int master_fd, List* suggestionList, BKTreeNode* bktree) {
    char buffer[256];
    char cmdBuffer[256];
    fd_set read_fds;
    ssize_t nread;
    int suggestion_flag = 0;
    int cmd_len = 0;
    int max_fd = master_fd > STDIN_FILENO ? master_fd : STDIN_FILENO;

    while (1) {
        // Set up the file descriptor set
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);  // Add standard input to the set
        FD_SET(master_fd, &read_fds);     // Add master PTY to the set

        // Wait for input from either the master PTY or stdin
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // If there's input from the terminal (stdin)
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            nread = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (nread > 0) {
                // Check if user pressed tab
                if (buffer[0] == '\t') {
                    handle_tab_autocomplete(suggestionList, cmdBuffer, bktree);
                } else if (buffer[0] == '\n') {
                    // Send the full command to the master PTY and reset cmdBuffer
                    write(master_fd, cmdBuffer, cmd_len);  // Send the input to the master PTY
                    write(master_fd, "\n", 1);  // Send newline
                    cmd_len = 0;  // Reset the command length
                    memset(cmdBuffer, 0, sizeof(cmdBuffer));  // Clear the buffer
                } else {
                    // Handle other input (regular characters)
                    strncat(cmdBuffer, buffer, nread);  // Append to the command buffer
                    cmd_len += nread;
                    write(master_fd, buffer, nread);  // Forward to master PTY
                }
            }
        }

        // If there's output from the master PTY
        if (FD_ISSET(master_fd, &read_fds)) {
            nread = read(master_fd, buffer, sizeof(buffer));
            if (nread > 0) {
                write(STDOUT_FILENO, buffer, nread);  // Write PTY output to the terminal
                suggestion_flag = handle_output(buffer, suggestionList, bktree, cmdBuffer);

            } else if (nread == 0) {
                // EOF: the process running in the slave PTY has exited
                break;
            }
        }
    }
}

void init_Config(BKTreeNode* bktree){

    const char *configFile = "terma.conf"; // Check if config file exists
    FILE *file = fopen(configFile, "r");
    char buffer[256];

    if (file) {

        if (file == NULL) {
            perror("Error opening file");
        }

        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            insert_bk(bktree, buffer);
        }

    } else {
        get_Path();
    }

}

int main() {
    int master_fd;
    pid_t pid;
    char *shell = "/bin/bash";  // The shell to run in the slave PTY

    BKTreeNode *bktree = createNode("terma");
    init_Config(bktree);

    // Step 1: Fork a child process and create a new PTY
    pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid < 0) {
        perror("forkpty");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Child process (Slave PTY)
        // The child process runs a shell inside the slave PTY
        execlp(shell, shell, (char *)NULL);
        perror("execlp");  // If execlp fails
        exit(EXIT_FAILURE);
    } else {  // Parent process (Master PTY)
        // The parent process interacts with the master PTY
        interact_with_pty(master_fd);

        // Wait for the child process to finish
        waitpid(pid, NULL, 0);
    }

    return 0;
}