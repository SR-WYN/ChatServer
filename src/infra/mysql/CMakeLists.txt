add_library(cs_mysql STATIC
    MySqlPool.h
    MySqlPool.cpp
    DbSession.h
    MySqlMgrImpl.h
    MySqlMgrImpl.cpp
)

target_link_libraries(cs_mysql PUBLIC
    chatserver_includes
    mysqlcppconn
)
