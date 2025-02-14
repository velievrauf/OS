cmake_minimum_required(VERSION 3.10)

project(BattleShip)

# Установка стандарта C++ (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Включить предупреждения
add_compile_options(-Wno-pedantic 
  -Wno-tautological-constant-compare 
  -Wno-implicit-fallthrough 
  -Wno-stringop-overflow 
  -Wno-switch
  -Wno-unused-result
  -Wno-deprecated-declarations
  -Wno-unused-variable
  -Wno-unused-but-set-variable
  -Wno-unused-parameter
)

# Установка ZeroMQ
include(FetchContent)

FetchContent_Declare(
    libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG v4.3.5
    TLS_VERIFY false
)

FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG v4.10.0
    TLS_VERIFY false
)

FetchContent_MakeAvailable(libzmq cppzmq)

# Добавить директории заголовочных файлов ZeroMQ
include_directories(${libzmq_SOURCE_DIR}/include ${cppzmq_SOURCE_DIR})

# Сервер
add_executable(server_cp server/main_server.cpp)
target_link_libraries(server_cp PRIVATE libzmq cppzmq)

# Клиент
add_executable(client_cp client/main_client.cpp)
target_link_libraries(client_cp PRIVATE libzmq cppzmq)