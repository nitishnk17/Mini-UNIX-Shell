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

// Function declaration
std::vector<std::string> parseLine(const std::string& line);
void executeSingleCommand(std::vector<std::string>& cleanArgs,
                          const std::string& inputFile,
                          const std::string& outputFile);
void executePipeline(std::vector<std::string>& leftArgs,
                     std::vector<std::string>& rightArgs,
                     const std::string& inputFile,
                     const std::string& outputFile);

// --- NEW: expand $VAR in arguments ---
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

    // Check for unclosed quotes
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
                          const std::string& outputFile) {
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

        // Handle input redirection
        if (!inputFile.empty()) {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Handle output redirection
        if (!outputFile.empty()) {
            int fd = open(outputFile.c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
        int status;
        waitpid(pid, &status, 0);
    }
}

void executePipeline(std::vector<std::string>& leftArgs,
                     std::vector<std::string>& rightArgs,
                     const std::string& inputFile,
                     const std::string& outputFile){
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("Pipe failed");
        return;
    }

    std::vector<char*> left_c_args;
    for (auto &arg : leftArgs) {
        left_c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    left_c_args.push_back(nullptr);

    std::vector<char*> right_c_args;
    for (auto &arg : rightArgs) {
        right_c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    right_c_args.push_back(nullptr);

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid1 == 0) {
        close(pipefd[0]);

        if (!inputFile.empty()) {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(left_c_args[0], left_c_args.data());

        std::cerr << "Error: Command not found: " << leftArgs[0] << "\n";
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();

    if (pid2 < 0) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid2 == 0) {
        close(pipefd[1]);

        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        if (!outputFile.empty()) {
            int fd = open(outputFile.c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(right_c_args[0], right_c_args.data());

        std::cerr << "Error: Command not found: " << rightArgs[0] << "\n";
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
}

int main() {
    std::string inputLine;
    std::vector<std::string> history;   // NEW: command history

    while (true) {
        std::cout << "Mini-shell> " << std::flush;

        if (!std::getline(std::cin, inputLine)) {
            std::cout << "\n";
            break;
        }

        if (inputLine.empty()) continue;

        // NEW: handle !! (repeat last command)
        if (inputLine == "!!") {
            if (history.empty()) {
                std::cerr << "No commands in history\n";
                continue;
            }
            inputLine = history.back();
            std::cout << inputLine << "\n";
        }

        // store *expanded* command line in history
        history.push_back(inputLine);

        // parse input
        std::vector<std::string> args = parseLine(inputLine);
        if (args.empty()) continue;

        // expand $VAR
        expandEnvironment(args);
        if (args.empty()) continue;

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

        // NEW: history builtin
        if (args[0] == "history") {
            for (size_t i = 0; i < history.size(); ++i) {
                std::cout << i + 1 << "  " << history[i] << "\n";
            }
            continue;
        }

        // NEW: env builtin
        if (args[0] == "env") {
            for (char **e = environ; *e != nullptr; ++e) {
                std::cout << *e << "\n";
            }
            continue;
        }

        // NEW: setenv VAR VALUE
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

        // NEW: unsetenv VAR
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

        // Check for pipeline operator
        int pipePosition = -1;
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == "|") {
                pipePosition = static_cast<int>(i);
                break;
            }
        }

        if (pipePosition != -1) {
            std::vector<std::string> leftSide;
            std::string inputFile;
            bool syntaxError = false;

            for (int i = 0; i < pipePosition; i++) {
                if (args[i] == "<") {
                    if (i + 1 < pipePosition) {
                        inputFile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Syntax error: no input file specified\n";
                        syntaxError = true;
                        break;
                    }
                } else {
                    leftSide.push_back(args[i]);
                }
            }

            std::vector<std::string> rightSide;
            std::string outputFile;

            for (size_t i = pipePosition + 1; i < args.size(); i++) {
                if (args[i] == ">") {
                    if (i + 1 < args.size()) {
                        outputFile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Syntax error: no output file specified\n";
                        syntaxError = true;
                        break;
                    }
                } else {
                    rightSide.push_back(args[i]);
                }
            }

            if (syntaxError) {
                continue;
            }

            if (leftSide.empty()) {
                std::cerr << "Syntax error: no command before |\n";
                continue;
            }

            if (rightSide.empty()) {
                std::cerr << "Syntax error: no command after |\n";
                continue;
            }

            executePipeline(leftSide, rightSide, inputFile, outputFile);
        }
        else {
            std::string inputFile;
            std::string outputFile;
            std::vector<std::string> cleanArgs;
            bool syntaxError = false;

            for (size_t i = 0; i < args.size(); i++) {
                if (args[i] == "<") {
                    if (i + 1 < args.size()) {
                        inputFile = args[i + 1];
                        i++;
                    } else {
                        std::cerr << "Syntax error: no input file specified\n";
                        syntaxError = true;
                        break;
                    }
                } else if (args[i] == ">") {
                    if (i + 1 < args.size()) {
                        outputFile = args[i + 1];
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

            executeSingleCommand(cleanArgs, inputFile, outputFile);
        }
    }
    return 0;
}

