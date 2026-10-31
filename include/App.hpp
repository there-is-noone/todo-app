#pragma once
#include "TodoList.hpp"
#include <ostream>
#include <vector>

class App {
public:
    explicit App(TodoList& todo):todo(todo){}
    // Returns the process exit code: 0 on success, 1 if any error was printed
    int run(int argc,char* argv[]);

    private:
    void printDefault();
    void printHelp(int argc,char* argv[]);
    void handleList(int argc, char* argv[], bool showArchived=false, int first=2);
    void handleEdit(int argc, char* argv[]);
    void handleAdd(int argc, char* argv[]);
    void handleRemove(int argc, char* argv[]);
    void handleDone(int argc, char* argv[]);
    void handleNotify(int argc, char* argv[]);
    void handleQueue(int argc, char* argv[]);
    void handleInfo(int argc, char* argv[]);
    void printIds() const;
    void printCount() const;
    // Error lines go to stderr and make the run exit with 1
    std::ostream& error();
    std::vector<int> parseIds(int argc, char* argv[], int first);
    std::vector<int> idsOrPick(int argc, char* argv[], const char* flag);
    std::vector<int> pickIds();
    TodoList& todo;
    int exitCode = 0;
};