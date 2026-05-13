# 入口在 app/ChatServer.cpp，由上层 add_executable 加入
add_library(chatserver_app_dir INTERFACE)
target_link_libraries(chatserver_app_dir INTERFACE chatserver_includes)
