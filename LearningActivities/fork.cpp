//
// Created by Curtis Oakley on 2/12/16.
//
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int pid = fork();
    std::cout << "PID is " << pid << std::endl;
    if (pid == 0) {
        // Child
        std::cout << "about to exec" << std::endl;
        execl("/bin/ls", "/bin/ls", (char *) 0);
    } else {
        // Parent
        int status;
        wait(&status);
        std::cout << " after wait " << std::endl;
    }
    exit(4);
}