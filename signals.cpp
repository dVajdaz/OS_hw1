#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C" << std::endl;
    SmallShell &smash = SmallShell::getInstance();
    if(smash.fgp != -1){
        int process_to_kill = smash.fgp;
        smash.fgp = -1;
        kill(process_to_kill, SIGINT);
        cout << "smash: process " << process_to_kill << " was killed" << endl;
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

