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

// Function declaration
std::vector<std::string> parseLine(const std::string& line);
void executeSingleCommand(std::vector<std::string>& cleanArgs,const std::string& inputFile,const std::string& outputFile);
void executePipeline(std::vector<std::string>& leftArgs,std::vector<std::string>& rightArgs,const std::string& inputFile,const std::string& outputFile);

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
void executeSingleCommand(std::vector<std::string>& cleanArgs,const std::string& inputFile,const std::string& outputFile) {
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
            int fd = open(outputFile.c_str(),O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

void executePipeline(std::vector<std::string>& leftArgs,std::vector<std::string>& rightArgs,const std::string& inputFile,const std::string& outputFile){

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

        // Check if there's a pipeline operator
        int pipePosition = -1;
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == "|") {
                pipePosition = i;
                break;
            }
        }

        // If pipeline exists, split into left and right parts
        if (pipePosition != -1) {
            // Extract left side (before |)
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

            // Extract right side (after |)
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

            // Check for errors
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

            // Execute pipeline
            executePipeline(leftSide, rightSide, inputFile, outputFile);
        }
        else {
            // No pipeline - single command execution
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

            // Execute single command
            executeSingleCommand(cleanArgs, inputFile, outputFile);
        }
    }
    return 0;
}
