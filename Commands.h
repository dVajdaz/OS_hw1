#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)   //TODO: CHECK FOR SEGFAULT

class Command {
    // TODO: Add your data members
public:
    const char* cmd;

    explicit Command(const char* cmd_line);
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    const char* get_cmd_line() { return cmd; }
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char* cmd_line);
};

class ExternalCommand : public Command {
public:
    const char* timeout_cmd;
    ExternalCommand(const char *cmd_line, const char *timeout_cmd);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    std::string filename;

    bool toAppend;
    bool redirected = true;

    int monitor_out;
    int opened_file_d;

public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() = default;
    void execute() override;
    void prepare();
    void cleanup();
};



class ChpromptCommand : public BuiltInCommand {
public:
    explicit ChpromptCommand(const char* cmd_line);
    virtual  ~ChpromptCommand() = default;
    void execute() override;
};

class PathWorkDirCommand : public BuiltInCommand {
public:
    explicit PathWorkDirCommand(const char* cmd_line);

    virtual  ~PathWorkDirCommand() = default;

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    char** plastPwd;
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};
/*
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};
*/
class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        std::string command_line;
        int job_id;
        pid_t job_pid;
        bool isStopped;

        JobEntry(std::string& command_line, int job_id, pid_t job_pid, bool isStopped);
    };
    int max_job_id;
    std::vector <JobEntry> job_vector;
    int unfinished_size;

    //JobsList();
    JobsList(int max_job_id,std::vector <JobEntry> job_vector,int unfinished_size);
    ~JobsList() = default;
    void addJob(Command* cmd, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char* cmd_line);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char* cmd_line);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const char* cmd_line);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
public:
    QuitCommand(const char* cmd_line);
    virtual ~QuitCommand() {}
    void execute() override;
};
class ChmodCommand : public BuiltInCommand {
public:
    ChmodCommand(const char* cmd_line);
    virtual ~ChmodCommand() {}
    void execute() override;
};
class TimeoutCommand: public Command{
public:
    TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class AlarmList{
public:
    class AlarmEntry{
    public:
        std::string command;
        time_t created;
        time_t duration;
        time_t time_limit;
        pid_t pid;
        AlarmEntry(std::string command, time_t time_created, time_t duration, pid_t pid);
        ~AlarmEntry() = default;
    };

    std::vector<AlarmEntry> alarm_vector;

    AlarmList(std::vector <AlarmEntry> alarm_vector);
    void addAlarm(std::string command, time_t duration, pid_t pid);
    void removeFinishedAlarms();
};

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
public:
    std::string prompt;
    //pid_t pid;
    char* plastPwd;
    JobsList* jobs;
    AlarmList* alarms;
    int fgp = -1 ;

    Command *CreateCommand(const char *cmd_line, const char *string);
    SmallShell(SmallShell const&) = delete; // disable copy ctor
    void operator=(SmallShell const&) = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char *cmd_line, const char *string);
    // TODO: add extra methods as needed
};
#endif //SMASH_COMMAND_H_