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

const std::string WHITESPACE = "\t\r\f\n\v ";

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
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
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
SmallShell::SmallShell() : prompt("smash"), plastPwd(nullptr){
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
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

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}



/*----------------------------------------------------------BUILT IN COMMANDS----------------------------------------------------------*/
ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {

}
void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];   //TODO: CHECK FOR SEGFAULT
    int num_args = _parseCommandLine(this->cmd, args);

    SmallShell &shell = SmallShell::getInstance();

    if(num_args>1){
        shell.prompt = args[1];
    } else if (num_args<1){
        //TODO: ERROR HANDLING?
    }
    else {
        shell.prompt = "smash";
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
//TODO: Should switch to shell.pid??
void ShowPidCommand::execute() {
    //SmallShell &shell = SmallShell::getInstance();
    std::cout << "smash pid is " << getpid();
}

PathWorkDirCommand::PathWorkDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void PathWorkDirCommand::execute() {
    size_t maxPathLength = (size_t) pathconf(".", _PC_PATH_MAX);

    if (maxPathLength == -1) {
        perror("Error getting maximum path length");
    } else {
        char buffer[maxPathLength];
        getcwd(buffer, maxPathLength);
        cout << buffer << endl;
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd): BuiltInCommand(cmd_line), plastPwd(plastPwd) {}
void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    size_t maxPathLength = (size_t) pathconf(".", _PC_PATH_MAX);
    char buffer[maxPathLength];

    if (getcwd(buffer, maxPathLength) == NULL) {
        perror("smash error: getcwd failed");
        //free_args(args, num_of_args);
        return;
    }

    SmallShell &shell = SmallShell::getInstance();

    if(num_args > 2){
        std::cout << "smash error: cd: too many arguments";
    } else if(*args[1] == '-'){                                 //Changing to previous working dir
        if(!*this->plastPwd){
            std::cout << "smash error: cd: OLDPWD not set";
        } else {
            if(chdir(shell.plastPwd) == -1){                    //chdir SYSCALL FAILED
                perror("smash error: chdir failed");
                //free(args);
            } else {
                //if (*plastPwd)
                //  free(*plastPwd);
                shell.plastPwd = buffer;
            }
        }
    } else {                                                    //Changing to given directory
        if(chdir(args[1]) == -1){                               //chdir SYSCALL FAILED
            perror("smash error: chdir failed");
            //free(args);
            return;
        } else {
            //if (*plastPwd)
            //  free(*plastPwd);
            shell.plastPwd = buffer;
        }
    }
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
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}
void ExternalCommand::execute() {
    std::string trim_cmd = _trim(string(this->cmd));
    char bash_exec[] = "/bin/bash";
    char bash_flag[] = "-c";
    char *bash_args[] = {bash_exec, bash_flag, const_cast<char *>(trim_cmd.c_str()), nullptr};

    bool is_background = _isBackgroundComamnd(this->cmd);
    if (is_background)
        _removeBackgroundSign(this->cmd);

    pid_t pid = fork();
    if (pid == 0) {                                                  //CHILD PROCESS
        char *args[COMMAND_MAX_ARGS + 1];
        int num_args = _parseCommandLine(this->cmd, args);

        if (arrayContainsSpecialCharacter(args, num_args)) {         //COMPLEX EXTERNAL COMMAND
            if (execv(bash_exec, bash_args) == -1) {
                perror("smash error: execv failed");
                return;
            } else {                                                 //BASIC EXTERNAL COMMAND
                if (execv(args[0], args) == -1) {
                    perror("smash error: execv failed");
                    return;
                }
            }
        }
    } else {                                                         //PARENT PROCESS
        if(is_background){
            //TODO: Add background implementation
        } else {
            int status;
            if(waitpid(pid, &status, 0) == -1){
                perror("smash error: waitpid failed");
                return;
            }
        }
    }
}



/*----------------------------------------------------------REDIRECTION COMMAND----------------------------------------------------------*/
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    char *args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    const char *outputRedirect = nullptr;

    for (int i = 0; args[i] != nullptr; ++i) {
        // Check if the current string contains ">" or ">>"
        if (strstr(args[i], ">") != nullptr) {
            outputRedirect = ">";
            this->toAppend = false;

            this->file = _trim(args[i+1]);                         //TODO: Should use strcpy for the file field??
            break;
        } else if (strstr(args[i], ">>") != nullptr) {
            outputRedirect = ">>";
            this->toAppend = true;
            this->file = _trim(args[i+1]);
            break;
        }
    }

    if(outputRedirect != nullptr){
        this->prepare();
    }

    
}

void RedirectionCommand::prepare() {

}
