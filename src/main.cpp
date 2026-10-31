#include <iostream>
#include <ostream>

#include "App.hpp"
#include "JsonStorage.hpp"
#include "TodoList.hpp"


int main(int argc, char* argv[]) {
    JsonStorage storage("/data/tasks.json");
    TodoList todo(storage);

    App app(todo);
    return app.run(argc,argv);
}
