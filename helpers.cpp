#include "helpers.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cerrno>

// history
static std::vector<std::string> history;
static const std::string historyFile = ".shell_history";
static const size_t maxDisplay = 15;

// clean zombie processes
void sigchldHandler(int signo)
{
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        
    }
}

// signal handler for SIGINT (Ctrl+C)
void sigintHandler(int signo)
{
    (void)signo;
    std::cout << "\n";
    std::cout << "Mini-shell> " << std::flush;
}


void setupSignalHandlers()
{
    signal(SIGCHLD, sigchldHandler);
    signal(SIGINT, sigintHandler);
}

bool isBuiltinCommand(const std::string &command)
{
    return (command == "cd" || command == "exit" || command == "history");
}

bool handleBuiltinCommand(const std::vector<std::string> &args)
{
    if (args[0] == "exit")
    {
        exit(0);
    }

    if (args[0] == "cd")
    {
        if (args.size() < 2)
        {
            const char *home = getenv("HOME");
            if (home != nullptr)
            {
                chdir(home);
            }
        }
        else
        {
            if (chdir(args[1].c_str()) != 0)
            {
                std::cerr << "cd: " << strerror(errno) << "\n";
            }
        }
        return true;
    }

    if (args[0] == "history")
    {
        displayHistory();
        return true;
    }

    return false;
}

// detect and handle background execution
bool detectBackgroundExecution(std::vector<std::string> &args)
{
    for (size_t i = 0; i < args.size() - 1; i++)
    {
        if (args[i] == "&")
        {
            std::cerr << "Syntax error: & can only appear at end of command\n";
            args.clear();
            return false;
        }
    }

    if (args.back() == "&")
    {
        args.pop_back();
        if (args.empty())
        {
            std::cerr << "Error: No command specified\n";
            return false;
        }
        return true;
    }
    return false;
}

int findPipePosition(const std::vector<std::string> &args)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "|")
        {
            return i;
        }
    }
    return -1;
}

void loadHistory()
{
    std::ifstream histFile(historyFile);
    if (!histFile.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(histFile, line))
    {
        if (!line.empty())
        {
            history.push_back(line);
        }
    }
    histFile.close();
}

void addToHistory(const std::string &command)
{
    if (!command.empty())
    {
        history.push_back(command);

        std::ofstream histFile(historyFile, std::ios::app);
        if (histFile.is_open())
        {
            histFile << command << "\n";
            histFile.close();
        }
    }
}

void displayHistory()
{
    if (history.empty())
    {
        std::cout << "No commands in history.\n";
        return;
    }

    size_t startIdx = 0;
    if (history.size() > maxDisplay)
    {
        startIdx = history.size() - maxDisplay;
    }

    for (size_t i = startIdx; i < history.size(); i++)
    {
        std::cout << "  " << (i + 1) << "  " << history[i] << "\n";
    }
}
