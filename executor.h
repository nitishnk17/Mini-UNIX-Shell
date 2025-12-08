#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include <vector>

// No pipeline
void executeSingleCommand(std::vector<std::string> &cleanArgs, const std::string &inputFile, const std::string &outputFile, bool background);

// pipeline: cmd1 | cmd2
void executePipeline(std::vector<std::string> &leftArgs, std::vector<std::string> &rightArgs, const std::string &inputFile, const std::string &outputFile, bool background);

#endif
