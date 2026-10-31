#include <cstdlib>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <string>
#include <unistd.h>

#include "App.hpp"
#include "JsonStorage.hpp"
#include "TodoList.hpp"
#include "colors.hpp"

// Passes text on to another stream buffer but drops colour codes (ESC [ ... m)
class NoColorBuf : public std::streambuf {
public:
    explicit NoColorBuf(std::streambuf* target) : target(target) {}

protected:
    int overflow(int c) override {
        if (traits_type::eq_int_type(c, traits_type::eof())) return traits_type::not_eof(c);
        char ch = traits_type::to_char_type(c);
        if (state == Escape) {
            state = Text;
            if (ch == '[') {
                state = Sequence;
                return c;
            }
            // A lone ESC is not a colour code, so it is kept
            if (traits_type::eq_int_type(target->sputc('\033'), traits_type::eof())) return traits_type::eof();
        } else if (state == Sequence) {
            // Parameters and intermediates are skipped up to the final letter, which ends the code
            if (ch >= 0x20 && ch <= 0x3F) return c;
            state = Text;
            if (ch >= 0x40 && ch <= 0x7E) return c;
        } else if (ch == '\033') {
            state = Escape;
            return c;
        }
        return target->sputc(ch);
    }

    int sync() override {
        return target->pubsync();
    }

private:
    enum State { Text, Escape, Sequence };
    std::streambuf* target;
    State state = Text;
};

// Strips the colours from a stream while it lives; restores the stream afterwards
class ColorFilter {
public:
    ColorFilter(std::ostream& stream, bool strip) : stream(stream), filter(stream.rdbuf()) {
        if (strip) original = stream.rdbuf(&filter);
    }
    ~ColorFilter() {
        if (original) {
            stream.flush();
            stream.rdbuf(original);
        }
    }
    ColorFilter(const ColorFilter&) = delete;
    ColorFilter& operator=(const ColorFilter&) = delete;

private:
    std::ostream& stream;
    NoColorBuf filter;
    std::streambuf* original = nullptr;
};

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
    // No colour codes when the output goes to a file or pipe, or when NO_COLOR is set
    const char* noColorEnv = std::getenv("NO_COLOR");
    bool noColor = noColorEnv && *noColorEnv;
    ColorFilter plainOut(std::cout, noColor || !isatty(STDOUT_FILENO));
    ColorFilter plainErr(std::cerr, noColor || !isatty(STDERR_FILENO));

    JsonStorage storage("/data/tasks.json");
    try {
        // Undo runs before the tasks are loaded, so it can also restore an unreadable file
        if (argc > 1 && (std::string(argv[1]) == "-u" || std::string(argv[1]) == "--undo")) {
            return undoLastChange(storage);
        }
        TodoList todo(storage);

        App app(todo);
        return app.run(argc,argv);
    } catch (const StorageError& e) {
        // The task file could not be read or written; it is left as it was
        std::cerr << RED "❌ " << e.what() << "\n" RESET;
        if (e.unreadable && storage.hasBackup()) {
            std::cerr << "   A backup exists: todo -u restores it\n";
        }
        return 1;
    }
}
