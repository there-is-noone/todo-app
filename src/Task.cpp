#include "Task.hpp"

Task::Task(int id, const std::string& description,Priority p, bool done,
           const std::string& queue, int step, bool hidden): priority_(p), id_(id), description_(description),
done_(done), queue_(queue), step_(step), hidden_(hidden) {}



void Task::setDone(bool is_done) {
    done_=is_done;
}
void Task::setDescription(const std::string& description) {
    description_ = description;
}
void Task::setId(int id) {
    id_ = id;
}

void Task::setPriority(Priority p) {
    priority_ = p;
}

void Task::setHidden(bool is_hidden) {
    hidden_ = is_hidden;
}

int Task::getId() const {
    return id_;
}
std::string Task::getDescription()  const{
    return description_;
}
bool Task::isDone() const {
    return done_;
}
Task::Priority Task::getPriority() const {
    return priority_;
}
std::string Task::getQueue() const {
    return queue_;
}
int Task::getStep() const {
    return step_;
}
bool Task::isHidden() const {
    return hidden_;
}
bool Task::isQueued() const {
    return !queue_.empty();
}

std::string Task::priorityToString(Priority p) {
    switch (p) {
        case Priority::HIGH: return "high";
        case Priority::MEDIUM: return "medium";
        case Priority::LOW: return "low";
    }
    return "low";
}

Task::Priority Task::parsePriority(const std::string& input) {
    if (input == "high") return Priority::HIGH;
    if (input == "medium") return Priority::MEDIUM;
    return Priority::LOW;
}