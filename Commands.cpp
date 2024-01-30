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

void free_args(char **args, int arg_num) {
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

//TODO: Should add pid field?
SmallShell::SmallShell() : prompt("smash"), plastPwd(nullptr){}

SmallShell::~SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command::Command(const char *cmd_line) : cmd(cmd_line) {}
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  //TODO: Consider & in the end of the command. Should be skipped in case the command is basic
  if (firstWord.compare("chprompt") == 0) {
      return new ChpromptCommand(cmd_line);
  }
  /*
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }*/
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
      return new PathWorkDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line, &plastPwd);
  }
  else {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line) {
  // for example:
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}



/*----------------------------------------------------------BUILT IN COMMANDS----------------------------------------------------------*/
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
}
void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS + 1];
    int num_args = _parseCommandLine(this->cmd, args);

    SmallShell &smash = SmallShell::getInstance();

    if(num_args>1){
        smash.prompt = args[1];
    } else if (num_args<1){
        //TODO: ERROR HANDLING?
    }
    else {
        smash.prompt = "smash";
    }

    free_args(args, num_args);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
//TODO: Should switch to shell.pid??
void ShowPidCommand::execute() {
    //SmallShell &smash = SmallShell::getInstance();
    std::cout << "smash pid is " << getpid()<< endl;
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

    char *buffer = (char *) malloc((size_t) pathconf(".", _PC_PATH_MAX));
    if (getcwd(buffer, (size_t) pathconf(".", _PC_PATH_MAX)) == NULL) {
        perror("smash error: getcwd failed");
        free(buffer);
        free_args(args, num_args);
        return;
    }

    if(num_args > 2){
        std::cerr << "smash error: cd: too many arguments" << endl;
    } else if (num_args == 1) {
        //TODO: ERROR HANDLING?
    }
    else if ((std::string)args[1] == "-"){   //Changing to previous working dir
        if(!(*plastPwd)){
            std::cerr << "smash error: cd: OLDPWD not set" << endl;
            free_args(args, num_args);
        } else if(chdir(*plastPwd) == -1) {
            perror("smash error: chdir failed");
            free_args(args, num_args);
        } else {
            if (*plastPwd)
                free(*plastPwd);
            *plastPwd = buffer;
        }
    } else if(chdir(args[1]) == -1) {    //Changing to given directory
        perror("smash error: chdir failed");
        free_args(args, num_args);
    } else {
        if (*plastPwd)
            free(*plastPwd);
        *plastPwd = buffer;
    }
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

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}
void ExternalCommand::execute() {
    std::string trim_cmd = _trim(string(this->cmd));
    char bash_exec[] = "/bin/bash";
    char bash_flag[] = "-c";
    char *bash_args[] = {bash_exec, bash_flag, const_cast<char *>(trim_cmd.c_str()), nullptr};

    bool is_background = _isBackgroundComamnd(this->cmd);

    /*if (is_background)
        _removeBackgroundSign(this->cmd);
    */
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
            this->filename = _trim(args[i+1]);                         //TODO: Should use strcpy for the file field??
            break;
        } else if (strstr(args[i], ">>") != nullptr) {
            outputRedirect = ">>";
            this->toAppend = true;
            this->filename = _trim(args[i+1]);
            break;
        }
    }

    if(outputRedirect != nullptr){
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

    if(this->toAppend){
        this->opened_file_d = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    } else {
        this->opened_file_d = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    }

    if(opened_file_d == -1){
        perror("smash error: open failed");
        this->redirected = false;
    }
}
void RedirectionCommand::cleanup() {
    if (dup2(this->monitor_out, 1) == -1)
        perror("smash error: dup2 failed");

    if (close(this->monitor_out) == -1)
        perror("smash error: close failed");

    if(close(this->opened_file_d) == -1)
        perror("smash error: close failed");
}
void RedirectionCommand::execute() {
    if(this->redirected){
        SmallShell &smash = SmallShell::getInstance();

        const char* redirectPos = strpbrk(cmd, ">");
        size_t length = redirectPos - cmd;
        char* command_line = (char *) malloc(sizeof(char) * (length + 1));
        strncpy(command_line, cmd, length);
        command_line[length] = '\0'; // Null-terminate the string

        smash.executeCommand(command_line);              //TODO: command_line is then built into Command object and deleted after. Requires Valgrind check!
        cleanup();
    }
    //cleanup();
}