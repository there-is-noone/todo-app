#pragma once
#include "Storage.hpp"
#include <string>
#include <vector>

class JsonStorage : public Storage {
private:
    std::string FileName;
    public:
    explicit JsonStorage(const std::string& name);
    std::vector<Task> load() override;
    void save(const std::vector<Task>& Tasks) override;
    const std::string& path() const;
    bool hasBackup() const;
    // Puts <file>.bak back in place; false if there is no backup. change says what was undone, if known
    bool undo(std::string& change);
    ~JsonStorage() override;
};