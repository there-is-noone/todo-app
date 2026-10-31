
#include <vector>
#include "JsonStorage.hpp"
#include "Task.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
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

JsonStorage::JsonStorage(const std::string& name) {
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
        const char* home = std::getenv("HOME");
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
    if (std::filesystem::exists(FileName, ec)) {
        std::filesystem::copy_file(FileName, FileName + ".bak",
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }

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
