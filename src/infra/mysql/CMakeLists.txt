add_library(cs_mysql STATIC MySqlPool.cpp MySqlDao.cpp MySqlMgr.cpp)
target_link_libraries(cs_mysql PUBLIC chatserver_includes cs_config cs_common mysqlcppconn)
