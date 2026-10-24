
#include <vector>
#include "JsonStorage.hpp"
#include "Task.hpp"
#include <fstream>
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

    json j;
    file >> j;

    std::vector<Task> tasks;

    for (auto& i : j) {
        tasks.emplace_back(Task(
            i["id"],
            i["description"],
            Task::parsePriority(i["priority"].get<std::string>()),
            i["done"],
            i.value("queue", std::string()),
            i.value("step", 0),
            i.value("hidden", false)
        ));
    }

    return tasks;
}

void JsonStorage::save(const std::vector<Task>& Tasks) {
    std::ofstream WriteFile(FileName);
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
        j.push_back(item);
    }
    WriteFile<< j.dump(4);
}

JsonStorage::~JsonStorage() =default;
