set(membership_service_client_source_files
    membership.pb.h
    membership.pb.cc
    membership.grpc.pb.h
    membership.grpc.pb.cc
    client.cpp
        ${CONFIG_FOLDER_PATH_VALUE}/test_comm_config.cpp
        ${CONFIG_FOLDER_PATH_VALUE}/config_file_parser.cpp
    ${concord_bft_tools_SOURCE_DIR}/KeyfileIOUtils.cpp
)

set(membership_service_replica_source_files
    jsonxx.h
    jsonxx.cc
    curlrequest.hpp
    couchdbxx.hpp
    replica.cpp
    ${CONFIG_FOLDER_PATH_VALUE}/test_comm_config.cpp
    ${CONFIG_FOLDER_PATH_VALUE}/config_file_parser.cpp
    ${concord_bft_tools_SOURCE_DIR}/KeyfileIOUtils.cpp
)

# This target should be moved to UT infrastructure code when created.
set(config_source_files
    ${CONFIG_FOLDER_PATH_VALUE}/config_file_parser.cpp
    ${CONFIG_FOLDER_PATH_VALUE}/config_file_parser_test.cpp
)

add_executable(membership_service_client
    ${membership_service_client_source_files}
)

add_executable(membership_service_server
    ${membership_service_replica_source_files}
)

add_executable(configTest
    ${config_source_files}
)

if(USE_LOG4CPP)
    target_compile_definitions(membership_service_server PUBLIC USE_LOG4CPP=1)
    target_compile_definitions(membership_service_client PUBLIC USE_LOG4CPP=1)
    target_compile_definitions(configTest PUBLIC USE_LOG4CPP=1)
endif()

if(${USE_COMM_PLAIN_TCP})
    target_compile_definitions(simnpleTest_client PUBLIC USE_COMM_PLAIN_TCP)
    target_compile_definitions(simnpleTest_server PUBLIC USE_COMM_PLAIN_TCP)
endif()

if(${USE_COMM_TLS_TCP})
    target_compile_definitions(simnpleTest_client PUBLIC USE_COMM_TLS_TCP)
    target_compile_definitions(simnpleTest_server PUBLIC USE_COMM_TLS_TCP)
endif()

set_target_properties(membership_service_client PROPERTIES OUTPUT_NAME client)
set_target_properties(membership_service_server PROPERTIES OUTPUT_NAME server)
set_target_properties(configTest PROPERTIES OUTPUT_NAME config)

target_link_libraries(membership_service_client PUBLIC corebft)
target_link_libraries(membership_service_client PUBLIC grpc++)
target_link_libraries(membership_service_client PUBLIC protobuf)
target_link_libraries(membership_service_client PUBLIC grpc)

target_link_libraries(membership_service_server PUBLIC corebft)
target_link_libraries(membership_service_server PUBLIC -lcurl)
target_link_libraries(configTest PUBLIC corebft)

target_include_directories(membership_service_client
                           PRIVATE
                           ${concord_bft_tools_SOURCE_DIR}
                           ${bftengine_SOURCE_DIR}/include
                           ${CONFIG_FOLDER_PATH_VALUE})

target_include_directories(membership_service_server
                           PRIVATE
                           ${concord_bft_tools_SOURCE_DIR}
                           ${bftengine_SOURCE_DIR}/include
                           ${CONFIG_FOLDER_PATH_VALUE})

target_include_directories(configTest
                           PRIVATE
                           ${concord_bft_tools_SOURCE_DIR}
                           ${bftengine_SOURCE_DIR}/include
                           ${CONFIG_FOLDER_PATH_VALUE})

add_custom_target(copy_simple_test_scripts ALL COMMENT "Copying scripts of membership_service")
add_custom_command(TARGET copy_simple_test_scripts
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:membership_service_client>/scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:membership_service_client>/scripts)

add_custom_target(copy_tls_script ALL COMMENT "Copying TLS script")
add_custom_command(TARGET copy_tls_script
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/scripts/linux/create_tls_certs.sh
     $<TARGET_FILE_DIR:membership_service_client>/scripts/create_tls_certs.sh)
