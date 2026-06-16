# 扁平 #include：各子目录均加入 INTERFACE，源文件写 "Foo.h" 即可

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED TRUE)

add_library(chatserver_includes INTERFACE)

target_include_directories(chatserver_includes INTERFACE

    ${CMAKE_CURRENT_SOURCE_DIR}/common

    ${CMAKE_CURRENT_SOURCE_DIR}/domain

    ${CMAKE_CURRENT_SOURCE_DIR}/domain/contract

    ${CMAKE_CURRENT_SOURCE_DIR}/net

    ${CMAKE_CURRENT_SOURCE_DIR}/application

    ${CMAKE_CURRENT_SOURCE_DIR}/application/mysql

    ${CMAKE_CURRENT_SOURCE_DIR}/application/redis

    ${CMAKE_CURRENT_SOURCE_DIR}/application/login

    ${CMAKE_CURRENT_SOURCE_DIR}/application/user

    ${CMAKE_CURRENT_SOURCE_DIR}/application/friend

    ${CMAKE_CURRENT_SOURCE_DIR}/application/chat

    ${CMAKE_CURRENT_SOURCE_DIR}/infra/config

    ${CMAKE_CURRENT_SOURCE_DIR}/infra/cache

    ${CMAKE_CURRENT_SOURCE_DIR}/infra/grpc

    ${PROTO_GEN_DIR}

)

add_subdirectory(common)

add_subdirectory(domain)

add_subdirectory(infra/config)

add_subdirectory(infra/grpc)

add_subdirectory(net)

add_subdirectory(application)

add_subdirectory(app)

add_executable(${PROJECT_NAME}

    app/ChatServer.cpp

    ${PROTO_SRCS}

)

set_target_properties(${PROJECT_NAME} PROPERTIES

    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"

)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROTO_GEN_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE

    chatserver_includes

    chatserver_app_dir

    cs_net

    cs_application

    cs_common

    cs_config

    cs_grpc

    Boost::system

    Boost::thread

    Boost::filesystem

    JsonCpp::JsonCpp

    gRPC::grpc++

    gRPC::grpc++_reflection

    protobuf::libprotobuf

    redis++

    hiredis

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
