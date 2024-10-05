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

void print_to_slave(int master_fd, const char *message) {
    write(master_fd, message, strlen(message));  // Write message directly to the slave PTY
}

void sanitize_input(char *input, char *sanitized) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (isprint(input[i])) {  // Only copy printable characters
            sanitized[j++] = input[i];
        }
    }
    sanitized[j] = '\0';  // Null-terminate the sanitized string
}

void extract_command(char *buffer, char *command) {
    int start = 0;

    // Find where the prompt ends (e.g., '$' character or another recognizable prompt symbol)
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '$' || buffer[i] == '#') {
            start = i + 1;  // The command starts after the prompt symbol
            break;
        }
    }

    // Copy only the command part from the buffer, skipping leading spaces
    while (isspace(buffer[start])) {
        start++;
    }

    strcpy(command, &buffer[start]);
}

int handle_output(char buffer[256], List* suggestionList, BKTreeNode* bktree, char* inputBuffer, int master_fd) {
    regex_t regex;
    int reti;
    char word[255] = {0}; 
    char message[512] = {0}; 

    extract_command(inputBuffer, word); 

    snprintf(message, sizeof(message), "Buffer: %s\n", word);
    print_to_slave(master_fd, message);  

    reti = regcomp(&regex, "command", 0);

    if (reti == 0) {
        free_list(suggestionList);  
        char sanitized[256] = {0};
        sanitize_input(inputBuffer, sanitized);
        query(bktree, sanitized, 4, suggestionList);

        List *temp = suggestionList;
        if (temp != NULL) {
            // Iterate over the suggestion list and print each word to the slave PTY
            while (temp != NULL) {
                snprintf(message, sizeof(message), "Suggestion: %s\n", temp->word);  
                print_to_slave(master_fd, message); 
                temp = temp->next;
            }
        }
        return 1;
    }
    return 0;
}

// Function to handle the I/O between master PTY and user terminal
void interact_with_pty(int master_fd, List* suggestionList, BKTreeNode* bktree) {
    char inputBuffer[256];
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
            nread = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer));
            if (nread > 0) {
                // Simply write the input from stdin to the master PTY
                write(master_fd, inputBuffer, nread);
            }
        }

        // If there's output from the master PTY
        if (FD_ISSET(master_fd, &read_fds)) {
            nread = read(master_fd, cmdBuffer, sizeof(cmdBuffer));
            if (nread > 0) {
                if (strncmp(cmdBuffer, inputBuffer, strlen(inputBuffer)) != 0){
                    handle_output(inputBuffer, suggestionList, bktree, cmdBuffer, master_fd);
                    write(STDOUT_FILENO, cmdBuffer, nread);
                } 
            } else if (nread == 0) {
                // EOF: the process running in the slave PTY has exited
                break;
            }
        }
    }
}

void init_Config(BKTreeNode* bktree) {

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
        fclose(file);  \
    } else {

        perror("Error opening config file");
        get_Path();
    }
}

int main() {
    int master_fd;
    pid_t pid;
    char *shell = "/bin/bash";  // The shell to run in the slave PTY

    printf("hi\r\n");

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
        interact_with_pty(master_fd, suggestionList, bktree);

        // Wait for the child process to finish
        waitpid(pid, NULL, 0);
    }

    return 0;
}