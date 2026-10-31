#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include "Task.hpp"

// Thrown when the task file cannot be read or written; what() is the message shown to the user
class StorageError : public std::runtime_error {
public:
    explicit StorageError(const std::string& message, bool unreadable=false)
        : std::runtime_error(message), unreadable(unreadable) {}
    // True when the file exists but could not be parsed
    bool unreadable;
};


class Storage {
public:
    virtual std::vector<Task> load()=0;
    virtual void save(const std::vector<Task>& Tasks)=0;

    virtual ~Storage()=default;
};
