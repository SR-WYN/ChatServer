# 扁平 #include：各子目录均加入 INTERFACE，源文件写 "Foo.h" 即可

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED TRUE)

add_library(chatserver_includes INTERFACE)

target_include_directories(chatserver_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/application/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/application/impl
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/base
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/config
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/cache
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/grpc
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/log
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/mysql
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/net
    ${PROTO_GEN_DIR}
)

target_link_libraries(chatserver_includes INTERFACE
    JsonCpp::JsonCpp
    Boost::system
    Boost::thread
    Boost::filesystem
    spdlog::spdlog
)

add_subdirectory(common)
add_subdirectory(infra/base)
add_subdirectory(infra/config)
add_subdirectory(infra/cache)
add_subdirectory(infra/grpc)
add_subdirectory(infra/log)
add_subdirectory(infra/mysql)
add_subdirectory(infra/net)
add_subdirectory(application)
add_subdirectory(app)

add_executable(${PROJECT_NAME}
    app/ChatServer.cpp
    ${PROTO_SRCS}
)

# 固定可执行文件输出到 build 根目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
)

# 确保 proto 文件先生成，再编译可执行文件
add_dependencies(${PROJECT_NAME} chat_proto_gen)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROTO_GEN_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    chatserver_includes
    cs_app
    common
    cs_base
    cs_cache
    cs_config
    cs_grpc
    cs_log
    cs_mysql
    cs_net
    Boost::system
    Boost::thread
    Boost::filesystem
    JsonCpp::JsonCpp
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    mysqlcppconn
    spdlog::spdlog
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/src/logdir"
    COMMENT "Copying config.json and creating src/logdir..."
)
