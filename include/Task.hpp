#pragma once

#include <string>

class Task {
public:
    enum class Priority { LOW, MEDIUM, HIGH };
    Priority priority_;
    static Priority parsePriority(const std::string& input);
    static std::string priorityToString(Priority p);
    static std::string today();
    static std::string daysAgo(int days);

    Task(int id, const std::string& description, Priority p, bool done=false,
         const std::string& queue="", int step=0, bool hidden=false,
         bool archived=false, const std::string& doneDate="");

    int getId() const;
    std::string getDescription() const;
    bool isDone() const;
    Priority getPriority() const;
    std::string getQueue() const;
    int getStep() const;
    bool isHidden() const;
    bool isQueued() const;
    bool isArchived() const;
    std::string getDoneDate() const;

    void setDone(bool is_done=true);
    void setDescription(const std::string& description);
    void setId(int id);
    void setPriority(Priority p);
    void setStep(int step);
    void setHidden(bool is_hidden=true);
    void setArchived(bool is_archived=true);
    void setDoneDate(const std::string& date);

    Task(const Task& other)=default;
    Task(Task&& other) noexcept =default;
    Task& operator=(const Task& other) =default;
    Task& operator=(Task&& other) noexcept = default;
    ~Task()=default;

private:
    int id_;
    std::string description_;
    bool done_;
    std::string queue_;
    int step_;
    bool hidden_;
    bool archived_;
    std::string doneDate_;
};