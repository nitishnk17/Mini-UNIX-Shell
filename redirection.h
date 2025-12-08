#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <string>
#include <vector>

bool parseRedirectionForPipeline(const std::vector<std::string> &args, int pipePos,
                                 std::vector<std::string> &leftSide, std::vector<std::string> &rightSide,
                                 std::string &inputFile, std::string &outputFile);

bool parseRedirectionForSingleCommand(const std::vector<std::string> &args,
                                      std::vector<std::string> &cleanArgs,
                                      std::string &inputFile, std::string &outputFile);

#endif
