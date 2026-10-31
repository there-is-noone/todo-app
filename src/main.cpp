#include <iostream>
#include <ostream>
#include <string>

#include "App.hpp"
#include "JsonStorage.hpp"
#include "TodoList.hpp"
#include "colors.hpp"


// todo -u: puts the backup made before the last change back in place
static int undoLastChange(JsonStorage& storage) {
    std::string change;
    if (!storage.undo(change)) {
        std::cerr << RED "❌ Nothing to undo\n" RESET;
        return 1;
    }
    if (change.empty()) {
        std::cout << YELLOW "↩️  Undid last change\n" RESET;
    } else {
        std::cout << YELLOW "↩️  Undid: " << change << "\n" RESET;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    JsonStorage storage("/data/tasks.json");
    try {
        // Undo runs before the tasks are loaded, so it can also restore an unreadable file
        if (argc > 1 && (std::string(argv[1]) == "-u" || std::string(argv[1]) == "--undo")) {
            return undoLastChange(storage);
        }
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
