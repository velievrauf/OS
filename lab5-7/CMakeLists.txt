# Указываем имя проекта
project(ZMQProject)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Параметры компиляции
include_directories(/opt/homebrew/include)
include_directories(${CMAKE_SOURCE_DIR})  # Добавляем текущую директорию для заголовков
link_directories(/opt/homebrew/lib)

# Основные цели: клиент и воркер
add_executable(client client.cpp)
add_executable(worker worker.cpp)

# Линковка ZeroMQ
target_link_libraries(client zmq)
target_link_libraries(worker zmq)

# Подключаем Google Test
find_package(GTest REQUIRED)    # Найти установленный Google Test
find_package(Threads REQUIRED) # Подключаем поддержку потоков
include_directories(${GTEST_INCLUDE_DIRS})

# Создаём цель для тестов
add_executable(tests
    tests/test_main.cpp        # Файл с тестами
    node.h                     # Заголовочные файлы
    net_func.h
)

# Линкуем тесты с Google Test, ZeroMQ и потоками
target_link_libraries(tests PRIVATE ${GTEST_LIBRARIES} zmq Threads::Threads)

# Включаем тестирование
enable_testing()
add_test(NAME ZMQTests COMMAND tests)

# Команда для очистки
add_custom_target(clean COMMAND ${CMAKE_COMMAND} -E remove -f client worker tests)
