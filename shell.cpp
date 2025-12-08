#include <iostream>
#include <string>
#include <vector>
#include "parser.h"
#include "helpers.h"
#include "redirection.h"
#include "executor.h"

void processCommand(std::vector<std::string> &args)
{
    bool background = detectBackgroundExecution(args);
    if (args.empty())
    {
        return;
    }

    int pipePosition = findPipePosition(args);

    if (pipePosition != -1)
    {
        std::vector<std::string> leftSide, rightSide;
        std::string inputFile, outputFile;

        if (parseRedirectionForPipeline(args, pipePosition, leftSide, rightSide, inputFile, outputFile))
        {
            if (!leftSide.empty() && isBuiltinCommand(leftSide[0]))
            {
                std::cerr << "Error: Built-in commands cannot be used in pipelines: " << leftSide[0] << "\n";
                return;
            }
            if (!rightSide.empty() && isBuiltinCommand(rightSide[0]))
            {
                std::cerr << "Error: Built-in commands cannot be used in pipelines: " << rightSide[0] << "\n";
                return;
            }

            executePipeline(leftSide, rightSide, inputFile, outputFile, background);
        }
    }
    else
    {
        if (handleBuiltinCommand(args))
        {
            return;
        }

        std::vector<std::string> cleanArgs;
        std::string inputFile, outputFile;

        if (parseRedirectionForSingleCommand(args, cleanArgs, inputFile, outputFile))
        {
            executeSingleCommand(cleanArgs, inputFile, outputFile, background);
        }
    }
}

int main()
{
    setupSignalHandlers();

    loadHistory();

    std::string inputLine;

    while (true)
    {
        std::cout << "Mini-shell> " << std::flush;

        if (!std::getline(std::cin, inputLine))
        {
            std::cout << "\n";
            break;
        }

        if (inputLine.empty())
            continue;

        addToHistory(inputLine);

        std::vector<std::string> args = parseLine(inputLine);

        if (args.empty())
            continue;

        processCommand(args);
    }

    return 0;
}
