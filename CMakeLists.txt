cmake_minimum_required(VERSION 3.10)
project(todo)

set(CMAKE_CXX_STANDARD 17)

include_directories(include external)

file(GLOB SRC_FILES src/*.cpp)

add_executable(todo ${SRC_FILES})
install(TARGETS todo RUNTIME DESTINATION bin)
install(PROGRAMS contrib/todo-notify-watch DESTINATION bin)
install(FILES contrib/todo-notify.service DESTINATION lib/systemd/user)
