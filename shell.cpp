

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cctype>

extern char **environ; // for env builtin

// Function declarations
std::vector<std::string> parseLine(const std::string& line);
void executeSingleCommand(std::vector<std::string>& cleanArgs,
                          const std::string& inputFile,
                          const std::string& outputFile,
                          bool append,
                          bool background);
void executePipelineChain(std::vector<std::vector<std::string>>& commands,
                          const std::string& inputFile,
                          const std::string& outputFile,
                          bool append,
                          bool background);

// --- expand $VAR in arguments ---
void expandEnvironment(std::vector<std::string>& args) {
    for (auto &arg : args) {
        std::string result;
        for (size_t i = 0; i < arg.size();) {
            if (arg[i] == '$') {
                size_t j = i + 1;
                while (j < arg.size() &&
                       (std::isalnum(static_cast<unsigned char>(arg[j])) ||
                        arg[j] == '_')) {
                    j++;
                }
                std::string varName = arg.substr(i + 1, j - (i + 1));
                const char* val = getenv(varName.c_str());
                if (val) {
                    result += val;
                }
                i = j;
            } else {
                result += arg[i];
                i++;
            }
        }
        arg = result;
    }
}

// Parser
std::vector<std::string> parseLine(const std::string& line) {
    std::vector<std::string> args;
    std::string currentWord = "";
    bool insideQuotes = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < line.length(); i++) {
        char ch = line[i];

        if ((ch == '"' || ch == '\'') && !insideQuotes) {
            insideQuotes = true;
            quoteChar = ch;
        }
        else if (ch == quoteChar && insideQuotes) {
            insideQuotes = false;
            quoteChar = '\0';
        }
        // If space or tab and not inside quotes
        else if ((ch == ' ' || ch == '\t') && !insideQuotes) {
            if (!currentWord.empty()) {
                args.push_back(currentWord);
                currentWord = "";
            }
        }
        else {
            currentWord += ch;
        }
    }

    if (insideQuotes) {
        std::cerr << "Error: Unclosed quote\n";
        return std::vector<std::string>();
    }

    if (!currentWord.empty()) {
        args.push_back(currentWord);
    }

    return args;
}

// Execute a single command (no pipeline)
void executeSingleCommand(std::vector<std::string>& cleanArgs,
                          const std::string& inputFile,
                          const std::string& outputFile,
                          bool append,
                          bool background) {
    std::vector<char*> c_args;
    for (auto &arg : cleanArgs) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed\n";
        return;
    } else if (pid == 0) { // child

        // input redirection
        if (!inputFile.empty()) {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // output redirection (truncate vs append)
        if (!outputFile.empty()) {
            int flags = O_WRONLY | O_CREAT;
            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            int fd = open(outputFile.c_str(), flags, 0644);
            if (fd < 0) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(c_args[0], c_args.data());

        std::cerr << "Error: Command not found: "
                  << cleanArgs[0] << "\n";
        exit(EXIT_FAILURE);
    } else { // parent process
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            std::cout << "[bg pid " << pid << "]\n";
        }
    }
}

// Execute chain: cmd1 | cmd2 | ... | cmdN  (+ optional redirection and bg)
void executePipelineChain(std::vector<std::vector<std::string>>& commands,
                          const std::string& inputFile,
                          const std::string& outputFile,
                          bool append,
                          bool background) {
    int n = commands.size();
    if (n == 0) return;

    std::vector<pid_t> pids;
    pids.reserve(n);

    int prev_read_fd = -1;

    for (int k = 0; k < n; ++k) {
        int pipefd[2] = {-1, -1};
        if (k < n - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return;
            }
        }

        std::vector<char*> c_args;
        for (auto &arg : commands[k]) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (pipefd[0] != -1) close(pipefd[0]);
            if (pipefd[1] != -1) close(pipefd[1]);
            return;
        } else if (pid == 0) {
            // child

            // if not first command: stdin from prev_read_fd
            if (k > 0) {
                dup2(prev_read_fd, STDIN_FILENO);
            }

            // if not last command: stdout to pipe write end
            if (k < n - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }

            // close unused fds in child
            if (prev_read_fd != -1) close(prev_read_fd);
            if (pipefd[0] != -1) close(pipefd[0]);
            if (pipefd[1] != -1) close(pipefd[1]);

            // first command: possible input redirection
            if (k == 0 && !inputFile.empty()) {
                int fd = open(inputFile.c_str(), O_RDONLY);
                if (fd < 0) {
                    perror("Failed to open input file");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // last command: possible output redirection
            if (k == n - 1 && !outputFile.empty()) {
                int flags = O_WRONLY | O_CREAT;
                if (append) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                int fd = open(outputFile.c_str(), flags, 0644);
                if (fd < 0) {
                    perror("Failed to open output file");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(c_args[0], c_args.data());
            std::cerr << "Error: Command not found: "
                      << commands[k][0] << "\n";
            exit(EXIT_FAILURE);
        } else {
            // parent
            pids.push_back(pid);

            // close old read end
            if (prev_read_fd != -1) close(prev_read_fd);

            // close write end in parent, keep read end for next command
            if (pipefd[1] != -1) close(pipefd[1]);
            prev_read_fd = pipefd[0];
        }
    }

    // close last read end in parent
    if (prev_read_fd != -1) close(prev_read_fd);

    if (!background) {
        for (pid_t p : pids) {
            int status;
            waitpid(p, &status, 0);
        }
    } else {
        std::cout << "[bg pids";
        for (pid_t p : pids) std::cout << " " << p;
        std::cout << " ]\n";
    }
}

int main() {
    std::string inputLine;
    std::vector<std::string> history;   // command history

    while (true) {
        std::cout << "Mini-shell> " << std::flush;

        if (!std::getline(std::cin, inputLine)) {
            std::cout << "\n";
            break;
        }

        if (inputLine.empty()) continue;

        // !! (repeat last command)
        if (inputLine == "!!") {
            if (history.empty()) {
                std::cerr << "No commands in history\n";
                continue;
            }
            inputLine = history.back();
            std::cout << inputLine << "\n";
        }

        // store in history
        history.push_back(inputLine);

        // parse input
        std::vector<std::string> args = parseLine(inputLine);
        if (args.empty()) continue;

        // expand $VAR
        expandEnvironment(args);
        if (args.empty()) continue;

        // background? (trailing &)
        bool background = false;
        if (!args.empty() && args.back() == "&") {
            background = true;
            args.pop_back();
            if (args.empty()) {
                std::cerr << "Syntax error: '&' with no command\n";
                continue;
            }
        }

        if (args[0] == "exit") break;

        // cd builtin
        if (args[0] == "cd") {
            if (args.size() < 2) {
                const char* home = getenv("HOME");
                if (home != nullptr) {
                    chdir(home);
                }
            } else {
                if (chdir(args[1].c_str()) != 0) {
                    std::cerr << "cd: " << strerror(errno) << "\n";
                }
            }
            continue;
        }

        // history builtin
        if (args[0] == "history") {
            for (size_t i = 0; i < history.size(); ++i) {
                std::cout << i + 1 << "  " << history[i] << "\n";
            }
            continue;
        }

        // env builtin
        if (args[0] == "env") {
            for (char **e = environ; *e != nullptr; ++e) {
                std::cout << *e << "\n";
            }
            continue;
        }

        // setenv VAR VALUE
        if (args[0] == "setenv") {
            if (args.size() < 3) {
                std::cerr << "Usage: setenv VAR VALUE\n";
            } else {
                if (setenv(args[1].c_str(), args[2].c_str(), 1) != 0) {
                    perror("setenv");
                }
            }
            continue;
        }

        // unsetenv VAR
        if (args[0] == "unsetenv") {
            if (args.size() < 2) {
                std::cerr << "Usage: unsetenv VAR\n";
            } else {
                if (unsetenv(args[1].c_str()) != 0) {
                    perror("unsetenv");
                }
            }
            continue;
        }

        // --- pipeline + redirection parsing (supports multiple pipes) ---

        // find pipe positions
        std::vector<size_t> pipePositions;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "|") pipePositions.push_back(i);
        }

        std::string inputFile;
        std::string outputFile;
        bool append = false;
        bool syntaxError = false;

        if (!pipePositions.empty()) {
            // multiple commands in pipeline
            std::vector<std::vector<std::string>> commands;

            size_t start = 0;
            int numCmds = pipePositions.size() + 1;

            for (int cmdIndex = 0; cmdIndex < numCmds; ++cmdIndex) {
                size_t end = (cmdIndex < (int)pipePositions.size())
                             ? pipePositions[cmdIndex]
                             : args.size();

                std::vector<std::string> cmd;
                for (size_t i = start; i < end; ++i) {
                    const std::string& tok = args[i];

                    if (tok == "<") {
                        if (cmdIndex != 0) {
                            std::cerr << "Syntax error: input redirection only allowed in first command of pipeline\n";
                            syntaxError = true;
                            break;
                        }
                        if (i + 1 < end) {
                            inputFile = args[i + 1];
                            ++i;
                        } else {
                            std::cerr << "Syntax error: no input file specified\n";
                            syntaxError = true;
                            break;
                        }
                    } else if (tok == ">" || tok == ">>") {
                        if (cmdIndex != numCmds - 1) {
                            std::cerr << "Syntax error: output redirection only allowed in last command of pipeline\n";
                            syntaxError = true;
                            break;
                        }
                        if (i + 1 < end) {
                            outputFile = args[i + 1];
                            append = (tok == ">>");
                            ++i;
                        } else {
                            std::cerr << "Syntax error: no output file specified\n";
                            syntaxError = true;
                            break;
                        }
                    } else {
                        cmd.push_back(tok);
                    }
                }

                if (syntaxError) break;
                if (cmd.empty()) {
                    std::cerr << "Syntax error: empty command in pipeline\n";
                    syntaxError = true;
                    break;
                }

                commands.push_back(cmd);
                start = end + 1; // skip '|'
            }

            if (syntaxError) continue;

            executePipelineChain(commands, inputFile, outputFile, append, background);
        }
        else {
            // single command (no pipes)
            std::string inFile;
            std::string outFile;
            std::vector<std::string> cleanArgs;

            for (size_t i = 0; i < args.size(); i++) {
                if (args[i] == "<") {
                    if (i + 1 < args.size()) {
                        inFile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Syntax error: no input file specified\n";
                        syntaxError = true;
                        break;
                    }
                } else if (args[i] == ">" || args[i] == ">>") {
                    if (i + 1 < args.size()) {
                        outFile = args[i + 1];
                        append = (args[i] == ">>");
                        i++;
                    } else {
                        std::cerr << "Syntax error: no output file specified\n";
                        syntaxError = true;
                        break;
                    }
                } else {
                    cleanArgs.push_back(args[i]);
                }
            }

            if (syntaxError || cleanArgs.empty()) {
                continue;
            }

            executeSingleCommand(cleanArgs, inFile, outFile, append, background);
        }
    }
    return 0;
}
