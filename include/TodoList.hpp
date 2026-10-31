#pragma once
#include <vector>
#include "Task.hpp"
#include "Storage.hpp"

class TodoList{
public:
    TodoList(Storage& storage): storage(storage), nextId(1) {
        TaskList=this->storage.load();
        for (const auto& task : TaskList) {
            if (task.getId() >= nextId) {
                nextId = task.getId() + 1;
            }
        }
    }
    TodoList(TodoList& todoList)=default;
    TodoList(TodoList&& todoList)=default;
    ~TodoList()=default;
    Task* findById(int id);
    void markTaskDone(int Id);
    void addTask(const std::string& description, Task::Priority priority);
    void addQueue(const std::string& name, Task::Priority priority, const std::vector<std::string>& items);
    Task* nextInQueue(const std::string& name);
    int queueTotal(const std::string& name) const;
    void removeTask(int Id);
    void renumberQueue(const std::string& name);
    void editTaskPriority(int id, Task::Priority priority);
    std::string showAllTasks(bool showPending,bool showDone,bool filterByPriority,Task::Priority priorityFilter,bool showHidden=false,bool showArchived=false,const std::string& search="");
    int archiveDone(const std::string& before);
    int clearArchive(const std::string& before);
    std::vector<Task> pendingTasks(Task::Priority priority) const;

    int getNextId();
    // Sets description and priority with a single save
    void editTask(int id, const std::string& description, Task::Priority priority);
    const std::vector<Task>& getTasks() const;

    private:
    std::vector<Task> TaskList;
    Storage& storage;
    int nextId;
};