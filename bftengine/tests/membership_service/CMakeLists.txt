
############
#######

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
else()
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

## GRPC 
include_directories(/home/stephenlee/concord-installs/grpc)
message(STATUS "Using gRPC via add_subdirectory.")
  
# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
    
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
set(_PROTOBUF_PROTOC  $<TARGET_FILE:protoc>)

set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
#  set(_GRPC_CPP_PLUGIN_EXECUTABLE /usr/local/bin/grpc_cpp_plugin)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

###########################

get_filename_component(mem_proto "scripts/go_client/membership/membership.proto" ABSOLUTE)
get_filename_component(mem_proto_path "${mem_proto}" PATH)

# Generated sources
set(mem_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/membership.pb.cc")
set(mem_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/membership.pb.h")
set(mem_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/membership.grpc.pb.cc")
set(mem_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/membership.grpc.pb.h")

add_custom_command(
      OUTPUT "${mem_proto_srcs}" "${mem_proto_hdrs}" "${mem_grpc_srcs}" "${mem_grpc_hdrs}"
      COMMAND protoc
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${mem_proto_path}"
        --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin
        "${mem_proto}"
      DEPENDS "${mem_proto}")

####
### Creates the proto files
##################################

set(membership_service_client_source_files
    ${mem_proto_srcs}
    ${mem_grpc_srcs}
    #membership.pb.h
    #membership.pb.cc
    #membership.grpc.pb.h
    #membership.grpc.pb.cc
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
    #${mem_proto_srcs}
    #${mem_grpc_srcs}
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

add_executable(membershipConfigTest
    ${config_source_files}
)

if(USE_LOG4CPP)
    target_compile_definitions(membership_service_server PUBLIC USE_LOG4CPP=1)
    target_compile_definitions(membership_service_client PUBLIC USE_LOG4CPP=1)
    target_compile_definitions(membershipConfigTest PUBLIC USE_LOG4CPP=1)
endif()

if(${USE_COMM_PLAIN_TCP})
    target_compile_definitions(membership_service_server PUBLIC USE_COMM_PLAIN_TCP)
    target_compile_definitions(membership_service_client PUBLIC USE_COMM_PLAIN_TCP)
endif()

if(${USE_COMM_TLS_TCP})
    target_compile_definitions(membership_service_server PUBLIC USE_COMM_TLS_TCP)
    target_compile_definitions(membership_service_client PUBLIC USE_COMM_TLS_TCP)
endif()

set_target_properties(membership_service_client PROPERTIES OUTPUT_NAME client)
set_target_properties(membership_service_server PROPERTIES OUTPUT_NAME server)
set_target_properties(membershipConfigTest PROPERTIES OUTPUT_NAME config)

target_link_libraries(membership_service_client PUBLIC corebft)
target_link_libraries(membership_service_client PUBLIC grpc++)
target_link_libraries(membership_service_client PUBLIC protobuf)
target_link_libraries(membership_service_client PUBLIC grpc)

target_link_libraries(membership_service_server PUBLIC corebft)
target_link_libraries(membership_service_server PUBLIC -lcurl)
target_link_libraries(membershipConfigTest PUBLIC corebft)

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

target_include_directories(membershipConfigTest
                           PRIVATE
                           ${concord_bft_tools_SOURCE_DIR}
                           ${bftengine_SOURCE_DIR}/include
                           ${CONFIG_FOLDER_PATH_VALUE})

add_custom_target(copy_simple_test_scripts_mem ALL COMMENT "Copying scripts of membership_service")
add_custom_command(TARGET copy_simple_test_scripts_mem
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:membership_service_client>/scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:membership_service_client>/scripts)

add_custom_target(copy_tls_script_mem ALL COMMENT "Copying TLS script")
add_custom_command(TARGET copy_tls_script_mem
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/scripts/linux/create_tls_certs.sh
     $<TARGET_FILE_DIR:membership_service_client>/scripts/create_tls_certs.sh)
