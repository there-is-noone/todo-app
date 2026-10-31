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
    ~JsonStorage() override;
};