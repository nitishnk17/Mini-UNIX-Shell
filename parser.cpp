#include "parser.h"
#include <iostream>

// parse input line and handle quoted strings
std::vector<std::string> parseLine(const std::string &line)
{
    std::vector<std::string> args;
    std::string currentWord = "";
    bool insideQuotes = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < line.length(); i++)
    {
        char ch = line[i];

        if ((ch == '"' || ch == '\'') && !insideQuotes)
        {
            insideQuotes = true;
            quoteChar = ch;
        }
        else if (ch == quoteChar && insideQuotes)
        {
            insideQuotes = false;
            quoteChar = '\0';
        }
        else if ((ch == ' ' || ch == '\t') && !insideQuotes)
        {
            if (!currentWord.empty())
            {
                args.push_back(currentWord);
                currentWord = "";
            }
        }
        else
        {
            currentWord += ch;
        }
    }

    if (insideQuotes)
    {
        std::cerr << "Error: Unclosed quote\n";
        return std::vector<std::string>();
    }

    if (!currentWord.empty())
    {
        args.push_back(currentWord);
    }

    return args;
}
