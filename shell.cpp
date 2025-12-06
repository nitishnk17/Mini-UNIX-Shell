#include<iostream>
#include<string>
#include<vector>
#include<sstream>
#include<cstring>
#include<unistd.h>
#include<sys/wait.h>
#include<cerrno>


std::vector<std::string>parseLine(const std::string& line){
    std::vector<std::string> args;
    std::stringstream ss(line);
    std::string token;

    while(ss >> token){  // >> splits by whitspace automatically
        args.push_back(token);
    }
    return args;
}

int main(){
    std::string inputLine;

    while(true){
        std::cout<<"Mini-shell> ";

        if(!std::getline(std::cin,inputLine)){ //read input
            std::cout<<"\n";
            break;
        }

        if(inputLine.empty()) continue;

        //parse input
        std::vector<std::string> args=parseLine(inputLine);
        if(args.empty()) continue;

        if(args[0]== "exit") break;

        if(args[0]=="cd"){   // for cd
            if(args.size()<2){
                std::cerr<<"cs: missing arguments\n";
            }else{
                if(chdir(args[1].c_str()) !=0){
                    std::cerr<<"cd: "<< strerror(errno)<<"\n";
                }
            }
            continue;
        }

        std::vector<char*> c_args;
        for(auto& arg:args){
            c_args.push_back(&arg[0]);
        }
        c_args.push_back(nullptr);

        pid_t pid=fork();

        if(pid<0) std::cerr<<"Fork failed: "<<strerror(errno)<< "\n";

        else if(pid==0){ //child process

            execvp(c_args[0],c_args.data());

            std::cerr<<"Error: Command not found: "<< args[0]<<"\n";
            exit(EXIT_FAILURE);
        }else{ //parent process
            int status;
            waitpid(pid,&status,0);
        }

    }
    return 0;
}