#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}



int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
        int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

char** init_args(const char* cmd_line) {
    char* args[COMMAND_MAX_ARGS + 1];   //TODO: CHECK FOR SEGFAULT
    _parseCommandLine(cmd_line, args);
    return args;
}


bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

//TODO: Should add pid field?
SmallShell::SmallShell() : prompt("smash"), plastPwd(nullptr) {
    // TODO: add your implementation
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char* cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

bool isNumber(std::string command) {
    try
    {
        int num_or_not = std::stoi(command);
    }
    catch (const std::exception&)
    {
        return false;
    }
    return true;
} 
/*----------------------------------------------------------Job Implementation----------------------------------------------------------*/

JobsList::JobEntry::JobEntry(std::string& command_line, int job_id, pid_t job_pid, bool isStopped) : command_line(command_line),
job_id(job_id),
job_pid(job_pid),
isStopped(isStopped) {}

JobsList::JobsList() : max_job_id(1),
job_vector(std::vector <JobEntry>()),
 unfinished_size(0) {}

void JobsList::killAllJobs() {
    for (auto& job : job_vector) {
        if (job.isStopped == false) {
           std::cout << job.job_pid << ": " << job.command_line << endl;
            kill(job.job_pid, SIGKILL);
        }
    }
}

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped = false) {
    this->removeFinishedJobs();
    if (job_vector.empty()) {
        std::string cmd_line = cmd->get_cmd_line();
        JobEntry job =  JobEntry(cmd_line, 1, pid); // how to create a new job entry? should i use new?
        job_vector.push_back(job); // check if written well.
        max_job_id = 1;
    }
    else {
        std::string cmd_line = cmd->get_cmd_line();
        JobEntry job =  JobEntry(cmd_line, max_job_id+1, pid); // how to create a new job entry? should i use new?????
        job_vector.push_back(job); // check if written well.
        max_job_id++;
    }
}

void JobsList::removeFinishedJobs() {
    if (job_vector.empty()) {
        max_job_id = 1;
        return;
    }
    else {
        for (auto it = job_vector.begin(); it != job_vector.end(); ++it) {
                auto job = *it;
                int ret_wait = waitpid(job.job_pid, NULL, WNOHANG); // RETURN TO IT FOR LOGIC!!!!!!!!!!!!!!!!!!!!!!!!!
                if (ret_wait == job.job_pid || ret_wait == -1) {
                    job_vector.erase(it);
                    --it;
                }
        }
        int max_alive_job_id = 1;
        for (auto& job : job_vector) {
            if (job.job_id > max_alive_job_id)
                max_alive_job_id = job.job_id;
        }

    }
}

void JobsList::removeJobById(int jobId) {
    for (auto it = job_vector.begin(); it != job_vector.end(); ++it) {
        auto job = *it;
        if (job.job_id == jobId) {
            job_vector.erase(it);
            return;
        }
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for (auto& job : job_vector) {
        if (job.job_id == jobId) {
            return &job;
        }
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {}

JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {}

bool JobsList::isListEmpty() {}


// TODO: Implement functions for class, make sure vector is sorted by job_id number when created

/*----------------------------------------------------------BUILT IN COMMANDS----------------------------------------------------------*/
ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}
void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];   //TODO: CHECK FOR SEGFAULT
    int num_args = _parseCommandLine(this->cmd, args);

    SmallShell& shell = SmallShell::getInstance();

    if (num_args > 1) {
        shell.prompt = args[1];
    }
    else if (num_args < 1) {
        //TODO: ERROR HANDLING?
    }
    else {
        shell.prompt = "smash";
    }
}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
//TODO: Should switch to shell.pid??
void ShowPidCommand::execute() {
    //SmallShell &shell = SmallShell::getInstance();
    std::cout << "smash pid is " << getpid();
}


PathWorkDirCommand::PathWorkDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void PathWorkDirCommand::execute() {
    size_t maxPathLength = (size_t)pathconf(".", _PC_PATH_MAX);

    if (maxPathLength == -1) {
        perror("Error getting maximum path length");
    }
    else {
        char buffer[maxPathLength];
        getcwd(buffer, maxPathLength);
        std::cout << buffer << std::endl;
    }
}


ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line),
plastPwd(plastPwd) {}
void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    size_t maxPathLength = (size_t)pathconf(".", _PC_PATH_MAX);
    char buffer[maxPathLength];

    if (getcwd(buffer, maxPathLength) == NULL) {
        perror("smash error: getcwd failed");
        //free_args(args, num_of_args);
        return;
    }

    SmallShell& shell = SmallShell::getInstance();

    if (num_args > 2) {
        std::cerr << "smash error: cd: too many arguments";
    }
    else if (*args[1] == '-') {                                 //Changing to previous working dir
        if (!*this->plastPwd) {
            std::cerr << "smash error: cd: OLDPWD not set";
        }
        else {
            if (chdir(shell.plastPwd) == -1) {                    //chdir SYSCALL FAILED
                perror("smash error: chdir failed");
                //free(args);
            }
            else {
                //if (*plastPwd)
                //  free(*plastPwd);
                shell.plastPwd = buffer;
            }
        }
    }
    else {                                                    //Changing to given directory
        if (chdir(args[1]) == -1) {                               //chdir SYSCALL FAILED
            perror("smash error: chdir failed");
            //free(args);
            return;
        }
        else {
            //if (*plastPwd)
            //  free(*plastPwd);
            shell.plastPwd = buffer;
        }
    }
}

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),  jobs(jobs) {}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    for (auto it = jobs->job_vector.begin(); it != jobs->job_vector.end(); ++it) { // iterating over vector of job list
        auto job = *it;
        std::cout << "[" << job.job_id << "] " << job.command_line << std::endl;
    }
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),  jobs(jobs) {} // not sure, need to check

void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);
    if (num_args > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
    else if (num_args == 2) {
        if (isNumber(args[1]) == false) { // TODO: Implement isNumber!
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
        else {
            int job_id = std::atoi(args[1]);
            if (jobs->getJobById(job_id) == NULL) {
                std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
                return;
            }
            else {
                JobsList::JobEntry* current_job = jobs->getJobById(job_id); // do i need to free it?
                std::cout << current_job->command_line << " " << current_job->job_pid << std::endl;
                // TODO: Wait for it to run, implement later
                jobs->removeJobById(job_id);
            }
        }
    }
    else {
        if (jobs->isListEmpty() == true) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }
        else {
            JobsList::JobEntry* current_job = jobs->getJobById(jobs->max_job_id); // do i need to free it?
            std::cout << current_job->command_line << " " << current_job->job_pid << std::endl;
            // TODO: Wait for it to run, implement later
            jobs->removeJobById(jobs->max_job_id);
        }
    }
    }





    QuitCommand::QuitCommand(const char* cmd_line, JobsList * jobs) : BuiltInCommand(cmd_line), jobs(jobs) {} // not sure, need to check

    void QuitCommand::execute() {
        jobs->removeFinishedJobs();
        char* args[COMMAND_MAX_ARGS + 1];
        if (string(args[1]).compare("kill") == 0) {
            std::cout << "smash: sending SIGKILL signal to " << jobs->unfinished_size << " jobs:" << endl;
            jobs->killAllJobs();
        }
        else
            exit(0);
        
    }

    KillCommand::KillCommand(const char* cmd_line, JobsList * jobs) : BuiltInCommand(cmd_line), jobs(jobs){} // not sure, need to check

    void KillCommand::execute() {
        jobs->removeFinishedJobs();
        char* args[COMMAND_MAX_ARGS + 1];
        int num_args = _parseCommandLine(this->cmd, args);
        if (num_args != 3) {
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
        }
        else {
            if (isNumber(args[1]) == false || isNumber(args[2]) == false) { // TODO: Implement isNumber!
                std::cerr << "smash error: kill: invalid arguments" << std::endl;
                return;
            }
            else {
                int job_id = std::atoi(args[2]);
                int signal_number = std::atoi(args[1]);
                if (jobs->getJobById(job_id) == NULL) {
                    std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
                    return;
                }
                else {
                    job_pid == jobs->getJobById(job_id)->job_pid;
                    if (kill(job_pid, signum) == -1) { // Not Successful
                        perror("smash error: kill failed");
                        return;
                    }
                    else {
                        std::cout << "signal number " << signal_number << " was sent to pid " << job_pid << std::endl;
                    }
                }

            }
        }






        /*----------------------------------------------------------EXTERNAL COMMANDS----------------------------------------------------------*/
        ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
        void ExternalCommand::execute() {
            auto command_arr = _trim(string(this->cmd)).c_str();


            bool is_background = _isBackgroundComamnd(this->cmd);
            if (is_background)
                _removeBackgroundSign(command_arr);

        }
