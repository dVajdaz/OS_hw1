#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"
#include <bits/stdc++.h>


int main(int argc, char* argv[]) {
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    struct sigaction act = {{nullptr}};
    act.sa_flags = SA_RESTART;
    act.sa_handler = alarmHandler;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGALRM, &act, nullptr) == -1) {
        perror("smash error: failed to set alarm handler");
    }
    SmallShell& smash = SmallShell::getInstance();

    std::vector<JobsList::JobEntry> vect;
    smash.jobs = new JobsList (1,vect,0);

    std::vector<AlarmList::AlarmEntry> alarms_vect;
    smash.alarms = new AlarmList(alarms_vect);
    while(true) {
        std::cout << smash.prompt << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if(!cmd_line.empty())
            smash.executeCommand(cmd_line.c_str(), nullptr);
    }
    return 0;
}