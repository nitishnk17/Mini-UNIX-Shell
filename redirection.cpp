#include "redirection.h"
#include <iostream>

// parse redirection for pipeline commands
bool parseRedirectionForPipeline(const std::vector<std::string> &args, int pipePos,
                                 std::vector<std::string> &leftSide, std::vector<std::string> &rightSide,
                                 std::string &inputFile, std::string &outputFile)
{   //left side
    for (int i = 0; i < pipePos; i++)
    {
        if (args[i] == "<")
        {
            if (i + 1 < pipePos)
            {
                inputFile = args[i + 1];
                i++;
            }
            else
            {
                std::cerr << "Syntax error: no input file specified\n";
                return false;
            }
        }
        else
        {
            leftSide.push_back(args[i]);
        }
    }

    //right side
    for (size_t i = pipePos + 1; i < args.size(); i++)
    {
        if (args[i] == ">")
        {
            if (i + 1 < args.size())
            {
                outputFile = args[i + 1];
                i++;
            }
            else
            {
                std::cerr << "Syntax error: no output file specified\n";
                return false;
            }
        }
        else
        {
            rightSide.push_back(args[i]);
        }
    }

    // validate commands
    if (leftSide.empty())
    {
        std::cerr << "Syntax error: no command before |\n";
        return false;
    }

    if (rightSide.empty())
    {
        std::cerr << "Syntax error: no command after |\n";
        return false;
    }

    return true;
}

// parse redirection for single command
bool parseRedirectionForSingleCommand(const std::vector<std::string> &args,
                                      std::vector<std::string> &cleanArgs,
                                      std::string &inputFile, std::string &outputFile)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "<")
        {
            if (i + 1 < args.size())
            {
                inputFile = args[i + 1];
                i++;
            }
            else
            {
                std::cerr << "Syntax error: no input file specified\n";
                return false;
            }
        }
        else if (args[i] == ">")
        {
            if (i + 1 < args.size())
            {
                outputFile = args[i + 1];
                i++;
            }
            else
            {
                std::cerr << "Syntax error: no output file specified\n";
                return false;
            }
        }
        else
        {
            cleanArgs.push_back(args[i]);
        }
    }

    if (cleanArgs.empty())
    {
        return false;
    }

    return true;
}
