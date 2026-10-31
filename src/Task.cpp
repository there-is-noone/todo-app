#include "Task.hpp"
#include <ctime>

Task::Task(int id, const std::string& description,Priority p, bool done,
           const std::string& queue, int step, bool hidden, bool archived, const std::string& doneDate):
priority_(p), id_(id), description_(description), done_(done), queue_(queue), step_(step), hidden_(hidden),
archived_(archived), doneDate_(doneDate) {}



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

void Task::setStep(int step) {
    step_ = step;
}

void Task::setHidden(bool is_hidden) {
    hidden_ = is_hidden;
}

void Task::setArchived(bool is_archived) {
    archived_ = is_archived;
}

void Task::setDoneDate(const std::string& date) {
    doneDate_ = date;
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
bool Task::isArchived() const {
    return archived_;
}
std::string Task::getDoneDate() const {
    return doneDate_;
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

std::string Task::today() {
    std::time_t now = std::time(nullptr);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
    return buffer;
}

// The date `days` days before today, in the same format as today()
std::string Task::daysAgo(int days) {
    std::time_t now = std::time(nullptr);
    std::tm date = *std::localtime(&now);
    date.tm_mday -= days;
    date.tm_hour = 12;    // midday, so a daylight saving switch cannot move the date
    date.tm_isdst = -1;
    std::mktime(&date);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
    return buffer;
}
