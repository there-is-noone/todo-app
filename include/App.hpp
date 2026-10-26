#pragma once
#include "TodoList.hpp"

class App {
public:
    explicit App(TodoList& todo):todo(todo){}
    void run(int argc,char* argv[]);

    private:
    void printDefault() const;
    void printHelp(int argc,char* argv[]) const;
    void handleList(int argc, char* argv[], bool showArchived=false);
    void handleEdit(int argc, char* argv[]);
    void handleAdd(int argc, char* argv[]);
    void handleRemove(int argc, char* argv[]);
    void handleDone(int argc, char* argv[]);
    void handleNotify(int argc, char* argv[]);
    void handleQueue(int argc, char* argv[]);
    TodoList& todo;
};