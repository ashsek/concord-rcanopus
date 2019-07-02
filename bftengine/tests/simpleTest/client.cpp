// Concord
//
// Copyright (c) 2018 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License").
// You may not use this product except in compliance with the Apache 2.0
// License.
//
// This product may include a number of subcomponents with separate copyright
// notices and license terms. Your use of these subcomponents is subject to the
// terms and conditions of the subcomponent's license, as noted in the LICENSE
// file.

// This program implements a client that sends requests to the simple register
// state machine defined in replica.cpp. It sends a preset number of operations
// to the replicas, and occasionally checks that the responses match
// expectations.
//
// Operations alternate:
//
//  1. `readMod-1` write operations, each with a unique value
//    a. Every second write checks that the returned sequence number is as
//       expected.
//  2. Every `readMod`-th operation is a read, which checks that the value
//     returned is the same as the last value written.
//
// The program expects no arguments. See the `scripts/` directory for
// information about how to run the client.

#include <cassert>
#include <thread>
#include <iostream>
#include <limits>

// bftEngine includes
#include "CommFactory.hpp"
#include "SimpleClient.hpp"

// simpleTest includes
#include "commonDefs.h"
#include "test_comm_config.hpp"
#include "test_parameters.hpp"
#include "Logging.hpp"

#ifdef USE_LOG4CPP
#include <log4cplus/configurator.h>
#endif

//grpc includes
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

//grpc end

using bftEngine::ICommunication;
using bftEngine::PlainUDPCommunication;
using bftEngine::PlainUdpConfig;
using bftEngine::PlainTCPCommunication;
using bftEngine::PlainTcpConfig;
using bftEngine::TlsTCPCommunication;
using bftEngine::TlsTcpConfig;
using bftEngine::SeqNumberGeneratorForClientRequests;
using bftEngine::SimpleClient;

concordlogger::Logger clientLogger =
    concordlogger::Logger::getLogger("simpletest.client");

// #define test_assert(statement, message) \
// { if (!(statement)) { \
// LOG_FATAL(clientLogger, "assert fail with message: " << message); assert(false);}}

void parse_params(int argc, char** argv, ClientParams &cp,
    bftEngine::SimpleClientParams &scp) {
  if(argc < 2)
    return;

  uint16_t min16_t_u = std::numeric_limits<uint16_t>::min();
  uint16_t max16_t_u = std::numeric_limits<uint16_t>::max();
  uint32_t max32_t_u = std::numeric_limits<uint32_t>::max();

  try {
    for (int i = 1; i < argc;) {
      string p(argv[i]);
      if (p == "-i") {
        auto numOp = std::stoi(argv[i + 1]);
        if (numOp < 0 || (uint32_t)numOp > max32_t_u) {
          printf("-i value is out of range (%u - %u)\n", 0, max32_t_u);
          exit(-1);
        }
        cp.numOfOperations = (uint32_t)numOp;
      } else if (p == "-id") {
        auto clId = std::stoi(argv[i + 1]);
        if (clId < min16_t_u || clId > max16_t_u) {
          printf(
              "-id value is out of range (%hu - %hu)\n", min16_t_u, max16_t_u);
          exit(-1);
        }
        cp.clientId = (uint16_t)clId;
      } else if (p == "-r") {
        auto numRep = std::stoi(argv[i + 1]);
        if (numRep < min16_t_u || numRep > max16_t_u) {
          printf("-r value is out of range (%hu - %hu)\n", min16_t_u,
              max16_t_u);
          exit(-1);
        }
        cp.numOfReplicas = (uint16_t)numRep;
      } else if (p == "-cl") {
        auto numCl = std::stoi(argv[i + 1]);
        if (numCl < min16_t_u || numCl > max16_t_u) {
          printf("-cl value is out of range (%hu - %hu)\n", min16_t_u,
          max16_t_u);
          exit(-1);
        }
        cp.numOfClients = (uint16_t)numCl;
      } else if (p == "-c") {
        auto numSlow = std::stoi(argv[i + 1]);
        if (numSlow < min16_t_u || numSlow > max16_t_u) {
          printf(
              "-c value is out of range (%hu - %hu)\n", min16_t_u, max16_t_u);
          exit(-1);
        }
        cp.numOfSlow = (uint16_t)numSlow;
      } else if (p == "-f") {
        auto numF = std::stoi(argv[i + 1]);
        if (numF < min16_t_u || numF > max16_t_u) {
          printf(
              "-f value is out of range (%hu - %hu)\n", min16_t_u, max16_t_u);
          exit(-1);
        }
        cp.numOfFaulty = (uint16_t)numF;
      } else if (p == "-irt") {
        scp.clientInitialRetryTimeoutMilli = std::stoull(argv[i + 1]);
      } else if (p == "-minrt") {
        scp.clientMinRetryTimeoutMilli = std::stoull(argv[i + 1]);
      } else if (p == "-maxrt") {
        scp.clientMaxRetryTimeoutMilli = std::stoull(argv[i + 1]);
      } else if (p == "-srft") {
        auto srft = std::stoi(argv[i + 1]);
        if (srft < min16_t_u || srft > max16_t_u) {
          printf(
              "-srft value is out of range (%hu - %hu)\n", min16_t_u,
              max16_t_u);
          exit(-1);
        }
        scp.clientSendsRequestToAllReplicasFirstThresh = (uint16_t)srft;
      } else if (p == "-srpt") {
        auto srpt = std::stoi(argv[i + 1]);
        if (srpt < min16_t_u || srpt > max16_t_u) {
          printf(
              "-srpt value is out of range (%hu - %hu)\n", min16_t_u,
              max16_t_u);
          exit(-1);
        }
        scp.clientSendsRequestToAllReplicasPeriodThresh = (uint16_t)srpt;
      } else if (p == "-prt") {
        auto prt = std::stoi(argv[i + 1]);
        if (prt < min16_t_u || prt > max16_t_u) {
          printf(
              "-prt value is out of range (%hu - %hu)\n", min16_t_u,
              max16_t_u);
          exit(-1);
        }
        scp.clientPeriodicResetThresh = (uint16_t)prt;
      } else if (p == "-cf") {
        cp.configFileName = argv[i + 1];
      }
      else {
        printf("Unknown parameter %s\n", p.c_str());
        exit(-1);
      }

      i += 2;
    }
  } catch (std::invalid_argument &e) {
    printf("Parameters should be integers only\n");
    exit(-1);
  } catch (std::out_of_range &e) {
    printf("One of the parameters is out of range\n");
    exit(-1);
  }

}


//global variables
ClientParams cp;
bftEngine::SimpleClientParams scp;
SimpleClient* client;

//grpc execution class and function,
class GreeterServiceImpl final : public Greeter::Service {

  //to convert chartacter byte stream back to string,
  string parse_char_string(char * request2, uint32_t requestSize2){

      std::string request_string = "";

      char* pReqId2 = request2;
      // cout << "yoolo:" << pReqId2;
      for (uint32_t i = 0; i < requestSize2; ++i)
      {
          // cout << "yoolo:" << *pReqId2;
          std::string char_request(1, *pReqId2); //conerting requests from char * back to string.
          request_string +=  char_request;
          pReqId2++;
      }

      return request_string;
  }


// grpc request handler, 
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    // std::string prefix("Concord");
    

    //concord start

      LOG_INFO(clientLogger, "Starting " << cp.numOfOperations);
      const int readMod = 1;
      SeqNumberGeneratorForClientRequests* pSeqGen =
          SeqNumberGeneratorForClientRequests::
          createSeqNumberGeneratorForClientRequests();

      for (uint32_t i = 1; i <= cp.numOfOperations; i++) {

        // // the python script that runs the client needs to know how many
        // // iterations has been done - that's the reason we use printf and not
        // // logging module - to keep the output exactly as we expect.


        if (i % readMod == 0) {
          // Read the latest value every readMod-th operation.

          // Prepare request parameters.
          const bool readOnly = true;
          
          std::string test = request->name();
          //Converting request string to char array;
          const uint32_t kRequestLength = test.length();
          char requestBuffer[kRequestLength];
          
          for (uint32_t i = 0; i < test.length(); ++i)
          {
              requestBuffer[i] = (char) test[i];    // Converting string to char
          }

          const char* rawRequestBuffer =
              reinterpret_cast<const char*>(requestBuffer);
          const uint32_t rawRequestLength = sizeof(uint64_t) * kRequestLength;

          const uint64_t requestSequenceNumber =
              pSeqGen->generateUniqueSequenceNumberForRequest();

          const uint64_t timeout = SimpleClient::INFINITE_TIMEOUT;
          const uint32_t kReplyBufferLength = 1000000;
          
          char rawReplyBuffer[kReplyBufferLength];
 
          char* rawReplyBuffer2 = reinterpret_cast<char*>(rawReplyBuffer);
 
          uint32_t actualReplyLength = 0;

          client->sendRequest(readOnly,
                              rawRequestBuffer, rawRequestLength,
                              requestSequenceNumber,
                              timeout,
                              kReplyBufferLength, rawReplyBuffer2, actualReplyLength);

          char* pReqId  = rawReplyBuffer2;
          
          // std::string request_string = parse_char_string(retVal, actualReplyLength);
          std::string request_string = "";

          // char* pReqId = retVal;

          for (uint32_t i = 0; i < actualReplyLength; ++i)
          {
              std::string char_request(1, *pReqId); //converting requests from char * back to string.
              request_string +=  char_request;
              pReqId++;
          }
          reply->set_message(request_string);
          LOG_INFO(clientLogger, "Server_output_read_mode_only" << actualReplyLength);

          // test_assert(actualReplyLength == sizeof(uint64_t),
              // "actualReplyLength != " << sizeof(uint64_t));

          // LOG_INFO(clientLogger, "Server_output_read_mode_only" << *reinterpret_cast<uint64_t*>(replyBuffer));

          // Only assert the last expected value if we have previous set a value.
          // if (hasExpectedLastValue)
            // test_assert(
                // *reinterpret_cast<uint64_t*>(replyBuffer) == expectedLastValue,
                // "*reinterpret_cast<uint64_t*>(replyBuffer)!=" << expectedLastValue);
        } else {
          // Send a write, if we're not doing a read.

          // Generate a value to store.
          // expectedLastValue = (i + 1)*(i + 7)*(i + 18);
          uint64_t mode = 3;

          // Prepare request parameters.
          const bool readOnly = false;

          
          // std::string test = "{ \"Mode\": \"Quorum_Size\", \"ip\": [\"8.8.8.8\", \"8.8.8.9\", \"8.8.9.8\"]}"; // Request to send.
          std::string test = request->name();



          // std::cout << "Enter string:";
          // std::cin >> test;
          //take string as input;
          // getline(cin,test);
          /* Expects JSON string as input, 
             if using directly send input without \" just put normal quotes, { "Mode": "Quorum_Size", "ip": ["8.8.8.8", "8.8.8.9", "8.88"]}
             if sending input from bash do it using ../client <<< "{ \"Mode\": \"Quorum_Size\", \"ip\": [\"8.8.8.8\", \"8.8.8.9\", \"8.88\"]}"
          */
          std::cout << test << "\n";


          // std::cin >> test;
          //Converting request string to char array;
          const uint32_t kRequestLength = test.length();
          char requestBuffer[kRequestLength];
          
          for (uint32_t i = 0; i < test.length(); ++i)
          {
              requestBuffer[i] = (char) test[i];    // Converting string to char
          }

          const char* rawRequestBuffer =
              reinterpret_cast<const char*>(requestBuffer);
          
          const uint32_t rawRequestLength = kRequestLength;

          const uint64_t requestSequenceNumber =
              pSeqGen->generateUniqueSequenceNumberForRequest();

          const uint64_t timeout = SimpleClient::INFINITE_TIMEOUT;

          const uint32_t kReplyBufferLength = 100;
          
          char rawReplyBuffer[kReplyBufferLength];
 
          char* rawReplyBuffer2 = reinterpret_cast<char*>(rawReplyBuffer);
 
          uint32_t actualReplyLength = 0;

          client->sendRequest(readOnly,
                              rawRequestBuffer, rawRequestLength,
                              requestSequenceNumber,
                              timeout,
                              kReplyBufferLength, rawReplyBuffer2, actualReplyLength);

          char* pReqId  = rawReplyBuffer2;
          
          // std::string request_string = parse_char_string(retVal, actualReplyLength);
          std::string request_string = "";

          // char* pReqId = retVal;

          for (uint32_t i = 0; i < actualReplyLength; ++i)
          {
              std::string char_request(1, *pReqId); //converting requests from char * back to string.
              request_string +=  char_request;
              pReqId++;
          }
          reply->set_message(request_string);
          LOG_INFO(clientLogger, "Server_output_read" << request_string << actualReplyLength);// << *retVal);
        }
      }

      // After all requests have been issued, stop communication and clean up.
      // comm->Stop();

      // delete pSeqGen;
      // delete client;
      // delete comm;

      // LOG_INFO(clientLogger, "test done, iterations: " << cp.numOfOperations);

      //concord request end,
    return Status::OK;
  }
};

//global parameters,


int main(int argc, char **argv) {
  parse_params(argc, argv, cp, scp); // parameters for concord 
  // RunServer();
  std::string server_address("0.0.0.0:50051"); // server for grpc
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  #ifdef USE_LOG4CPP
    using namespace log4cplus;
    initialize();
    BasicConfigurator config;
    config.configure();
  #endif

    LOG_INFO(clientLogger, "SimpleClientParams: clientInitialRetryTimeoutMilli: " << scp.clientInitialRetryTimeoutMilli
      << ", clientMinRetryTimeoutMilli: " << scp.clientMinRetryTimeoutMilli
      << ", clientMaxRetryTimeoutMilli: " << scp.clientMaxRetryTimeoutMilli
      << ", clientSendsRequestToAllReplicasFirstThresh: " << scp.clientSendsRequestToAllReplicasFirstThresh
      << ", clientSendsRequestToAllReplicasPeriodThresh: " << scp.clientSendsRequestToAllReplicasPeriodThresh
      << ", clientPeriodicResetThresh: " << scp.clientPeriodicResetThresh);
   const uint16_t id = cp.clientId;

  // How often to read the latest value of the register (every `readMod` ops).


    // Concord clients must tag each request with a unique sequence number. This
    // generator handles that for us.


    TestCommConfig testCommConfig(clientLogger);
    // Configure, create, and start the Concord client to use.
  #ifdef USE_COMM_PLAIN_TCP
    PlainTcpConfig conf = testCommConfig.GetTCPConfig(
        false, id, cp.numOfClients, cp.numOfReplicas, cp.configFileName);
  #elif USE_COMM_TLS_TCP
    TlsTcpConfig conf = testCommConfig.GetTlsTCPConfig(
        false, id, cp.numOfClients, cp.numOfReplicas, cp.configFileName);
  #else
    PlainUdpConfig conf = testCommConfig.GetUDPConfig(
        false, id, cp.numOfClients, cp.numOfReplicas, cp.configFileName);
  #endif

    LOG_INFO(clientLogger, "ClientParams: clientId: "
                           << cp.clientId   
                           << ", numOfReplicas: " << cp.numOfReplicas
                           << ", numOfClients: " << cp.numOfClients
                           << ", numOfIterations: " << cp.numOfOperations
                           << ", fVal: " << cp.numOfFaulty
                           << ", cVal: " << cp.numOfSlow);

    // Perform this check once all parameters configured.
    if (3 * cp.numOfFaulty + 2 * cp.numOfSlow + 1 != cp.numOfReplicas) {
      LOG_FATAL(clientLogger, "Number of replicas is not equal to 3f + 2c + 1 :"
                              " f=" << cp.numOfFaulty << ", c=" << cp.numOfSlow <<
                              ", numOfReplicas=" << cp.numOfReplicas);
      exit(-1);
    }

    ICommunication* comm = bftEngine::CommFactory::create(conf);

    client =
        SimpleClient::createSimpleClient(comm, id, cp.numOfFaulty, cp.numOfSlow);
    comm->Start(); // concord client communication start.

    // This client's index number. Must be larger than the largest replica index
    // number.
  
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait(); //grpc server wait.
  return 0;
}