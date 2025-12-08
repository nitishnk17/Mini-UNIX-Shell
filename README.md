# Mini UNIX Shell

Our implementation of a command-line shell in C++ for the COD7001 Cornerstone Lab. This project provided hands-on experience with fundamental UNIX concepts: process creation and management, file descriptor manipulation, inter-process communication via pipes, and signal handling. Building this shell from scratch helped us understand how terminal programs like bash work internally.

## What We Built

We set out to create a shell that actually works like bash or zsh (well, a simplified version). Here's what we managed to get working:

- **Running external programs** - Searches PATH and executes commands using `execvp()`
- **Proper quote handling** - Single and double quotes work correctly, preserving spaces
- **I/O redirection** - Both `<` for input and `>` for output using `dup2()`
- **Pipelines** - Connect commands with `|` (single-stage for now)
- **Background jobs** - Append `&` to run processes in the background
- **Built-in commands** - Implemented `cd`, `exit`, and `history`
- **Command history** - Saves your commands to `.shell_history` and shows last 15
- **Robust error handling** - Doesn't crash on bad input, cleans up zombie processes, and handles Ctrl+C gracefully

## How We Organized the Code

We split the implementation into five main modules to keep things manageable:

```
shell.cpp          - Main loop and command orchestration
parser.cpp/h       - Parsing user input and handling quotes
executor.cpp/h     - Fork-exec logic for running commands
redirection.cpp/h  - Parsing and validating I/O redirection
helpers.cpp/h      - Built-in commands, signals, and history
```

This modular approach really helped us work in parallel and debug issues without stepping on each other's toes.

## Quick Start

**What you need:** g++ with C++11 support, make, and a POSIX system (Linux/macOS/BSD)

```bash
make
./shell
make clean
make rebuild
```

We compile with `-Wall -Wextra -std=c++11 -pedantic` and get zero warnings.

## Examples

Here are some things we can try:

**Basic commands:**

```bash
Mini-shell> ls -la
Mini-shell> pwd
Mini-shell> echo "Hello World"
```

**File redirection:**

```bash
Mini-shell> cat < input.txt
Mini-shell> ls > output.txt
Mini-shell> cat < input.txt > output.txt
```

**Pipes:**

```bash
Mini-shell> ls | grep txt
Mini-shell> cat file.txt | wc -l
Mini-shell> ps aux | grep shell
```

**Background execution:**

```bash
Mini-shell> sleep 10 &
[Background] PID: 12345
Mini-shell>
```

**Combining features:**

```bash
Mini-shell> cat < input.txt | sort > output.txt &
[Background] PIDs: 12345, 12346
```

**Built-in commands:**

```bash
Mini-shell> cd /some/directory
Mini-shell> cd
Mini-shell> history
  1  ls
  2  pwd
  3  echo 'Hello World'
  4  cat file.txt
  5  history
Mini-shell> exit
```

## How We Implemented It

### Step 1: Getting the Basics Right

We started with the main REPL (Read-Eval-Print Loop) in `shell.cpp`. This was straightforward - just read a line, parse it, execute it, and repeat. The tricky part came with everything else.

### Step 2: Parsing with Quote Support

The parser turned out to be more complex than we initially thought. We implemented a simple state machine that tracks whether we're inside quotes or not. The key insight was maintaining a boolean flag for the quote state and remembering which quote character opened it. Whitespace only splits tokens when we're outside quotes.

We spent a good hour debugging cases like `echo "hello world"` versus `echo hello world` to make sure spaces were preserved correctly inside quotes.

### Step 3: Process Execution

For external commands, we used the classic fork-exec pattern:

1. Call `fork()` to create a child process
2. In the child, set up any I/O redirection with `dup2()`
3. Call `execvp()` which searches PATH and runs the command
4. In the parent, either wait for the child (foreground) or return immediately (background)

Built-in commands like `cd` and `exit` had to be handled differently since they need to modify the shell's own state. We check for these before forking.

### Step 4: I/O Redirection

This was where we really learned about file descriptors. For output redirection (`>`):

- Open the output file
- Use `dup2()` to make stdout (file descriptor 1) point to this file
- Close the original file descriptor

For input redirection (`<`), it's similar but with stdin (file descriptor 0). The cool thing is you can combine both since they're independent.

One bug that took us a while to find: we forgot to close the original file descriptors after `dup2()`, which leaked file descriptors until the shell ran out.

### Step 5: Pipelines

Pipes were definitely the most challenging part. Here's how we got them working:

1. Create a pipe using `pipe()` - this gives us two file descriptors (read end and write end)
2. Fork twice - once for each command
3. In the first child, redirect stdout to the pipe's write end
4. In the second child, redirect stdin to the pipe's read end
5. Critical step we initially missed: Close both pipe ends in the parent process

That last step was crucial. We spent an afternoon debugging why our second command wouldn't terminate - turns out if the parent doesn't close the pipe ends, the read end never sees EOF because there's still a potential writer (the parent).

### Step 6: Background Jobs and Signal Handling

For background execution, we simply skip the `waitpid()` call when we see `&` at the end. But this creates zombie processes when they finish. We set up a `SIGCHLD` handler that automatically reaps these zombies by calling `waitpid(-1, NULL, WNOHANG)` in a loop.

We also handle `SIGINT` (Ctrl+C) to prevent the shell itself from being killed while still allowing foreground commands to be interrupted.

### Step 7: Command History

We added persistent history as a bonus feature. Every command gets appended to `.shell_history`, and we load this file when the shell starts. The `history` command displays the last 15 entries to keep the output readable.

## Testing Journey

We tested this thing extensively - wrote and ran over 25+ different test cases covering:

- Basic command execution (PATH lookup, arguments, etc.)
- All three built-in commands
- Input redirection, output redirection, and combined redirection
- Simple pipes, pipes with redirection, pipes in background
- Background jobs and zombie cleanup
- Quote handling (single, double, mixed, nested)
- Error cases (invalid commands, bad syntax, missing files)
- Edge cases (empty input, very long commands, special characters)

We had a 100% pass rate by the end, though getting there involved fixing quite a few bugs:

- Pipeline deadlocks (forgot to close pipe ends in parent)
- Memory leaks (not cleaning up vectors properly)
- Race conditions with background jobs
- Incorrect handling of built-ins in pipelines

## Known Limitations

We didn't implement everything a full shell has:

**Not implemented:**

- Multi-stage pipelines (`cmd1 | cmd2 | cmd3`)
- Append redirection (`>>`)
- Error redirection (`2>`)
- Job control (`jobs`, `fg`, `bg`)
- Wildcard expansion (`*.txt`)
- Command substitution (`` `command` `` or `$(command)`)
- Environment variable export

**Constraints:**

- Built-in commands can't be used in pipelines (they modify the shell's state)
- The `&` symbol must appear at the end of the command

## What We Learned

This project was a great hands-on introduction to operating systems concepts:

1. **Fork-exec model** - Went from abstract concept to something we truly understand
2. **File descriptors** - The power and flexibility of Unix's "everything is a file" philosophy
3. **Process management** - Parent-child relationships, waiting, zombie processes
4. **Inter-process communication** - Pipes as a simple but powerful IPC mechanism
5. **Signal handling** - Asynchronous event handling in C++
6. **Modular design** - Breaking a complex system into manageable components

The hardest parts were debugging race conditions and understanding when to close which file descriptors. The most satisfying moment was when we got our first pipeline working correctly.

## Requirements Checklist

**All mandatory requirements completed:**

- External program execution with PATH lookup
- Argument parsing with quoted string support
- I/O redirection (`<`, `>`)
- Single-stage pipelines (`cmd1 | cmd2`)
- Background execution (`&`)
- Built-in commands (`cd`, `exit`)
- Robust error handling (no crashes)
- Zombie process cleanup
- Ctrl+C handling

**Bonus features:**

- Persistent command history

**AI Assistance:**
We used Google Gemini to assist with:

- **Learning OS concepts** - Understanding fork-exec model, file descriptors, pipes, and signal handling
- **Test case generation** - Creating comprehensive test suite (25+ test cases) to validate all shell features
- **Debugging assistance** - Help identifying and fixing bugs like pipeline deadlocks, file descriptor leaks, and race conditions

---

**Author:**

- Nitish Kumar (2025MCS2100)
- Astitwa Saxena (2025MCS3005)
