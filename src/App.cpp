#include "App.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <cctype>
#include "colors.hpp"

extern char** environ;

void App::run(int argc, char* argv[]) {
    if (argc<2) {
        printDefault();
        return;
    }
    std::string command=argv[1];
    if (command=="--help" || command=="-h") {
        printHelp(argc,argv);
    }
    else if (command=="--list" || command=="-l") {
        handleList(argc,argv);
    }
    else if (command=="--archive" || command=="-arch" || command=="-A") {
        handleList(argc,argv,true);
    }
    else if (command=="--add" || command=="-a") {
        handleAdd(argc,argv);
    }
    else if (command=="--remove" || command=="-r") {
        if (argc<3) {
            printDefault();
            return;
        }
        handleRemove(argc,argv);
    }
    else if (command=="--done" || command=="-d") {
        if (argc<3) {
            printDefault();
            return;
        }
        handleDone(argc,argv);
    }
    else if (command=="--edit" || command=="-e") {
        handleEdit(argc,argv);
    }
    else if (command=="--notify" || command=="-n") {
        handleNotify(argc,argv);
    }
    else if (command=="--queue" || command=="-q") {
        handleQueue(argc,argv);
    }
    else {
        printDefault();
    }

}

void App::printDefault() const {
    std::cout<< "Unknown or missing command. Use --help" << std::endl;
}
void App::printHelp(int argc,char* argv[]) const {
    if (argc<3){
        std::cout <<
        "Todo CLI - Help\n\n"
        "Usage:\n"
        "  todo [command] [options]\n\n"
        "Commands:\n"
        "  -a, --add       Add a new task\n"
        "  -l, --list      Show all tasks\n"
        "  -arch, --archive Show archived done tasks\n"
        "  -r, --remove    Remove a task\n"
        "  -d, --done      Mark task as done\n"
        "  -e, --edit      Edit the priority of a task\n"
        "  -n, --notify    Popup with HIGH priority tasks (once a day)\n"
        "  -q, --queue     Add a checklist whose steps appear one by one\n\n"
        "More help:\n"
        "  todo -h [add | a]\n"
        "  todo -h [remove | r] \n"
        "  todo -h [list | l]\n"
        "  todo -h [archive | arch]\n"
        "  todo -h [done | d]\n"
        "  todo -h [edit | e]\n"
        "  todo -h [notify | n]\n"
        "  todo -h [queue | q]\n\n";
    }
    else {
        std::string command=argv[2];
        if (command=="add" || command=="a") {
            std::cout<<
                "ADD COMMAND\n"
"Usage:\n"
"todo -a [description] [priority]\n\n"
"Description:\n"
"Adds a new task to the todo list.\n\n"
"Arguments:\n"
"description   The task description (can contain spaces)\n"
"priority      Optional: low, medium, high (default: low)\n"

"Examples:\n"
"todo -a \"Buy milk\"\n"
"todo -a \"Finish project\" high\n\n"
"Interactive mode:\n"
"If no description is provided, the program will prompt you\n"
"to enter description and priority step-by-step.\n";
        }
        else if (command == "remove" || command=="r") {
            std::cout <<
                "REMOVE COMMAND\n"
                "Usage:\n"
                "todo -r <id>\n\n"
                "Description:\n"
                "Removes a task by its ID.\n\n"
                "Arguments:\n"
                "id    The ID of the task to remove\n\n"
                "Example:\n"
                "todo -r 3\n";
        }
        else if (command == "list" || command =="l") {
            std::cout <<
               "LIST COMMAND\n"
               "Usage:\n"
               "todo -l                    Show all tasks\n"
               "todo -l --pending (-p)     Show only incomplete tasks\n"
               "todo -l --done (-d)        Show only completed tasks\n"
               "todo -l --high             Show only HIGH priority\n"
               "todo -l --medium           Show only MEDIUM priority\n"
               "todo -l --low              Show only LOW priority\n"
               "todo -l --queued (-q)      Also show queued steps not revealed yet\n"
               "todo -l --archive (-arch)  Show archived tasks instead\n\n"
               "Combine filters:\n"
               "todo -l -p --high          Show HIGH priority tasks that are TODO\n\n"
               "Examples:\n"
               "todo -l\n"
               "todo -l --pending\n"
               "todo -l --high --pending\n";       }
        else if (command == "archive" || command == "arch") {
            std::cout <<
               "ARCHIVE COMMAND\n"
               "Usage:\n"
               "todo -arch                 Show archived tasks\n"
               "todo --archive --high      Show archived HIGH priority tasks\n\n"
               "Description:\n"
               "Done tasks are moved to the archive by the daily todo -n run,\n"
               "once the day they were finished is over, so they no longer\n"
               "clutter todo -l. The same priority filters as todo -l work here.\n"
               "Marking an archived task as not done with todo -d <id> brings\n"
               "it back to the list.\n";
        }
        else if (command == "done" || command == "d") {
            std::cout <<
                "DONE COMMAND\n"
                "Usage:\n"
                "todo -d <id>\n\n"
                "Description:\n"
                "Marks a task as completed.\n\n"
                "Arguments:\n"
                "id    The ID of the task to mark as done\n\n"
                "Example:\n"
                "todo -d 2\n";
        }
        else if (command=="edit" || command=="e") {
            std::cout <<
    "EDIT COMMAND\n"
    "Usage:\n"
    "todo -e <id> <priority>\n\n"
    "Description:\n"
    "Changes the priority of an existing task.\n\n"
    "Arguments:\n"
    "id        The ID of the task to edit\n"
    "priority  New priority: low, medium, high\n\n"
    "Example:\n"
    "todo -e 3 high\n";
        }
        else if (command=="notify" || command=="n") {
            std::cout <<
    "NOTIFY COMMAND\n"
    "Usage:\n"
    "todo -n [--force]\n\n"
    "Description:\n"
    "Shows a desktop popup listing all pending HIGH priority tasks.\n"
    "Only the first call of each day shows the popup, so it is safe\n"
    "to run on every login / screen unlock.\n"
    "That first call also moves tasks finished on earlier days\n"
    "to the archive (see todo -h archive).\n\n"
    "Options:\n"
    "--force, -f   Show the popup even if it was already shown today\n\n"
    "Example:\n"
    "todo -n\n";
        }
        else if (command=="queue" || command=="q") {
            std::cout <<
    "QUEUE COMMAND\n"
    "Usage:\n"
    "todo -q <name> [priority] < steps.txt\n\n"
    "Description:\n"
    "Adds a checklist as a queue of tasks. Steps are read from stdin,\n"
    "one per line. Only the current step is shown in the list; marking\n"
    "it done with todo -d reveals the next one.\n"
    "Leading \"- \", \"- [ ] \", \"* \" and \"1. \" markers are removed.\n\n"
    "Arguments:\n"
    "name      Name of the queue (can contain spaces)\n"
    "priority  Optional: low, medium, high (default: low)\n\n"
    "Examples:\n"
    "todo -q \"Phase 0 setup\" medium < phase0.txt\n"
    "printf 'Step one\\nStep two\\n' | todo -q Demo high\n\n"
    "Interactive mode:\n"
    "When stdin is a terminal, type one step per line and finish\n"
    "with an empty line.\n";
        }
        else {
            std::cout << "Unknown help topic. Use:\n";
            std::cout << "todo -h add|remove|list|archive|done|edit|notify|queue\n";
        }
    }
    }
void App::handleAdd(int argc, char* argv[]) {
    std::string description;
    std::string prioritystring;
    Task::Priority priority = Task::Priority::LOW;
    if (argc < 3) {
        std::cout << BLUE "📝 Add New Task\n" RESET;
        std::cout << "Enter description: ";
        std::getline(std::cin, description);

        if (description.empty()) {
            std::cout << RED "❌ Description cannot be empty\n" RESET;
            return;
        }
        std::cout << "Enter priority (low/medium/high) [default: low]: ";
        std::getline(std::cin, prioritystring);
        if (!prioritystring.empty()) {
            priority = Task::parsePriority(prioritystring);
        }
    } else {
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "low" || arg == "medium" || arg == "high") {
                priority = Task::parsePriority(arg);
            } else {
                if (!description.empty()) description += " ";
                description += arg;
            }
        }
    }
    todo.addTask(description, priority);
    std::cout << GREEN "✅ Task added: " RESET << description << "\n";
}

void App::handleList(int argc,char* argv[], bool showArchived) {
    Task::Priority priorityFilter = Task::Priority::HIGH;
    bool showPending = false;
    bool showDone = false;
    bool filterByPriority = false;
    bool showHidden = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--archive" || arg == "-arch" || arg == "-A") {
            showArchived = true;
        } else if (arg == "--queued" || arg == "-q") {
            showHidden = true;
        } else if (arg == "--pending" || arg == "-p") {
            showPending = true;
        } else if (arg == "--done" || arg == "-d") {
            showDone = true;
        } else if (arg == "--high") {
            priorityFilter = Task::Priority::HIGH;
            filterByPriority = true;
        } else if (arg == "--medium") {
            priorityFilter = Task::Priority::MEDIUM;
            filterByPriority = true;
        } else if (arg == "--low") {
            priorityFilter = Task::Priority::LOW;
            filterByPriority = true;
        }
    }
    std::cout << "\n";
    std::cout << todo.showAllTasks(showPending, showDone, filterByPriority, priorityFilter, showHidden, showArchived) << std::endl;
    std::cout << "\n";
}

void App::handleDone(int argc, char* argv[]) {
    int id;
    try {
        id = std::stoi(argv[2]);
    }
    catch (const std::invalid_argument&) {
        std::cout << RED "❌ Invalid ID: not a number\n" RESET;
        return;
    }
    catch (const std::out_of_range&) {
        std::cout << RED "❌ ID out of range\n" RESET;
        return;
    }

    if (Task* task = todo.findById(id)) {
        todo.markTaskDone(id);
        if (!task->isDone()) {
            std::cout << YELLOW "↩️  Task " << id << " marked as not done\n" RESET;
            return;
        }
        std::cout << GREEN "✅ Task " << id << " marked as done!\n" RESET;
        if (task->isQueued()) {
            std::string queue = task->getQueue();
            if (Task* next = todo.nextInQueue(queue)) {
                std::cout << CYAN "➡️  Next in \"" << queue << "\": " RESET
                          << "#" << next->getId() << " " << next->getDescription() << "\n";
            } else {
                std::cout << GREEN "🏁 Queue \"" << queue << "\" complete!\n" RESET;
            }
        }
    } else {
        std::cout << RED "❌ No task with ID " << id << "\n" RESET;
    }
}

// Strips list markers like "- ", "- [ ] ", "* " or "1. " that come from pasted checklists
static std::string cleanStep(std::string line) {
    auto trim = [](std::string& str) {
        size_t start = str.find_first_not_of(" \t\r");
        size_t end = str.find_last_not_of(" \t\r");
        str = (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    };
    trim(line);
    if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) line = line.substr(2);
    if (line.rfind("[ ] ", 0) == 0 || line.rfind("[x] ", 0) == 0) line = line.substr(4);
    size_t digits = 0;
    while (digits < line.size() && std::isdigit(static_cast<unsigned char>(line[digits]))) digits++;
    if (digits > 0 && digits + 1 < line.size() && (line[digits] == '.' || line[digits] == ')')
        && line[digits + 1] == ' ') {
        line = line.substr(digits + 2);
    }
    trim(line);
    return line;
}

void App::handleQueue(int argc, char* argv[]) {
    std::string name;
    Task::Priority priority = Task::Priority::LOW;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "low" || arg == "medium" || arg == "high") {
            priority = Task::parsePriority(arg);
        } else {
            if (!name.empty()) name += " ";
            name += arg;
        }
    }

    bool interactive = isatty(STDIN_FILENO);
    if (name.empty()) {
        if (!interactive) {
            std::cout << RED "❌ Usage: todo -q <name> [priority] < steps.txt\n" RESET;
            return;
        }
        std::cout << BLUE "📝 New Queue\n" RESET << "Enter queue name: ";
        std::getline(std::cin, name);
        if (name.empty()) {
            std::cout << RED "❌ Queue name cannot be empty\n" RESET;
            return;
        }
    }
    if (todo.queueTotal(name) > 0) {
        std::cout << RED "❌ A queue named \"" << name << "\" already exists\n" RESET;
        return;
    }

    if (interactive) {
        std::cout << "Enter steps, one per line (empty line to finish):\n";
    }
    std::vector<std::string> steps;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string step = cleanStep(line);
        if (step.empty()) {
            if (interactive) break;
            continue;
        }
        steps.push_back(step);
    }

    if (steps.empty()) {
        std::cout << RED "❌ No steps given\n" RESET;
        return;
    }
    todo.addQueue(name, priority, steps);
    std::cout << GREEN "✅ Queue \"" << name << "\" added: " RESET << steps.size()
              << " steps, current: " << steps.front() << "\n";
}

void App::handleRemove(int argc, char* argv[]) {
    int id;
    try {
        id = std::stoi(argv[2]);
    }
    catch (const std::invalid_argument&) {
        std::cout << RED "❌ Invalid ID: not a number\n" RESET;
        return;
    }
    catch (const std::out_of_range&) {
        std::cout << RED "❌ ID out of range\n" RESET;
        return;
    }

    if (todo.findById(id)) {
        todo.removeTask(id);
        std::cout << GREEN "✅ Task removed\n" RESET;
    } else {
        std::cout << RED "❌ No task with ID " << id << "\n" RESET;
    }
}

void App::handleEdit(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << RED "❌ Usage: todo -e <id> <priority>\n" RESET;
        return;
    }
    int id;
    try {
        id = std::stoi(argv[2]);
    }
    catch (const std::invalid_argument&) {
        std::cout << RED "❌ Invalid ID: not a number\n" RESET;
        return;
    }
    catch (const std::out_of_range&) {
        std::cout << RED "❌ ID out of range\n" RESET;
        return;
    }

    std::string priorityStr = argv[3];
    if (priorityStr != "low" && priorityStr != "medium" && priorityStr != "high") {
        std::cout << RED "❌ Invalid priority. Use: low, medium, high\n" RESET;
        return;
    }

    if (!todo.findById(id)) {
        std::cout << RED "❌ No task with ID " << id << "\n" RESET;
        return;
    }

    Task::Priority newPriority = Task::parsePriority(priorityStr);
    todo.editTaskPriority(id, newPriority);
    std::cout << GREEN "✅ Task " << id << " priority updated to " RESET
              << YELLOW << priorityStr << RESET << "\n";
}

static std::filesystem::path notifyStampPath() {
    std::filesystem::path stateDir;
    if (const char* xdg = std::getenv("XDG_STATE_HOME"); xdg && *xdg) {
        stateDir = xdg;
    } else if (const char* home = std::getenv("HOME")) {
        stateDir = std::filesystem::path(home) / ".local/state";
    } else {
        throw std::runtime_error("Cannot determine HOME directory");
    }
    return stateDir / "todo-app" / "last-notify";
}

void App::handleNotify(int argc, char* argv[]) {
    bool force = false;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--force" || arg == "-f") force = true;
    }

    auto stampPath = notifyStampPath();
    std::string date = Task::today();
    if (!force) {
        std::ifstream stamp(stampPath);
        std::string lastDate;
        if (stamp >> lastDate && lastDate == date) {
            return;
        }
    }

    // The first run of the day also moves tasks finished on earlier days to the archive
    todo.archiveDone(date);

    std::vector<Task> tasks = todo.pendingTasks(Task::Priority::HIGH);
    if (!tasks.empty()) {
        std::string title = "🔥 " + std::to_string(tasks.size()) + " high priority task"
                            + (tasks.size() == 1 ? "" : "s") + " for today";
        std::string body;
        for (const auto& task : tasks) {
            if (!body.empty()) body += "\n";
            body += "• #" + std::to_string(task.getId()) + " " + task.getDescription();
        }

        // Spawn notify-send directly (no shell) so task text is never interpreted as a command.
        // Critical urgency keeps the popup on screen until it is dismissed.
        const char* args[] = {"notify-send", "--app-name=Todo", "--urgency=critical",
                              "--icon=view-task", title.c_str(), body.c_str(), nullptr};
        pid_t pid;
        if (posix_spawnp(&pid, "notify-send", nullptr, nullptr,
                         const_cast<char* const*>(args), environ) != 0) {
            std::cout << RED "❌ Could not run notify-send (is libnotify installed?)\n" RESET;
            return;
        }
        int status = 0;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::cout << RED "❌ notify-send failed\n" RESET;
            return;
        }
    }

    std::filesystem::create_directories(stampPath.parent_path());
    std::ofstream(stampPath) << date << "\n";
}