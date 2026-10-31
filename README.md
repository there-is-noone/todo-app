# Todo CLI App

A lightweight Linux command-line todo list application in modern C++. Manage tasks by priority with persistent JSON storage.

## Features

- 🎯 Priority-based task management (HIGH/MEDIUM/LOW)
- 📋 Filter tasks by status (`--pending`, `--done`) or priority
- ✏️ Full CRUD operations (add, list, edit, remove, done)
- 💾 Persistent JSON storage
- 🎨 Color-coded output with clean formatting

## Quick Start

### Build

```bash
git clone https://github.com/yourusername/todo-app.git
cd todo-app
mkdir -p cmake-build-debug && cd cmake-build-debug
cmake .. && sudo make install
```

Now run from anywhere:
```bash
todo -l
todo -a "Buy milk" high
todo -h
```


## Usage Guide

### Basic Commands

#### Add a task
```bash
todo -a "Buy groceries"              # Default: LOW priority
todo -a "Finish project" high        # HIGH priority
todo -a "Read a book" medium         # MEDIUM priority
 
# Interactive mode (no arguments)
todo -a
# Prompts:
# Enter description: Study for exam
# Enter priority (low/medium/high): medium
```

#### List tasks
```bash
todo -l                   # Show all tasks
todo -l --pending (-p)    # Show only incomplete tasks
todo -l --done (-d)       # Show only completed tasks
todo -l --high            # Show only HIGH priority tasks
todo -l --medium          # Show only MEDIUM priority tasks
todo -l --low             # Show only LOW priority tasks
 
# Combine filters
todo -l --high --pending  # HIGH priority tasks that are TODO
todo -l --medium --done   # MEDIUM priority tasks that are DONE
```

#### Mark task as done
```bash
todo -d 1                 # Mark task #1 as done
todo --done 3             # Mark task #3 as done
```

#### Edit task priority
```bash
todo -e 2 high            # Change task #2 to HIGH priority
todo --edit 5 low         # Change task #5 to LOW priority
```

#### Remove a task
```bash
todo -r 1                 # Delete task #1
todo --remove 3           # Delete task #3
```
#### Checklist queues
```bash
todo -q "Phase 0 setup" medium < steps.txt     # One step per line
printf 'Create venv\nRun simulator\n' | todo -q Demo high
todo -l --queued (-q)     # Also show steps that are not revealed yet
```
Only the current step of a queue is shown. Marking it done with `todo -d <id>`
reveals the next one, until the queue is complete. Leading `- `, `- [ ] `, `* ` and `1. `
markers are stripped, so a pasted checklist works as-is.

#### Archive
```bash
todo -arch                # Show archived done tasks (also --archive, -A)
todo --archive --high     # Same priority filters as todo -l
todo -arch --clear 30     # Delete archived tasks finished more than 30 days ago
```
The first `todo -n` of each day moves tasks finished on earlier days to the archive,
so `todo -l` only shows open tasks and the ones you finished today.
`todo -d <id>` on an archived task marks it not done and brings it back to the list.
`--clear <days>` deletes old archived tasks for good (ones without a date count as old);
`todo -u` right after it brings them back.

#### Undo the last change
```bash
todo -r 4
todo -u                   # ↩️  Undid: removed #4 Buy milk (also --undo)
```
Before every change the task file is copied to `tasks.json.bak`; `todo -u` puts that copy back.
There is one level of undo, so a second `todo -u` right after says `❌ Nothing to undo`.
If `tasks.json` ever becomes unreadable, `todo -u` restores the backup as well.

#### Daily popup with HIGH priority tasks
```bash
todo -n                   # Desktop popup listing pending HIGH priority tasks
todo --notify --force     # Show it even if it was already shown today
```
`todo -n` only shows the popup the first time it runs each day (the date is kept in
`~/.local/state/todo-app/last-notify`).

To get it automatically the first time you log in / unlock / open the laptop each day,
enable the bundled user service (installed by `sudo make install`):
```bash
systemctl --user daemon-reload
systemctl --user enable --now todo-notify.service
```
It runs `todo -n` on login, screen unlock and resume from suspend (GNOME, needs `notify-send` and `gdbus`).

---
## Data Storage

Tasks are saved as plain JSON. The file is chosen in this order:
1. `$TODO_FILE`, if set (its directory is created when missing)
2. `data/tasks.json` next to the `todo` binary, if that directory is writable
3. `~/.local/share/todo-app/tasks.json`

```bash
TODO_FILE=~/work/tasks.json todo -l      # a separate list for work
export TODO_FILE=~/Sync/todo/tasks.json  # keep the list in a synced folder
```
Every save writes a temp file and renames it over `tasks.json`, and the previous
version is kept as `tasks.json.bak` (see `todo -u`).

Example `tasks.json`:
```json
[
  {"id": 1, "description": "Study", "priority": "high", "done": false},
  {"id": 2, "description": "Coffee", "priority": "low", "done": true}
]
```

## Architecture

**Classes:**
- `Task` - Individual todo item (ID, description, priority, done status)
- `TodoList` - Collection management, filtering, sorting
- `App` - CLI interface and command parsing
- `Storage`/`JsonStorage` - Persistence abstraction

**Patterns:**
- RAII for memory management
- Strategy pattern for storage abstraction
- Move semantics (C++17)
- Const-correctness throughout

## Requirements

- C++17+
- CMake 3.10+ (or Makefile alternative)

## Build Without CMake

Create `Makefile`:
```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -Iexternal

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

TARGET = todo

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

.PHONY: all clean install
```

Then:
```bash
make && sudo make install
```

## Example Session

```bash
$ todo -a "Study for exam" high
✅ Task added: Study for exam

$ todo -a "Buy groceries" low
✅ Task added: Buy groceries

$ todo -l
● #1 Study for exam [⏳ TODO]
● #2 Buy groceries [⏳ TODO]

Shown: 2 | Done: 0 | Pending: 2

$ todo -d 1
✅ Task 1 marked as done!

$ todo -l --pending
● #2 Buy groceries [⏳ TODO]

Shown: 1 | Done: 0 | Pending: 1
```

## Project Architecture
### File Structure
```
todo-app/
├── src/
│   ├── main.cpp          # Application entry point
│   ├── App.cpp           # Command handling logic
│   ├── TodoList.cpp      # Task list management
│   ├── Task.cpp          # Individual task operations
│   ├── JsonStorage.cpp   # JSON persistence
│   └── Storage.cpp       # Storage interface
├── include/
│   ├── App.hpp
│   ├── TodoList.hpp
│   ├── Task.hpp
│   ├── JsonStorage.hpp
│   └── Storage.hpp
├── external/
│   └── json.hpp          # nlohmann/json library
├── CMakeLists.txt
└── README.md
```
### Technical Highlights

- Modern C++ (C++17) with move semantics
- RAII for automatic resource management
- Minimal dependencies (single header json library)
- Polymorphic storage abstraction
- Error handling with user-friendly messages
- UTF-8 support for international text

---
## Getting Help

- **First time using the app?** Run `todo --help` for interactive guide
- **Forgot a command?** Try `todo -h add` (or any command name)
- **Issues?** Check that the `~/.local/share/todo-app/` directory exists and is writable
- **Lost data?** Check `~/.local/share/todo-app/tasks.json` - it's just plain JSON!



## University Assignment

- Presented to 30 people
- Started as a personal project, available for everybody to utilize and enjoy

---

**Ready to use. A clean CLI todo app.** 🚀

Last updated: 13.06.2026