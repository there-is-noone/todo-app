#include <iostream>
#include <ostream>

#include "App.hpp"
#include "JsonStorage.hpp"
#include "TodoList.hpp"
#include "colors.hpp"


int main(int argc, char* argv[]) {
    JsonStorage storage("/data/tasks.json");
    try {
        TodoList todo(storage);

        App app(todo);
        app.run(argc,argv);
        return 0;
    } catch (const StorageError& e) {
        // The task file could not be read or written; it is left as it was
        std::cerr << RED "❌ " << e.what() << "\n" RESET;
        if (e.unreadable && storage.hasBackup()) {
            std::cerr << "   A backup exists: todo -u restores it\n";
        }
        return 1;
    }
}
