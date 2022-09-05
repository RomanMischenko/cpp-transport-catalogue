# cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/home/roman/protobuf/package
# cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/roman/protobuf/package

cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH /home/roman/protobuf/package)

# Эта команда найдёт собранный нами пакет Protobuf.
# REQUIRED означает, что библиотека обязательна.
# Путь для поиска укажем в параметрах команды cmake.
find_package(Protobuf REQUIRED)
# Помимо Protobuf, понадобится библиотека Threads
find_package(Threads REQUIRED)

# Команда вызова protoc. 
# Ей переданы названия переменных, в которые будут сохранены 
# списки сгенерированных файлов, а также сам proto-файл.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto transport_router.proto)

add_executable(
    transport_catalogue
    main.cpp 
    domain.cpp domain.h
    geo.cpp geo.h
    graph.h
    json.cpp json.h 
    json_builder.cpp json_builder.h 
    json_reader.cpp json_reader.h
    map_renderer.cpp map_renderer.h map_renderer.proto
    ranges.h
    request_handler.cpp request_handler.h
    router.h
    svg.cpp svg.h
    transport_catalogue.cpp transport_catalogue.h transport_catalogue.proto
    transport_router.cpp transport_router.h transport_router.proto
    serialization.cpp serialization.h
    ${PROTO_SRCS} ${PROTO_HDRS}
)

# find_package определила переменную Protobuf_INCLUDE_DIRS,
# которую нужно использовать как include-путь.
# Также нужно добавить как include-путь директорию, куда
# protoc положит сгенерированные файлы.
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${PROTOBUF_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "${Protobuf_LIBRARY_DEBUG}" Threads::Threads)



