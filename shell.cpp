#include<iostream>
#include<string>
#include<vector>
#include<sstream>
#include<cstring>
#include<unistd.h>
#include<sys/wait.h>
#include<cerrno>


// asdfsd
using namespace std;

vector<string>parseLine(const string& line){
    vector<string> args;
    stringstream ss(line);
    string token;

    while(ss >> token){  // >> splits by whitspace automatically
        args.push_back(token);
    }
    return args;
}

int main(){
    string inputLine;

    while(true){
        cout<<"Mini-shell> ";

        if(!getline(cin,inputLine)){ //read input
            cout<<"\n";
            break;
        }

        if(inputLine.empty()) continue;

        //parse input
        vector<string> args=parseLine(inputLine);
        if(args.empty()) continue;

        if(args[0]== "exit") break;

        if(args[0]=="cd"){
            if(args.size()<2){
                cerr<<"cs: missing arguments\n";
            }else{
                if(chdir(args[1].c_str()) !=0){
                    cerr<<"cd: "<< strerror(errno)<<"\n";
                }
            }
            continue;
        }

        vector<char*> c_args;
        for(auto& arg:args){
            c_args.push_back(&arg[0]);
        }
        c_args.push_back(nullptr);

        pid_t pid=fork();

        if(pid<0) cerr<<"Fork failed: "<<strerror(errno)<< "\n";

        else if(pid==0){ //child process

            execvp(c_args[0],c_args.data());

            cerr<<"Error: Command not found: "<< args[0]<<"\n";
            exit(EXIT_FAILURE);
        }else{ //parent process
            int status;
            waitpid(pid,&status,0);
        }

    }
    return 0;
}