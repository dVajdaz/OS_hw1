#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

#include <fcntl.h>

#include <sys/stat.h>

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

const std::string WHITESPACE = "\t\r\f\n\v ";

string _ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
string _rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
string _trim(const std::string& s) {
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

void free_args(char** args, int arg_num) {
    if (!args)
        return;

    for (int i = 0; i < arg_num; i++) {
        if (args[i])
            free(args[i]);
    }
    //free(args);
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
bool isValidfilePermission(const char* command) {
    int num = std::atoi(command);
    if (num > 777 || num < 0)
        return false;
    else {
        while (num >= 1) {
            if (num % 10 > 7)
                return false;
            num = num / 10;
        }
        return true;
    }
}

//TODO: Should add pid field?
SmallShell::SmallShell() : prompt("smash"), plastPwd(nullptr), jobs() {}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command::Command(const char* cmd_line) : cmd(cmd_line) {}
Command* SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (strstr(cmd_line, ">") != nullptr || strstr(cmd_line, ">>") != nullptr) {
        return new RedirectionCommand(cmd_line);
    }

    if (firstWord.compare("chprompt") == 0|| firstWord.compare("chprompt&") == 0) {
        return new ChpromptCommand(cmd_line);
    }
        /*
        else if (firstWord.compare("pwd") == 0) {
          return new GetCurrDirCommand(cmd_line);
        }*/
    else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0|| firstWord.compare("pwd&") == 0) {
        return new PathWorkDirCommand(cmd_line);
    }
    else if (firstWord.compare("kill") == 0|| firstWord.compare("kill&") == 0) {
        return new KillCommand(cmd_line, jobs);
    }
    else if (firstWord.compare("quit") == 0|| firstWord.compare("quit&") == 0) {
        return new QuitCommand(cmd_line, jobs);
    }
    else if (firstWord.compare("jobs") == 0|| firstWord.compare("jobs&") == 0) {
        return new JobsCommand(cmd_line, jobs);
    }
    else if (firstWord.compare("fg") == 0|| firstWord.compare("fg&") == 0) {
        return new ForegroundCommand(cmd_line, jobs);
    }
    else if (firstWord.compare("cd") == 0|| firstWord.compare("cd&") == 0) {
        return new ChangeDirCommand(cmd_line, &plastPwd);
    }
    else if (firstWord.compare("chmod") == 0|| firstWord.compare("chmod&") == 0) {
        return new ChmodCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}
void SmallShell::executeCommand(const char* cmd_line) {
    // for example:
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    delete cmd;
    this->fgp = -1;
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}



/*----------------------------------------------------------BUILT IN COMMANDS----------------------------------------------------------*/
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {
    if(_isBackgroundComamnd(cmd_line)){
        char cmd_line_copy[COMMAND_ARGS_MAX_LENGTH+1];

        strcpy(cmd_line_copy, cmd_line);
        _removeBackgroundSign(cmd_line_copy);

        std::string cmd_string = _trim(cmd_line_copy);

        char *new_cmd_line = (char *) malloc(sizeof(char) * (cmd_string.length() + 1)); //TODO: There is a leak
        strcpy(new_cmd_line, cmd_line_copy);
        this->cmd = new_cmd_line;
    }
}

ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
}
void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    SmallShell& smash = SmallShell::getInstance();

    if (num_args > 1) {
        smash.prompt = args[1];
    }
    else if (num_args < 1) {
        //TODO: ERROR HANDLING?
    }
    else {
        smash.prompt = "smash";
    }

    free_args(args, num_args);
}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute() {
    //SmallShell &smash = SmallShell::getInstance();
    std::cout << "smash pid is " << getpid() << endl;
}   //TODO: Should switch to shell.pid??

PathWorkDirCommand::PathWorkDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void PathWorkDirCommand::execute() {
    size_t maxPathLength = (size_t)pathconf(".", _PC_PATH_MAX);

    if (maxPathLength == -1) {
        perror("Error getting maximum path length");
    }
    else {
        char buffer[maxPathLength];
        getcwd(buffer, maxPathLength);
        cout << buffer << endl;
    }
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {}
void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    char* buffer = (char*)malloc((size_t)pathconf(".", _PC_PATH_MAX));
    if (getcwd(buffer, (size_t)pathconf(".", _PC_PATH_MAX)) == NULL) {
        perror("smash error: getcwd failed");
        free(buffer);
        free_args(args, num_args);
        return;
    }

    if (num_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << endl;
    }
    else if (num_args == 1) {
        //TODO: ERROR HANDLING?
    }
    else if ((std::string)args[1] == "-") {   //Changing to previous working dir
        if (!(*plastPwd)) {
            std::cerr << "smash error: cd: OLDPWD not set" << endl;
            free_args(args, num_args);
        }
        else if (chdir(*plastPwd) == -1) {
            perror("smash error: chdir failed");
            free_args(args, num_args);
        }
        else {
            if (*plastPwd)
                free(*plastPwd);
            *plastPwd = buffer;
        }
    }
    else if (chdir(args[1]) == -1) {    //Changing to given directory
        perror("smash error: chdir failed");
        free_args(args, num_args);
    }
    else {
        if (*plastPwd)
            free(*plastPwd);
        *plastPwd = buffer;
    }
    free_args(args, num_args);
}

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    SmallShell& smash = SmallShell::getInstance();
    for (auto it = jobs->job_vector.begin(); it != jobs->job_vector.end(); ++it) { // iterating over vector of job list
        auto job = *it;
        std::cout << "[" << job.job_id << "] " << job.command_line << std::endl;
    }
    smash.jobs = jobs;
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void ForegroundCommand::execute() {
    jobs->removeFinishedJobs();
    SmallShell& smash = SmallShell::getInstance();
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);
    if (num_args > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
    else if (num_args == 2) {
        if (isNumber(args[1]) == false) {
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
                smash.fgp = current_job->job_pid;
                std::cout << current_job->command_line << " " << current_job->job_pid << std::endl;
                if (waitpid(current_job->job_pid, NULL, WUNTRACED) == -1) {
                    free_args(args, num_args);
                    return;
                }
                jobs->removeJobById(job_id);
            }
        }
    }
    else {
        if (jobs->job_vector.empty() == true) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }
        else {
            JobsList::JobEntry* current_job = jobs->getJobById(jobs->max_job_id); // do i need to free it?
            std::cout << current_job->command_line << " " << current_job->job_pid << std::endl;
            smash.fgp = current_job->job_pid;
            if (waitpid(current_job->job_pid, NULL, WUNTRACED) == -1) {
                free_args(args, num_args);
                return;
            }
            jobs->removeJobById(jobs->max_job_id);
        }
    }
    smash.jobs = jobs;
    free_args(args, num_args);
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
void QuitCommand::execute() {
    jobs->removeFinishedJobs();
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    if (num_args > 1){
        if (std::string(args[1]).compare("kill") == 0) {
            std::cout << "smash: sending SIGKILL signal to " << jobs->unfinished_size << " jobs:" << endl;
            jobs->killAllJobs();
        }
    }
    free_args(args, num_args);
    delete this;
    exit(0);

}

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {} //
void KillCommand::execute() {
    jobs->removeFinishedJobs();
    SmallShell& smash = SmallShell::getInstance();
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);
    if (num_args != 3) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
    }
    else {
        if (isNumber(args[1]) == false || isNumber(args[2]) == false) {
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
                pid_t job_pid = jobs->getJobById(job_id)->job_pid;
                if (kill(job_pid, signal_number) == -1) { // Not Successful
                    perror("smash error: kill failed");
                    return;
                }
                else {
                    std::cout << "signal number " << signal_number << " was sent to pid " << job_pid << std::endl;
                }
            }

        }
    }
    smash.jobs = jobs;
    free_args(args, num_args);
}



/*----------------------------------------------------------EXTERNAL COMMANDS----------------------------------------------------------*/
bool containsSpecialCharacter(const char* str) {
    // Iterate through each character in the string
    while (*str) {
        if (*str == '?' || *str == '*') {
            return true; // Special character found
        }
        str++;
    }
    return false; // No special character found
}
bool arrayContainsSpecialCharacter(char* array[], size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (containsSpecialCharacter(array[i])) {
            return true; // Array element contains a special character
        }
    }
    return false; // No special character found in any array element
}

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
void ExternalCommand::execute() {
    std::string trim_cmd_string = _trim(string(this->cmd));
    char cmd_line_copy[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_line_copy, trim_cmd_string.c_str());

    char bash_exec[] = "/bin/bash";
    char bash_flag[] = "-c";
    char *bash_args[] = {bash_exec, bash_flag, cmd_line_copy, nullptr};

    bool is_background = _isBackgroundComamnd(cmd_line_copy);
    if (is_background)
        _removeBackgroundSign(cmd_line_copy);

    pid_t pid = fork();
    if (pid == 0) {                                                  //CHILD PROCESS
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }

        char *args[COMMAND_MAX_ARGS + 1];
        int num_args = _parseCommandLine(cmd_line_copy, args);

        if (arrayContainsSpecialCharacter(args, num_args)) {         //COMPLEX EXTERNAL COMMAND
            if (execv(bash_exec, bash_args) == -1) {
                perror("smash error: execv failed");
                return;
            }
        } else if (execvp(args[0], args) == -1){
            std::cout << " \n" << args[0] << "\n" << std::endl;                 //SIMPLE EXTERNAL COMMAND
            std::cout << " \n" << "PENIS" << "\n" << std::endl;
            perror("smash error: execv failed");
            return;
        }
    } else {                                                         //PARENT PROCESS
        if(is_background){
            SmallShell &smash = SmallShell::getInstance();
            smash.jobs->addJob(this, pid,false);
        } else {
            int status;
            if(waitpid(pid, &status, 0) == -1){     //TODO: Switch to wait??
                perror("smash error: waitpid failed");
                return;
            }
        }
    }
}



/*----------------------------------------------------------REDIRECTION COMMAND----------------------------------------------------------*/
RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    const char* outputRedirect = nullptr;

    for (int i = 0; args[i] != nullptr; ++i) {
        // Check if the current string contains ">" or ">>"
        if (strstr(args[i], ">>") != nullptr) {
            outputRedirect = ">>";
            this->toAppend = true;
            this->filename = _trim(args[i + 1]);
            break;
        }
        else if (strstr(args[i], ">") != nullptr) {
            outputRedirect = ">";
            this->toAppend = false;
            this->filename = _trim(args[i + 1]);                         //TODO: Should use strcpy for the file field??
            break;
        }
    }

    if (outputRedirect != nullptr) {
        this->prepare();
    }

    free_args(args, num_args);
}
void RedirectionCommand::prepare() {
    this->monitor_out = dup(1);

    if (close(1) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (this->toAppend) {
        this->opened_file_d = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
    }
    else {
        this->opened_file_d = open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    }

    if (opened_file_d == -1) {
        perror("smash error: open failed");
        this->redirected = false;
    }
}

void RedirectionCommand::cleanup() {
    if (close(this->opened_file_d) == -1)
        perror("smash error: close failed");

    if (dup2(this->monitor_out, 1) == -1)
        perror("smash error: dup2 failed");

    if (close(this->monitor_out) == -1)
        perror("smash error: close failed");
}

void RedirectionCommand::execute() {
    if (this->redirected) {
        SmallShell& smash = SmallShell::getInstance();

        const char* redirectPos = strpbrk(cmd, ">");
        size_t length = redirectPos - cmd;
        char* command_line = (char*)malloc(sizeof(char) * (length + 1));
        strncpy(command_line, cmd, length);
        command_line[length] = '\0'; // Null-terminate the string

        smash.executeCommand(command_line);              //TODO: command_line is then built into Command object and deleted after. Requires Valgrind check!
        //cleanup();
    }
    cleanup();
}



/*----------------------------------------------------------CHMOD COMMAND----------------------------------------------------------*/
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ChmodCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);
    if (isNumber(args[1]) == false || isValidfilePermission(args[1]) == false) { // should I also check if path to file is valid?
        std::cerr << "smash error: chmod: invalid arguments" << std::endl;
    }
    else {
        mode_t new_mode = stoi(args[1], nullptr,8);

        if(chmod(args[2],new_mode) == -1)
            perror("smash error: chmod failed");
    }

    free_args(args, num_args);
}



/*----------------------------------------------------------Job Implementation----------------------------------------------------------*/
JobsList::JobEntry::JobEntry(std::string& command_line, int job_id, pid_t job_pid, bool isStopped) : command_line(command_line),
                                                                                                     job_id(job_id),
                                                                                                     job_pid(job_pid),
                                                                                                     isStopped(isStopped) {}

JobsList::JobsList( int max_job_id,std::vector <JobEntry> job_vector,int unfinished_size) : max_job_id(1),
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

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped) {
    this->removeFinishedJobs();
    if (job_vector.empty()) {
        std::string cmd_line = cmd->get_cmd_line();
        JobEntry job = JobEntry(cmd_line, 1, pid, false); // how to create a new job entry? should i use new?
        job_vector.push_back(job); // check if written well.
        max_job_id = 1;
    }
    else {
        std::string cmd_line = cmd->get_cmd_line();
        JobEntry job = JobEntry(cmd_line, max_job_id + 1, pid, false); // how to create a new job entry? should i use new?????
        job_vector.push_back(job); // check if written well.
        max_job_id++;
    }
    unfinished_size++;
}

void JobsList::removeFinishedJobs() {   //TODO: Change more!!!
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
    unfinished_size = job_vector.size();
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