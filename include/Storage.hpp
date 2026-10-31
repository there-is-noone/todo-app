#pragma once

#include <stdexcept>
#include <vector>
#include "Task.hpp"

// Thrown when the task file cannot be read or written; what() is the message shown to the user
class StorageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};


class Storage {
public:
    virtual std::vector<Task> load()=0;
    virtual void save(const std::vector<Task>& Tasks)=0;

    virtual ~Storage()=default;
};
