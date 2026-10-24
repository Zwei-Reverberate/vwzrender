project(example)

add_executable(${PROJECT_NAME} src/example.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE zwindow)
