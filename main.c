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

// Function to write a message to the slave PTY
void print_to_slave(int master_fd, const char *message) {
    write(master_fd, message, strlen(message));  // Write message directly to the slave PTY
}

// Function to sanitize input by removing non-printable characters
void sanitize_input(const char *input, char *sanitized) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (isprint((unsigned char)input[i])) {  // Only copy printable characters
            sanitized[j++] = input[i];
        }
    }
    sanitized[j] = '\0';  // Null-terminate the sanitized string
}

// Function to extract the command from the shell prompt
void extract_command(const char *buffer, char *command) {
    int start = 0;
    int i = 0;

    // Find where the prompt ends (e.g., '$' character or another recognizable prompt symbol)
    while (buffer[i] != '\0') {
        if (buffer[i] == '$' || buffer[i] == '#') {
            start = i + 1;  // The command starts after the prompt symbol
            break;
        }
        i++;
    }

    // Skip leading whitespace
    while (isspace((unsigned char)buffer[start])) {
        start++;
    }

    // Copy the command part from the buffer
    strcpy(command, &buffer[start]);
}

// Function to handle output and provide suggestions
int handle_output(char *cmdBuffer, List **suggestionList, BKTreeNode *bktree, char *inputBuffer, int master_fd) {
    regex_t regex;
    int reti;
    char word[255] = {0};
    char message[512] = {0};

    // Extract the command from the input buffer
    extract_command(inputBuffer, word);

    // Sanitize the extracted command
    char sanitized[256] = {0};
    sanitize_input(word, sanitized);

    // Compile a regex to detect "command not found" messages
    reti = regcomp(&regex, "command not found", REG_EXTENDED | REG_NOSUB | REG_ICASE);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return 0;
    }

    // Check if the cmdBuffer contains "command not found"
    reti = regexec(&regex, cmdBuffer, 0, NULL, 0);
    regfree(&regex);

    if (reti == 0) {
        // "command not found" was found in cmdBuffer
        // Clear previous suggestions
        if (*suggestionList != NULL) {
            free_list(*suggestionList);
            *suggestionList = NULL;
        }

        // Query the BK-tree for similar commands within a distance of 2
        query(bktree, sanitized, 2, *suggestionList);

        // If suggestions are found, print them to the slave PTY
        if (*suggestionList != NULL) {
            List *temp = *suggestionList;
            snprintf(message, sizeof(message), "Command '%s' not found. Did you mean:\n", sanitized);
            print_to_slave(master_fd, message);

            while (temp != NULL) {
                snprintf(message, sizeof(message), "  %s\n", temp->word);
                print_to_slave(master_fd, message);
                temp = temp->next;
            }
        }
        return 1;
    }
    return 0;
}

// Function to handle the I/O between master PTY and user terminal
void interact_with_pty(int master_fd, List **suggestionList, BKTreeNode *bktree) {
    char inputBuffer[256] = {0};
    char cmdBuffer[256] = {0};
    fd_set read_fds;
    ssize_t nread;
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
            nread = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1);
            if (nread > 0) {
                inputBuffer[nread] = '\0';  // Null-terminate the input
                // Write the input from stdin to the master PTY
                write(master_fd, inputBuffer, nread);
            }
        }

        // If there's output from the master PTY
        if (FD_ISSET(master_fd, &read_fds)) {
            nread = read(master_fd, cmdBuffer, sizeof(cmdBuffer) - 1);
            if (nread > 0) {
                cmdBuffer[nread] = '\0';  // Null-terminate the buffer

                // Write the output from the PTY to the terminal
                write(STDOUT_FILENO, cmdBuffer, nread);

                // Handle output for suggestions
                handle_output(cmdBuffer, suggestionList, bktree, inputBuffer, master_fd);

                // Clear inputBuffer after processing
                memset(inputBuffer, 0, sizeof(inputBuffer));
            } else if (nread == 0) {
                // EOF: the process running in the slave PTY has exited
                break;
            }
        }
    }
}

// Function to initialize the BK-tree with commands from the config file
void init_Config(BKTreeNode *bktree) {
    const char *configFile = "terma.conf";
    FILE *file = fopen(configFile, "r");
    char buffer[256];

    if (file) {
        // Read each line from the configuration file
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            insert_bk(bktree, buffer);
        }
        fclose(file);
    } else {
        perror("Error opening config file");
        // Handle error or initialize with default commands
    }
}

int main() {
    int master_fd;
    pid_t pid;
    char *shell = "/bin/bash";  // The shell to run in the slave PTY

    BKTreeNode *bktree = createNode("terma");
    List *suggestionList = NULL;
    init_Config(bktree);

    // Fork a child process and create a new PTY
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
        interact_with_pty(master_fd, &suggestionList, bktree);

        // Wait for the child process to finish
        waitpid(pid, NULL, 0);
    }

    // Free resources
    if (suggestionList != NULL) {
        free_list(suggestionList);
    }

    return 0;
}