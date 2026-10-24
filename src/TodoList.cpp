#include "TodoList.hpp"
#include "Storage.hpp"
#include "Task.hpp"
#include "colors.hpp"
#include <algorithm>


Task* TodoList::findById(int Id) {
    for (auto& task : TaskList) {
        if (task.getId() == Id) {
            return &task;
        }
    }
    return nullptr;
}

void TodoList::markTaskDone(int Id) {
    Task* task= findById(Id);
    if (!task -> isDone())
        task->setDone(true);
    else task ->setDone(false);

    // Finishing a queued step reveals the next hidden step of the same queue
    if (task->isDone() && task->isQueued()) {
        Task* next = nextInQueue(task->getQueue());
        if (next && next->isHidden()) {
            next->setHidden(false);
        }
    }

    storage.save(TaskList);
}
void TodoList::addTask(const std::string& description,Task::Priority priority){
    Task newTask(getNextId(), description,priority, false);
    TaskList.push_back(newTask);
    storage.save(TaskList);
}

void TodoList::addQueue(const std::string& name, Task::Priority priority, const std::vector<std::string>& items) {
    int step = 1;
    for (const auto& item : items) {
        TaskList.emplace_back(getNextId(), item, priority, false, name, step, step > 1);
        step++;
    }
    storage.save(TaskList);
}

Task* TodoList::nextInQueue(const std::string& name) {
    Task* next = nullptr;
    for (auto& task : TaskList) {
        if (task.getQueue() == name && !task.isDone()) {
            if (!next || task.getStep() < next->getStep()) {
                next = &task;
            }
        }
    }
    return next;
}

int TodoList::queueTotal(const std::string& name) const {
    int total = 0;
    for (const auto& task : TaskList) {
        if (task.getQueue() == name) total++;
    }
    return total;
}

void TodoList::removeTask(int Id) {
    for (auto it=TaskList.begin(); it!=TaskList.end(); ++it) {
        if (it->getId() == Id) {
            TaskList.erase(it);
            storage.save(TaskList);
            return;
        }
    }
}

std::string TodoList::showAllTasks(bool showPending, bool showDone, bool filterByPriority, Task::Priority priorityFilter, bool showHidden) {
    if (TaskList.empty()) {
        return "\n" CYAN "📋 No tasks yet!\n" RESET
               "   Add one with: ./todo -a \"task\" [priority]\n\n";
    }

    std::vector<Task> filtered;

    for (const auto& task : TaskList) {
        // Status filter: if neither --pending nor --done is set, show both
        bool statusMatch = true;
        if (showPending || showDone) {  // ← only apply filter if explicitly set
            statusMatch = (showPending && !task.isDone()) || (showDone && task.isDone());
        }

        if (!statusMatch) continue;

        // Queued steps that are not revealed yet stay hidden unless asked for
        if (task.isHidden() && !showHidden) continue;

        // Priority filter
        if (filterByPriority && task.getPriority() != priorityFilter) continue;

        filtered.push_back(task);
    }

    if (filtered.empty()) {
        return "\n--- No matching tasks ---\n\n";
    }

    // Sort by priority
    std::sort(filtered.begin(), filtered.end(),
    [](const Task& a, const Task& b) {
        if (a.getPriority() != b.getPriority())
            return a.getPriority() > b.getPriority();
        if (a.getQueue() != b.getQueue())
            return a.getQueue() < b.getQueue();
        return a.getStep() < b.getStep();
    });

    std::string result;
    result += "\n";
    int done_count = 0;

    for (const auto& task : filtered) {
        if (task.isDone()) done_count++;

        if (task.getPriority() == Task::Priority::HIGH) {
            result += RED "●" RESET;
        } else if (task.getPriority() == Task::Priority::MEDIUM) {
            result += YELLOW "●" RESET;
        } else {
            result += GRAY "●" RESET;
        }

        result += " #" + std::to_string(task.getId());
        if (task.isQueued()) {
            result += " " CYAN "[" + task.getQueue() + " " + std::to_string(task.getStep()) + "/"
                      + std::to_string(queueTotal(task.getQueue())) + "]" RESET;
        }
        result += " " + task.getDescription();
        result += " ";

        if (task.isHidden()) {
            result += GRAY "[🔒 QUEUED]" RESET;
        } else if (task.isDone()) {
            result += GREEN "[✓ DONE]" RESET;
        } else {
            result += YELLOW "[⏳ TODO]" RESET;
        }
        result += "\n";
    }

    int total = filtered.size();
    int pending = total - done_count;
    result += "\n" CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n";
    result += CYAN "Shown: " RESET + std::to_string(total) +
              CYAN " | " RESET GREEN "Done: " RESET + std::to_string(done_count) +
              CYAN " | " RESET YELLOW "Pending: " RESET + std::to_string(pending) + "\n\n";

    return result;
}

void TodoList::editTaskPriority(int id, Task::Priority priority) {
    Task* task= findById(id);
    if (!task) {
        return;
    }
    task->setPriority(priority);
    storage.save(TaskList);
}


std::vector<Task> TodoList::pendingTasks(Task::Priority priority) const {
    std::vector<Task> result;
    for (const auto& task : TaskList) {
        if (!task.isDone() && !task.isHidden() && task.getPriority() == priority) {
            result.push_back(task);
        }
    }
    return result;
}

int TodoList::getNextId() {
    return nextId++;
}