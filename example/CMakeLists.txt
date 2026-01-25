cmake_minimum_required(VERSION 3.5)
project(blocksci_example)

file(GLOB EXAMPLE_HEADERS "*.hpp")

add_executable(blocksci_example EXCLUDE_FROM_ALL example.cpp performance.cpp ${EXAMPLE_HEADERS})
target_link_libraries(blocksci_example blocksci)

add_executable(hash_index_test EXCLUDE_FROM_ALL hash_index_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(hash_index_test blocksci blocksci_internal)

add_executable(hash_index_create_test EXCLUDE_FROM_ALL hash_index_create_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(hash_index_create_test blocksci blocksci_internal)

add_executable(hash_index_debug EXCLUDE_FROM_ALL hash_index_debug.cpp ${EXAMPLE_HEADERS})
target_link_libraries(hash_index_debug blocksci blocksci_internal)
