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

// Parsing input line & handle quoted strings
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
        // Regular character 
        else {
            currentWord += ch;
        }
    }

    // Check for unclosed quotes
    if (insideQuotes) {
        std::cerr << "Error: Unclosed quote\n";
        return std::vector<std::string>(); 
    }

    // Add last word 
    if (!currentWord.empty()) {
        args.push_back(currentWord);
    }

    return args;
}

int main() {
    std::string inputLine;

    while (true) {
        std::cout << "Mini-shell> " << std::flush;

        if (!std::getline(std::cin, inputLine)) { // read input
            std::cout << "\n";
            break;
        }

        if (inputLine.empty()) continue;

        // parse input
        std::vector<std::string> args = parseLine(inputLine);
        if (args.empty()) continue;

        if (args[0] == "exit") break;

        if (args[0] == "cd") {   // for cd
            if (args.size() < 2) {
                // No argument - go to HOME directory
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

        // Skip execution if syntax error or no command
        if (syntaxError || cleanArgs.empty()) {
            continue;
        }

        std::vector<char*> c_args;
        for (auto &arg : cleanArgs) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Fork failed\n";
            continue;
        } else if (pid == 0) { // child process

            if (!inputFile.empty()) {
                int fd = open(inputFile.c_str(), O_RDONLY);
                if (fd < 0) {
                    perror("Failed to open input file");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (!outputFile.empty()) {
                // O_WRONLY: Write only
                // O_CREAT: Create file if it doesn't exist
                // O_TRUNC: Truncate file (clear it) if it exists
                // 0644: File permissions (rw-r--r--)
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
    return 0;
}
