#pragma once
#include "TodoList.hpp"
#include <ostream>

class App {
public:
    explicit App(TodoList& todo):todo(todo){}
    // Returns the process exit code: 0 on success, 1 if any error was printed
    int run(int argc,char* argv[]);

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
    void handleClearArchive(int argc, char* argv[]);
    // Error lines go to stderr and make the run exit with 1
    std::ostream& error();
    TodoList& todo;
    int exitCode = 0;
};