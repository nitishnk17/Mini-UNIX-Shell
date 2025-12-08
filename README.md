# Mini UNIX Shell

A functional command-line interpreter in C++ demonstrating core UNIX shell concepts: process management, I/O redirection, pipelines, background jobs, and signal handling.

## Features

- **External Commands:** PATH lookup via `execvp()`
- **Quote-Aware Parsing:** Handles single/double quotes preserving spaces
- **I/O Redirection:**  `<  ` input,  `> ` output using  `dup2() `
- **Pipelines:** Single-stage  `cmd1 | cmd2 `
- **Background Jobs:** Async execution with  `& `
- **Built-ins:**  `cd `,  `exit `,  `history ` (last 15 commands)
- **Persistent History:** Commands saved to  `.shell _history `
- **Robust:** Zero crashes on malformed input, automatic zombie cleanup, Ctrl-C safe

## Architecture

 ` ` `
shell.cpp          - Main REPL loop
parser.cpp/h       - Tokenization with quote handling
executor.cpp/h     - Process creation (fork-exec, pipelines)
redirection.cpp/h  - I/O redirection parsing
helpers.cpp/h      - Built-ins, signals, history
 ` ` `

Modular design with separation of concerns enables independent testing and maintenance.

## Building and Running

**Requirements:** g++ with C++11, make, POSIX OS (Linux/macOS/BSD)

 ` ` `bash
make              # Build
./shell           # Run
make clean        # Clean
make rebuild      # Rebuild
 ` ` `

Compiles with  `-Wall -Wextra -std=c++11 -pedantic ` (zero warnings).

## Examples

**Basic usage:**
 ` ` `bash
Mini-shell> ls -la
Mini-shell> pwd
Mini-shell> echo "Hello World"
 ` ` `

**Redirecting files:**
 ` ` `bash
Mini-shell> cat < input.txt
Mini-shell> ls > output.txt
Mini-shell> cat < input.txt > output.txt
 ` ` `

**Using pipes:**
 ` ` `bash
Mini-shell> ls | grep txt
Mini-shell> cat file.txt | wc -l
Mini-shell> ps aux | grep shell
 ` ` `

**Background jobs:**
 ` ` `bash
Mini-shell> sleep 10 &
[Background] PID: 12345
Mini-shell> 
 ` ` `

**we can combine these:**
 ` ` `bash
Mini-shell> cat < input.txt | sort > output.txt &
[Background] PIDs: 12345, 12346
 ` ` `

**Built-in commands:**
 ` ` `bash
Mini-shell> cd /some/directory
Mini-shell> cd
Mini-shell> history
  1  ls
  2  pwd
  3  echo 'Hello World'
  4  cat file.txt
  5  history
Mini-shell> exit
 ` ` `

**About history:** Commands saved to  `.shell _history `. The  `history ` command shows last 15 entries.

## Implementation

The shell is built around a Read-Eval-Print Loop (REPL) in `shell.cpp`, orchestrating modular functions for parsing and execution.

### The Life of a Command

1.  **Reading:** The `main` function in `shell.cpp` reads a line of input from the user.
2.  **Parsing:** The raw input string is passed to `parseLine` in `parser.cpp`. This function tokenizes the input into a vector of strings, correctly handling quoted arguments to treat them as a single token.
3.  **Analysis:** The vector of arguments is then inspected to determine the command structure:
    *   Is it a simple command?
    *   Does it contain a pipe (`|`)?
    *   Does it have input/output redirection (`<`, `>`)?
    *   Should it run in the background (`&`)?
4.  **Execution:**
    *   **Built-in Commands:** If the command is a built-in (`cd`, `exit`, `history`), it is handled directly within the main shell process. This is necessary for commands like `cd` that need to modify the shell's own environment.
    *   **External Commands:** For all other commands, the shell uses the classic `fork-exec` model.
        *   `fork()` creates a new child process.
        *   The child process handles any I/O redirection using `dup2()`.
        *   `execvp()` then replaces the child process's memory space with the command to be executed. `execvp` automatically searches the directories in the `PATH` environment variable.
    *   **Pipelines:** For a command like `cmd1 | cmd2`, the shell creates a pipe and forks two child processes. The standard output of the first child is redirected to the write-end of the pipe, and the standard input of the second child is redirected to the read-end of the pipe.
5.  **Process Management:**
    *   For foreground commands, the parent shell waits for the child process(es) to complete using `waitpid()`.
    *   For background commands (ending with `&`), the parent shell does not wait, immediately prompting for the next command. A `SIGCHLD` signal handler is used to asynchronously clean up (reap) terminated background processes to prevent them from becoming zombies.

### Signal Handling

*   `SIGINT` (Ctrl+C): A signal handler is set up to catch this signal. Instead of terminating the shell, it simply prints a new line and re-displays the prompt, providing a more robust user experience.
*   `SIGCHLD`: This signal is sent to the parent process when a child process terminates. The handler for this signal cleans up zombie processes.

## How to Contribute

Contributions are welcome! If you have suggestions for improvements or new features, please feel free to open an issue or submit a pull request.

## Testing

The shell was tested against a comprehensive suite of **25 test cases**, achieving a 100% pass rate. This suite validated all major features, including I/O redirection, complex pipeline setups, background execution, and robust error handling.


## Limitations

**Not Implemented:** Multi-stage pipelines ( `cmd1|cmd2|cmd3 `), append redirection ( `>> `), stderr redirection ( `2> `), job control ( `jobs/fg/bg `), wildcard expansion, interactive history navigation, command substitution, environment variable export.

**Constraints:** Built-ins can't be used in pipelines.  `& ` must be at end of command.

## Performance

**Complexity:** Parsing O(n), PATH lookup O(m). Fork/exec and pipes use kernel optimizations.

## Requirements

**All Mandatory Requirements Met ✓**
- External programs (PATH lookup)
- Argument parsing (quoted strings)
- I/O redirection ( `<  `,  `> `)
- Pipelines ( `cmd1 | cmd2 `)
- Background execution ( `& `)
- Built-ins ( `cd `,  `exit `)
- No crashes, zombie cleanup, Ctrl-C safe

**Bonus:** History with persistence


