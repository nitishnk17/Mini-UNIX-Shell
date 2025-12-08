#include "executor.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>

// No pipeline
void executeSingleCommand(std::vector<std::string> &cleanArgs, const std::string &inputFile, const std::string &outputFile, bool background)
{
    std::vector<char *> c_args;
    for (auto &arg : cleanArgs)
    {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "Fork failed\n";
        return;
    }
    else if (pid == 0)
    {
        if (!inputFile.empty())
        {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd < 0)
            {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (!outputFile.empty())
        {
            int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(c_args[0], c_args.data());

        std::cerr << "Error: Command not found: " << cleanArgs[0] << "\n";
        exit(EXIT_FAILURE);
    }
    else
    {
        if (background)
        {
            std::cout << "[Background] PID: " << pid << "\n";
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

// Pipeline: cmd1 | cmd2
void executePipeline(std::vector<std::string> &leftArgs, std::vector<std::string> &rightArgs, const std::string &inputFile, const std::string &outputFile, bool background)
{
    // pipefd[0] is read end, pipefd[1] is write end
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        perror("Pipe failed");
        return;
    }

    // for left command
    std::vector<char *> left_c_args;
    for (auto &arg : leftArgs)
    {
        left_c_args.push_back(const_cast<char *>(arg.c_str()));
    }
    left_c_args.push_back(nullptr);

    // for right command
    std::vector<char *> right_c_args;
    for (auto &arg : rightArgs)
    {
        right_c_args.push_back(const_cast<char *>(arg.c_str()));
    }
    right_c_args.push_back(nullptr);

    // first child for left command
    pid_t pid1 = fork();

    if (pid1 < 0)
    {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    else if (pid1 == 0)
    {
        close(pipefd[0]);

        if (!inputFile.empty())
        {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd < 0)
            {
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

    // second child for right command
    pid_t pid2 = fork();

    if (pid2 < 0)
    {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    else if (pid2 == 0)
    {
        close(pipefd[1]);

        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        if (!outputFile.empty())
        {
            int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
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

    // close both ends and wait for children
    close(pipefd[0]);
    close(pipefd[1]);

    if (background)
    {
        std::cout << "[Background] PIDs: " << pid1 << ", " << pid2 << "\n";
    }
    else
    {
        int status;
        waitpid(pid1, &status, 0);
        waitpid(pid2, &status, 0);
    }
}
