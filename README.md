
# BK-Tree Based Command-Line Tool with Autocompletion and Configuration Parsing

This project is a command-line tool that provides autocompletion functionality based on a BK-Tree (Burkhard-Keller Tree) data structure. It reads commands from a configuration file (`terma.conf`) and allows for interactive command input, including suggestions when commands are not found. Additionally, the program uses the Levenshtein distance to find and suggest similar commands when a command is mistyped.

## Features

- **BK-Tree**: Efficiently stores and queries strings using Levenshtein distance.
- **Autocompletion**: Provides suggestions based on partial command input.
- **Command Parsing**: Reads valid commands from a configuration file (`terma.conf`).
- **Interactive PTY Handling**: Manages interaction between the user and a shell running in a pseudo-terminal (PTY).
- **Tab Completion**: Press `TAB` to autocomplete commands based on valid entries in the BK-Tree.

## Requirements

- **GCC** (for compiling)
- **gdb** (for debugging)
- **Standard C Libraries**: `fcntl.h`, `stdio.h`, `stdlib.h`, `unistd.h`, `termios.h`, `string.h`, `sys/wait.h`, `sys/select.h`, `pty.h`, `regex.h`, `ctype.h`

## Installation

1. Clone the repository or download the source files.
2. Compile the program with debugging information enabled:

   ```bash
   gcc -g -o myprogram main.c config.c algorithms.c -lm
   ```

   The `-g` flag is important for enabling debugging information.

3. Run the compiled program:

   ```bash
   ./myprogram
   ```

## How It Works

### BK-Tree

The BK-Tree is used to store and search for command suggestions based on Levenshtein distance. This allows for efficient autocompletion and command correction.

- **Insertion (`insert_bk`)**: Adds a word to the BK-Tree.
- **Querying (`query`)**: Searches the tree for words within a certain Levenshtein distance to suggest corrections.

### Command Parsing

The program reads valid commands from a configuration file (`terma.conf`). If this file exists, it is parsed, and each command is inserted into the BK-Tree.

- **Configuration File**: The configuration file (`terma.conf`) is expected to contain a list of commands, one per line. These commands are used for autocompletion and correction suggestions.

### Autocompletion

While running in an interactive terminal, the program listens for user input. When the `TAB` key is pressed, it provides autocompletion suggestions based on the current input string and the BK-Tree contents.

### Debugging

This program uses recursion in its BK-Tree insertion and query functions. If a segmentation fault occurs, it can be difficult to trace the issue. To help with debugging, compile the program with `-g` and use `gdb`.

### Debugging with `gdb`

If you encounter a segmentation fault (SIGSEGV), you can debug it using `gdb`:

1. **Compile with debugging symbols**:

   ```bash
   gcc -g -o myprogram main.c config.c algorithms.c -lm
   ```

2. **Run with `gdb`**:

   ```bash
   gdb ./myprogram
   ```

3. **Set breakpoints**:

   You can set breakpoints to pause execution at specific points, such as the start of the `insert_bk` function:

   ```gdb
   (gdb) break insert_bk
   ```

4. **Run the program**:

   Start running the program:

   ```gdb
   (gdb) run
   ```

5. **Inspect Segmentation Faults**:

   If the program crashes, use the `backtrace` command to see where the crash occurred:

   ```gdb
   (gdb) backtrace
   ```

   This will show you the sequence of function calls leading to the crash.

6. **Inspect variables**:

   Use `print` to check the value of pointers or variables at any point in the program:

   ```gdb
   (gdb) print root
   ```

## Usage

1. **Running the Program**:
   - The program starts a shell in a pseudo-terminal (PTY) and listens for user input.
   - You can type commands and press `Enter` to execute them.

2. **Tab Completion**:
   - Press `TAB` to trigger autocompletion based on the current input and the commands in `terma.conf`.

3. **Command Suggestions**:
   - If an unknown command is entered, the program suggests similar commands based on Levenshtein distance.

4. **Configuration File (`terma.conf`)**:
   - The program reads commands from the configuration file `terma.conf`.
   - Each line in the file should contain one valid command.
   - Example `terma.conf`:
     ```
     cloud-init
     systemctl
     reboot
     shutdown
     ```

## File Structure

- **main.c**: Handles the main program flow, interaction with the shell, and command input.
- **config.c**: Responsible for reading the configuration file (`terma.conf`) and inserting commands into the BK-Tree.
- **algorithms.c**: Contains the BK-Tree implementation and Levenshtein distance function.

## Troubleshooting

1. **Segmentation Faults**:
   - Ensure the BK-Tree is initialized properly before insertion.
   - Check pointer values using `gdb` and ensure that memory is correctly allocated.

2. **Autocompletion Not Working**:
   - Verify that `terma.conf` contains valid commands.
   - Ensure that the `TAB` key is triggering the correct functionality in the `interact_with_pty` function.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

---

This `README.md` provides clear documentation on how to install, run, and debug the program, while also explaining the core components. You can customize it further depending on any additional functionality you implement.