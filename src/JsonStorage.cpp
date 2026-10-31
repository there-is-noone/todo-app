
#include <vector>
#include "JsonStorage.hpp"
#include "Task.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include "json.hpp"
#include <iostream>
#include <unistd.h>
std::filesystem::path getExecutablePath() {
    char buffer[1024];

    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        throw std::runtime_error("Cannot get executable path");
    }

    buffer[len] = '\0';
    return std::filesystem::path(buffer);
}

using json=nlohmann::json;

// Turns a json error into a short reason like "invalid JSON at line 3, column 1"
static std::string shortReason(const json::exception& e) {
    std::string reason = e.what();
    if (reason.rfind("[json.exception.", 0) == 0) {
        size_t end = reason.find("] ");
        if (end != std::string::npos) reason = reason.substr(end + 2);
    }
    size_t at = reason.find("at line ");
    if (at != std::string::npos) {
        size_t colon = reason.find(':', at);
        return "invalid JSON " + reason.substr(at, colon == std::string::npos ? std::string::npos : colon - at);
    }
    return reason;
}

// Reads a task file; a broken file throws StorageError and is never overwritten
static std::vector<Task> readTasks(const std::string& path) {
    std::vector<Task> tasks;
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw StorageError("Could not read " + path + ": cannot open file", true);
        }
        json j;
        file >> j;
        if (!j.is_array()) {
            throw StorageError("Could not read " + path + ": expected a list of tasks", true);
        }

        for (const auto& i : j) {
            tasks.emplace_back(Task(
                i.at("id").get<int>(),
                i.at("description").get<std::string>(),
                Task::parsePriority(i.at("priority").get<std::string>()),
                i.at("done").get<bool>(),
                i.value("queue", std::string()),
                i.value("step", 0),
                i.value("hidden", false),
                i.value("archived", false),
                i.value("doneDate", std::string())
            ));
        }
    } catch (const json::exception& e) {
        throw StorageError("Could not read " + path + ": " + shortReason(e), true);
    }
    return tasks;
}

// Describes what restoring `before` over `now` takes back, e.g. "removed #4 Buy milk"; empty if unclear
static std::string describeChange(const std::vector<Task>& now, const std::vector<Task>& before) {
    auto find = [](const std::vector<Task>& tasks, int id) -> const Task* {
        for (const auto& task : tasks) {
            if (task.getId() == id) return &task;
        }
        return nullptr;
    };
    std::vector<const Task*> added, removed, done, undone, edited, archived;
    for (const auto& task : now) {
        const Task* old = find(before, task.getId());
        if (!old) {
            added.push_back(&task);
            continue;
        }
        // Step numbers and hidden flags only change as a side effect, so they are not compared
        if (task.isDone() != old->isDone()) {
            (task.isDone() ? done : undone).push_back(&task);
        } else if (task.isArchived() != old->isArchived()) {
            archived.push_back(&task);
        } else if (task.getPriority() != old->getPriority() || task.getDescription() != old->getDescription()) {
            edited.push_back(&task);
        }
    }
    for (const auto& task : before) {
        if (!find(now, task.getId())) removed.push_back(&task);
    }

    auto name = [](const Task* task) { return "#" + std::to_string(task->getId()) + " " + task->getDescription(); };
    auto count = [](size_t n) { return std::to_string(n) + " task" + (n == 1 ? "" : "s"); };
    size_t kinds = !added.empty() + !removed.empty() + !done.empty() + !undone.empty() + !edited.empty() + !archived.empty();
    if (kinds != 1) return "";

    if (!added.empty()) {
        if (added.size() == 1) return "added " + name(added.front());
        const std::string& queue = added.front()->getQueue();
        bool sameQueue = !queue.empty() && std::all_of(added.begin(), added.end(),
                             [&](const Task* task) { return task->getQueue() == queue; });
        if (sameQueue) return "added queue \"" + queue + "\" (" + std::to_string(added.size()) + " steps)";
        return "added " + count(added.size());
    }
    if (!removed.empty()) return "removed " + (removed.size() == 1 ? name(removed.front()) : count(removed.size()));
    if (!done.empty()) return "marked " + (done.size() == 1 ? name(done.front()) : count(done.size())) + " as done";
    if (!undone.empty()) return "marked " + (undone.size() == 1 ? name(undone.front()) : count(undone.size())) + " as not done";
    if (!archived.empty()) return "archived " + count(archived.size());
    if (edited.size() == 1) {
        const Task* old = find(before, edited.front()->getId());
        return "edited #" + std::to_string(old->getId()) + " " + old->getDescription();
    }
    return "edited " + count(edited.size());
}

JsonStorage::JsonStorage(const std::string& name) {
    // TODO_FILE picks the task file directly and skips the lookup below
    if (const char* custom = std::getenv("TODO_FILE"); custom && *custom) {
        std::filesystem::path file(custom);
        std::error_code ec;
        if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path(), ec);
        FileName = file.string();
        return;
    }

    // An existing list in the home directory wins, so a freshly built
    // binary keeps using it instead of starting an empty data/tasks.json
    const char* home = std::getenv("HOME");
    if (home && *home) {
        auto homeFile = std::filesystem::path(home) / ".local/share/todo-app/tasks.json";
        std::error_code ec;
        if (std::filesystem::exists(homeFile, ec) ||
            std::filesystem::exists(homeFile.string() + ".bak", ec)) {
            FileName = homeFile.string();
            return;
        }
    }

    std::filesystem::path baseDir;

    try {
        auto exePath = getExecutablePath();
        auto exeDir = exePath.parent_path();
        auto dataDir = exeDir / "data";

        std::filesystem::create_directories(dataDir);

        auto testFile = dataDir / "test.tmp";
        std::ofstream test(testFile);
        if (test) {
            test.close();
            std::filesystem::remove(testFile);

            baseDir = dataDir;
        } else {
            throw std::runtime_error("Cannot write to exe directory");
        }

    } catch (...) {
        if (!home) {
            throw std::runtime_error("Cannot determine HOME directory");
        }

        baseDir = std::filesystem::path(home) / ".local/share/todo-app";
        std::filesystem::create_directories(baseDir);
    }

    FileName = (baseDir / "tasks.json").string();
}

std::vector<Task> JsonStorage::load() {
    std::ifstream file(FileName);

    // 🔥 If file doesn't exist → create it
    if (!file.is_open()) {
        std::error_code ec;
        if (std::filesystem::exists(FileName, ec)) {
            throw StorageError("Could not read " + FileName + ": cannot open file", true);
        }
        std::ofstream create(FileName);
        create << "[]";
        create.close();
        return {};
    }

    // 🔥 Check if file is empty
    if (file.peek() == std::ifstream::traits_type::eof()) {
        file.close();
        std::ofstream fix(FileName);
        fix << "[]";
        fix.close();
        return {};
    }

    file.close();
    return readTasks(FileName);
}

const std::string& JsonStorage::path() const {
    return FileName;
}

bool JsonStorage::hasBackup() const {
    std::error_code ec;
    return std::filesystem::exists(FileName + ".bak", ec);
}

bool JsonStorage::undo(std::string& change) {
    std::string backupName = FileName + ".bak";
    if (!hasBackup()) return false;

    // A broken backup would replace good data with garbage, so it is never restored
    std::vector<Task> before;
    try {
        before = readTasks(backupName);
    } catch (const StorageError& e) {
        throw StorageError(std::string(e.what()) + " (nothing was restored)");
    }
    change.clear();
    try {
        change = describeChange(readTasks(FileName), before);
    } catch (const StorageError&) {
        // The current file is unreadable; restoring the backup is exactly what is wanted
    }

    // Renaming uses the backup up, so there is only one level of undo
    std::error_code ec;
    std::filesystem::rename(backupName, FileName, ec);
    if (ec) {
        throw StorageError("Could not restore " + backupName + ": " + ec.message());
    }
    return true;
}

void JsonStorage::save(const std::vector<Task>& Tasks) {
    json j=json::array();
    for (const Task& Task : Tasks) {
        json item={
            {"id",Task.getId()},
            {"description", Task.getDescription()},
                {"priority", Task::priorityToString(Task.getPriority())},
            {"done",Task.isDone()}
        };
        // Queue fields are only written for queued tasks, so plain tasks keep the old format
        if (Task.isQueued()) {
            item["queue"]=Task.getQueue();
            item["step"]=Task.getStep();
            item["hidden"]=Task.isHidden();
        }
        if (Task.isDone() && !Task.getDoneDate().empty()) item["doneDate"]=Task.getDoneDate();
        if (Task.isArchived()) item["archived"]=true;
        j.push_back(item);
    }
    std::string content = j.dump(4);

    // Saving the same data again keeps the file and its backup untouched
    {
        std::ifstream current(FileName, std::ios::binary);
        if (current.is_open()) {
            std::stringstream old;
            old << current.rdbuf();
            if (old.str() == content) return;
        }
    }

    // One level of backup, restored by todo -u
    std::error_code ec;
    if (!backedUp && std::filesystem::exists(FileName, ec)) {
        std::filesystem::copy_file(FileName, FileName + ".bak",
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }
    backedUp = true;

    // Write a temp file and rename it over the real one, so a crash never leaves half a file
    std::string tmpName = FileName + ".tmp";
    {
        std::ofstream WriteFile(tmpName, std::ios::trunc);
        WriteFile << content;
        WriteFile.close();
        if (!WriteFile) {
            std::filesystem::remove(tmpName, ec);
            throw StorageError("Could not save " + FileName + ": cannot write " + tmpName);
        }
    }
    std::filesystem::rename(tmpName, FileName, ec);
    if (ec) {
        std::filesystem::remove(tmpName, ec);
        throw StorageError("Could not save " + FileName + ": " + ec.message());
    }
}

JsonStorage::~JsonStorage() =default;
