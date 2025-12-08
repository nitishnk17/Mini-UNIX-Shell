#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>

// signal handler
void sigchldHandler(int signo);
void sigintHandler(int signo);

void setupSignalHandlers();

bool isBuiltinCommand(const std::string &command);

bool handleBuiltinCommand(const std::vector<std::string> &args);

bool detectBackgroundExecution(std::vector<std::string> &args);

int findPipePosition(const std::vector<std::string> &args);

void loadHistory();
void addToHistory(const std::string &command);
void displayHistory();

#endif
