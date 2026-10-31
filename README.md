# Todo CLI App

A lightweight Linux command-line todo list application in modern C++. Manage tasks by priority with persistent JSON storage.

## Features

- 🎯 Priority-based task management (HIGH/MEDIUM/LOW)
- 📋 Filter tasks by status (`--pending`, `--done`) or priority, search by text (`-s`)
- ✏️ Full CRUD operations (add, list, edit priority or description, remove, done)
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
todo -a call high school             # "call high school", LOW: only the last word counts as priority
 
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

#### Search tasks
```bash
todo -s milk              # Tasks whose description contains "milk" (case-insensitive)
todo --search buy milk    # All words after -s form the search text
todo -l -p --high -s milk # As a filter of todo -l: -s must come last
todo -arch -s report      # Search the archive
```

#### Mark task as done
```bash
todo -d 1                 # Mark task #1 as done
todo --done 3             # Mark task #3 as done
todo -d 1 4 7             # Several tasks at once
todo -d 2-5               # A range of IDs
```
Each ID gets its own result line. Unknown IDs are reported, the others are still
applied, and the exit code is 1 if any ID failed.

#### Task details
```bash
todo -i 3                 # Full record of task #3 (also --info)
```
Shows the ID, description, priority, status (TODO/DONE/QUEUED/ARCHIVED), the queue
name and step for queued tasks, and the date the task was done.

#### Edit a task
```bash
todo -e 2 high            # Change task #2 to HIGH priority
todo --edit 5 low         # Change task #5 to LOW priority
todo -e 3 "Buy oat milk"  # Rename task #3
todo -e 3 high "Buy oat milk"   # Rename and change priority
todo -e 3 Buy oat milk high     # Same: priority may be the first or the last word
```

#### Remove a task
```bash
todo -r 1                 # Delete task #1
todo --remove 3           # Delete task #3
todo -r 1 3-4             # Delete #1, #3 and #4
```
#### Checklist queues
```bash
todo -q "Phase 0 setup" medium < steps.txt     # One step per line
printf 'Create venv\nRun simulator\n' | todo -q Demo high
todo -l --queued (-q)     # Also show steps that are not revealed yet
```
As with `todo -a`, only the last argument is taken as priority.
Only the current step of a queue is shown. Marking it done with `todo -d <id>`
reveals the next one, until the queue is complete. Leading `- `, `- [ ] `, `* ` and `1. `
markers are stripped, so a pasted checklist works as-is.

#### Archive
```bash
todo -arch                # Show archived done tasks (also --archive, -A)
todo --archive --high     # Same priority filters as todo -l
todo -arch -s report      # Search the archive
```
The first `todo -n` of each day moves tasks finished on earlier days to the archive,
so `todo -l` only shows open tasks and the ones you finished today.
`todo -d <id>` on an archived task marks it not done and brings it back to the list.

#### Pending count for your shell prompt
```bash
todo -c                   # 🔥2 ⏳7  (pending HIGH, all pending), ✅0 if nothing is pending
```
One short line without colours, so it can go straight into a prompt, e.g. in `~/.bashrc`:
```bash
PS1='[$(todo -c)] \w \$ '
```

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

Tasks saved to `./.local/share/todo-app/tasks.json`:
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