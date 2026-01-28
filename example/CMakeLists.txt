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

add_executable(tx_list_test EXCLUDE_FROM_ALL tx_list_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(tx_list_test blocksci blocksci_internal)

add_executable(address_debug EXCLUDE_FROM_ALL address_debug.cpp ${EXAMPLE_HEADERS})
target_link_libraries(address_debug blocksci blocksci_internal)

add_executable(address_index_debug EXCLUDE_FROM_ALL address_index_debug.cpp ${EXAMPLE_HEADERS})
target_link_libraries(address_index_debug blocksci blocksci_internal)

add_executable(equiv_tx_test EXCLUDE_FROM_ALL equiv_tx_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(equiv_tx_test blocksci blocksci_internal)

add_executable(hash_index_verify EXCLUDE_FROM_ALL hash_index_verify.cpp ${EXAMPLE_HEADERS})
target_link_libraries(hash_index_verify blocksci blocksci_internal)

add_executable(tx_lookup_test EXCLUDE_FROM_ALL tx_lookup_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(tx_lookup_test blocksci blocksci_internal)

add_executable(block_tx_check EXCLUDE_FROM_ALL block_tx_check.cpp ${EXAMPLE_HEADERS})
target_link_libraries(block_tx_check blocksci blocksci_internal)

add_executable(block_compare EXCLUDE_FROM_ALL block_compare.cpp ${EXAMPLE_HEADERS})
target_link_libraries(block_compare blocksci blocksci_internal)

add_executable(data_integrity_check EXCLUDE_FROM_ALL data_integrity_check.cpp ${EXAMPLE_HEADERS})
target_link_libraries(data_integrity_check blocksci blocksci_internal)

add_executable(block_tx_verify EXCLUDE_FROM_ALL block_tx_verify.cpp ${EXAMPLE_HEADERS})
target_link_libraries(block_tx_verify blocksci blocksci_internal)

add_executable(block_hash_verify EXCLUDE_FROM_ALL block_hash_verify.cpp ${EXAMPLE_HEADERS})
target_link_libraries(block_hash_verify blocksci blocksci_internal)

add_executable(rawblock_check EXCLUDE_FROM_ALL rawblock_check.cpp ${EXAMPLE_HEADERS})
target_link_libraries(rawblock_check blocksci blocksci_internal)

add_executable(quick_tx_check EXCLUDE_FROM_ALL quick_tx_check.cpp ${EXAMPLE_HEADERS})
target_link_libraries(quick_tx_check blocksci blocksci_internal)

add_executable(find_corruption EXCLUDE_FROM_ALL find_corruption.cpp ${EXAMPLE_HEADERS})
target_link_libraries(find_corruption blocksci blocksci_internal)

add_executable(narrow_corruption EXCLUDE_FROM_ALL narrow_corruption.cpp ${EXAMPLE_HEADERS})
target_link_libraries(narrow_corruption blocksci blocksci_internal)

add_executable(address_cluster_analysis EXCLUDE_FROM_ALL address_cluster_analysis.cpp ${EXAMPLE_HEADERS})
target_link_libraries(address_cluster_analysis blocksci blocksci_internal)

add_executable(address_connection EXCLUDE_FROM_ALL address_connection.cpp ${EXAMPLE_HEADERS})
target_link_libraries(address_connection blocksci blocksci_internal)

add_executable(tx_analysis EXCLUDE_FROM_ALL tx_analysis.cpp ${EXAMPLE_HEADERS})
target_link_libraries(tx_analysis blocksci blocksci_internal)

add_executable(tx_analysis_fast EXCLUDE_FROM_ALL tx_analysis_fast.cpp ${EXAMPLE_HEADERS})
target_link_libraries(tx_analysis_fast blocksci blocksci_internal)

add_executable(check_script_type EXCLUDE_FROM_ALL check_script_type.cpp ${EXAMPLE_HEADERS})
target_link_libraries(check_script_type blocksci blocksci_internal)

add_executable(extract_cluster_addresses EXCLUDE_FROM_ALL extract_cluster_addresses.cpp ${EXAMPLE_HEADERS})
target_link_libraries(extract_cluster_addresses blocksci blocksci_internal)

add_executable(classify_wallet_roles EXCLUDE_FROM_ALL classify_wallet_roles.cpp ${EXAMPLE_HEADERS})
target_link_libraries(classify_wallet_roles blocksci blocksci_internal)

add_executable(trace_fund_flow EXCLUDE_FROM_ALL trace_fund_flow.cpp ${EXAMPLE_HEADERS})
target_link_libraries(trace_fund_flow blocksci blocksci_internal)

add_executable(trace_fund_flow_fast EXCLUDE_FROM_ALL trace_fund_flow_fast.cpp ${EXAMPLE_HEADERS})
target_link_libraries(trace_fund_flow_fast blocksci blocksci_internal)

add_executable(taproot_test EXCLUDE_FROM_ALL taproot_test.cpp ${EXAMPLE_HEADERS})
target_link_libraries(taproot_test blocksci blocksci_internal)

add_executable(taproot_verify EXCLUDE_FROM_ALL taproot_verify.cpp ${EXAMPLE_HEADERS})
target_link_libraries(taproot_verify blocksci blocksci_internal)

add_executable(taproot_tx_list EXCLUDE_FROM_ALL taproot_tx_list.cpp ${EXAMPLE_HEADERS})
target_link_libraries(taproot_tx_list blocksci blocksci_internal)

add_executable(check_specific_tx EXCLUDE_FROM_ALL check_specific_tx.cpp ${EXAMPLE_HEADERS})
target_link_libraries(check_specific_tx blocksci blocksci_internal)

add_executable(find_all_scriptnums EXCLUDE_FROM_ALL find_all_scriptnums.cpp ${EXAMPLE_HEADERS})
target_link_libraries(find_all_scriptnums blocksci blocksci_internal)

add_executable(build_taproot_index EXCLUDE_FROM_ALL build_taproot_index.cpp ${EXAMPLE_HEADERS})
target_link_libraries(build_taproot_index blocksci blocksci_internal)

add_executable(taproot_lookup EXCLUDE_FROM_ALL taproot_lookup.cpp ${EXAMPLE_HEADERS})
target_link_libraries(taproot_lookup blocksci blocksci_internal)

add_executable(build_taproot_rocksdb EXCLUDE_FROM_ALL build_taproot_rocksdb.cpp ${EXAMPLE_HEADERS})
target_link_libraries(build_taproot_rocksdb blocksci blocksci_internal rocksdb)

add_executable(taproot_lookup_fast EXCLUDE_FROM_ALL taproot_lookup_fast.cpp ${EXAMPLE_HEADERS})
target_link_libraries(taproot_lookup_fast blocksci blocksci_internal rocksdb)
