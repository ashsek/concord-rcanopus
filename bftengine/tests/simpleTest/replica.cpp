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

// This replica implements a single unsigned 64-bit register. Supported
// operations are:
//
//  1. Read the last value of the register.
//  2. Set the value of the register.
//
// Requests are given as either eight or sixteen byte strings. The first eight
// bytes of a requests specifies the type of the request, and the optional
// additional eight bytes specifies the parameters.
//
// # Read Operation
//
// Request bytes must be equal to `(uint64_t)100`.
//
// Response bytes will be equal to `(uint64_t)value`, where `value` is the value
// that was last written to the register.
//
// # Write Operation
//
// Request bytes must be equal to `(uint64_t)200` followed by `(uint64_t)value`,
// where `value` is the value to write to the register.
//
// Response bytes will be equal to `(uint64_t)sequence_number`, where
// `sequence_number` is the count of how many times the register has been
// written to (including this write, so the first response will be `1`).
//
// # Notes
//
// Endianness is not specified. All replicas are assumed to use the same
// native endianness.
//
// The read request must be specified as readOnly. (See
// bftEngine::SimpleClient::sendRequest.)
//
// Values for request types (the `100` and `200` mentioned above) are defined in
// commonDefs.h
//
// See the `scripts/` directory for information about how to run the replicas.

#include <cassert>
#include <thread>
#include <csignal>
#include <cstring>

// bftEngine includes
#include "CommFactory.hpp"
#include "Replica.hpp"
#include "ReplicaConfig.hpp"
#include "SimpleStateTransfer.hpp"
#include "test_comm_config.hpp"
#include "test_parameters.hpp"
#include "Logging.hpp"

// simpleTest includes
#include "commonDefs.h"

// Couchdb includes
#include "couchdbxx.hpp"

#ifdef USE_LOG4CPP
#include <log4cplus/configurator.h>
#endif

using namespace std;

concordlogger::Logger replicaLogger =
    concordlogger::Logger::getLogger("simpletest.replica");
bftEngine::Replica* replica = nullptr;
ReplicaParams rp;

#define test_assert(statement, message) \
{ if (!(statement)) { \
LOG_FATAL(logger, "assert fail with message: " << message); assert(false);}}

using bftEngine::ICommunication;
using bftEngine::PlainUDPCommunication;
using bftEngine::PlainUdpConfig;
using bftEngine::PlainTCPCommunication;
using bftEngine::PlainTcpConfig;
using bftEngine::TlsTcpConfig;
using bftEngine::Replica;
using bftEngine::ReplicaConfig;
using bftEngine::RequestsHandler;
using namespace std;

void parse_params(int argc, char** argv) {
  if(argc < 2) {
    throw std::runtime_error("Unable to read replica id");
  }

  uint16_t min16_t_u = std::numeric_limits<uint16_t>::min();
  uint16_t max16_t_u = std::numeric_limits<uint16_t>::max();
  uint32_t max32_t_u = std::numeric_limits<uint32_t>::max();

  rp.keysFilePrefix = "private_replica_";

  if(argc < 3) { // backward compatibility, only ID is passed
      auto replicaId =  std::stoi(argv[1]);
      if (replicaId < min16_t_u || replicaId > max16_t_u) {
          printf("-id value is out of range (%hu - %hu)", min16_t_u, max16_t_u);
          exit(-1);
      }
      rp.replicaId = replicaId;
  } else {
    try {
      for (int i = 1; i < argc;) {
        string p(argv[i]);
        if (p == "-r") {
          auto numRep = std::stoi(argv[i + 1]);
          if (numRep < min16_t_u || numRep > max16_t_u) {
            printf("-r value is out of range (%hu - %hu)",
                   min16_t_u,
                   max16_t_u);
            exit(-1);
          }
          rp.numOfReplicas = numRep;
          i += 2;
        } else if (p == "-id") {
          auto repId = std::stoi(argv[i + 1]);
          if (repId < min16_t_u || repId > max16_t_u) {
            printf("-id value is out of range (%hu - %hu)",
                   min16_t_u,
                   max16_t_u);
            exit(-1);
          }
          rp.replicaId = repId;
          i += 2;
        } else if (p == "-c") {
          auto numCl = std::stoi(argv[i + 1]);
          if (numCl < min16_t_u || numCl > max16_t_u) {
            printf("-c value is out of range (%hu - %hu)",
                   min16_t_u,
                   max16_t_u);
            exit(-1);
          }
          rp.numOfClients = numCl;
          i += 2;
        } else if (p == "-debug") {
          rp.debug = true;
          i++;
        } else if (p == "-vc") {
          rp.viewChangeEnabled = true;
          i++;
        } else if (p == "-vct") {
          auto vct = std::stoi(argv[i + 1]);
          if (vct < 0 || (uint32_t)vct > max32_t_u) {
            printf("-vct value is out of range (%u - %u)", 0, max32_t_u);
            exit(-1);
          }
          rp.viewChangeTimeout = (uint32_t)vct;
          i += 2;
        } else if (p == "-cf") {
          rp.configFileName = argv[i + 1];
          i += 2;
        } else {
          printf("Unknown parameter %s\n", p.c_str());
          exit(-1);
        }
      }
    } catch (std::invalid_argument &e) {
        printf("Parameters should be integers only\n");
        exit(-1);
      } catch (std::out_of_range &e) {
        printf("One of the parameters is out of range\n");
        exit(-1);
      }
  }

}

// The replica state machine.
class SimpleAppState : public RequestsHandler {
 private:
  uint64_t client_to_index(NodeNum clientId) {
    return clientId - numOfReplicas;
  }

  uint64_t get_last_state_value(NodeNum clientId) {
    auto index = client_to_index(clientId);
    return statePtr[index].lastValue;
  }

  uint64_t get_last_state_num(NodeNum clientId) {
    auto index = client_to_index(clientId);
    return statePtr[index].stateNum;
  }

  void set_last_state_value(NodeNum clientId, uint64_t value) {
    auto index = client_to_index(clientId);
    statePtr[index].lastValue = value;
  }

  void set_last_state_num(NodeNum clientId, uint64_t value) {
    auto index = client_to_index(clientId);
    statePtr[index].stateNum = value;
  }

 public:

  SimpleAppState(uint16_t numCl, uint16_t numRep) :
    statePtr{new SimpleAppState::State[numCl]},
    numOfClients{numCl},
    numOfReplicas{numRep} {}

  //to convert chartacter byte stream back to string,
  string parse_char_string(const char * request, uint32_t requestSize){

      std::string request_string = "";

      const char* pReqId = request;

      for (uint32_t i = 0; i < requestSize; ++i)
      {
          std::string char_request(1, *pReqId); //conerting requests from char * back to string.
          request_string +=  char_request;
          pReqId++;
      }

      return request_string;
  }


  // Handler for the upcall from Concord-BFT.
  int execute(uint16_t clientId,
              uint64_t sequenceNum,
              bool readOnly,
              uint32_t requestSize,
              const char* request,

              uint32_t maxReplySize,
              char* outReply,
              uint32_t& outActualReplySize) override {
    if (readOnly) {

      //Only reading values from couchdb,
      std::string request_string = parse_char_string(request, requestSize);
      jsonxx::Object request_json;
      request_json.parse(request_string);

   
      /*
       Public keys,
       expects: Mode: public_key, BGID: 0/1/2/3....
       returns: json of {ip,public key} and quorum certificate
      */
      if(request_json.get<jsonxx::String>("Mode") == "public_keys"){
        wezside::CouchDBXX couch;  // Object for couchdb

        jsonxx::Object body; // JSON object
        jsonxx::Object o = couch.doc("global_membership_service","public_keys"); // fetch document public key from couchdb
        // cout << o.get<jsonxx::Object>(request_json.get<string>("BGID")).json() << '\n';

        string reply_json = o.get<jsonxx::Object>(request_json.get<string>("BGID")).json();
        // std::string test = "worldhello";
        const uint32_t kReplyLength = reply_json.length();
        
        std::strcpy(outReply, reply_json.c_str());
        outActualReplySize = kReplyLength;
      }

      /*
       Quorum_size,
       expects: Mode: quorum_size, BGID: 0/1/2/3....
       returns: json of {cycle_number, certificate, size }
      */
      if(request_json.get<jsonxx::String>("Mode") == "quorum_size"){
        wezside::CouchDBXX couch;  // Object for couchdb

        jsonxx::Object body; // JSON object
        jsonxx::Object o = couch.doc("global_membership_service","quorum_size"); // fetch document public key from couchdb
        // cout << o.get<jsonxx::Object>(request_json.get<string>("BGID")).json() << '\n';

        string reply_json = o.get<jsonxx::Object>(request_json.get<string>("BGID")).json();
        // std::string test = "worldhello";
        const uint32_t kReplyLength = reply_json.length();
        
        std::strcpy(outReply, reply_json.c_str());
        outActualReplySize = kReplyLength;
      }
      
      /*
       Byzantine_groups,
       expects: Mode: byzantine_groups, BGID: 0/1/2/3....
       returns: json of leader's ip + global quorum certificate
      */
      if(request_json.get<jsonxx::String>("Mode") == "byzantine_groups"){
        wezside::CouchDBXX couch;  // Object for couchdb

        jsonxx::Object body; // JSON object
        jsonxx::Object o = couch.doc("global_membership_service","byzantine_groups"); // fetch document public key from couchdb
        // cout << o.get<jsonxx::Object>(request_json.get<string>("BGID")).json() << '\n';

        string reply_json = o.get<jsonxx::Object>(request_json.get<string>("BGID")).json();
        // std::string test = "worldhello";
        const uint32_t kReplyLength = reply_json.length();
        
        std::strcpy(outReply, reply_json.c_str());
        outActualReplySize = kReplyLength;
      }

      /*
        TODO:
        Emulators,
        expects: Mode: emulators, BGID:0/1/2/3, level:0,1,2,3
        returns: list of ip's
      */


    } else {

      /* Things left to do,
        1. Certificate verification
        2. Emulators,
        3. Cant have dynamic confiuguratuion so cant do anything about membership service changes currently.
      */
      
      // Invoking Couchdb for modifications in the stored values.
      std::string request_string = parse_char_string(request, requestSize); // Casting char * to string.
      jsonxx::Object request_json;
      request_json.parse(request_string); // string to jsonxx object


      /*Update_Quorum_size,
        expects: BGID, cycle, size, quorum_cert
        returns: OK
        Sample: {"Mode": "quorum_size", "BGID":"0", "size":"5", "quorum_cert": "cert" , cycle}
      */

      
      std::string return_ok = "OK";
      uint32_t kReplyLength = return_ok.length();

     
      
      if(request_json.get<jsonxx::String>("Mode") == "quorum_size"){

          std::string cert = request_json.get<jsonxx::String>("quorum_cert");
          //if cert is valid then do the following: (how to check?)


          wezside::CouchDBXX couch;  // Object for couchdb
          jsonxx::Object body; // JSON object to store modified things
          jsonxx::Object o = couch.doc("global_membership_service","quorum_size"); // Fetch document quorum_size from global_membership_service.
          std::cout << o.json() << std::endl; // Print the data fetched from membership service on the console

          body << "_id" << "quorum_size";  //id determines which document to edit
          body << "_rev" << o.get<string>("_rev"); // rev should match previous rev else the document wont be updated          
          
          std::string BGID_tochange = request_json.get<jsonxx::String>("BGID");
      
          // o is old data, body is new which we are building, request_json is values which we want to change
          for(auto kv : o.kv_map()) //iterartor for all values,
          {
            if(kv.first != "_id" && kv.first != "_rev" && kv.first != BGID_tochange) // if we dont have to change it then just copy it
            {
              // jsonxx::Object sub_json;
              cout << kv.first << '\n';
              body << kv.first << o.get<jsonxx::Object>(kv.first); // key/value for that BGID.
            }
          }

          jsonxx::Object sub_json; // make a sub object to send things
          sub_json << "size" << request_json.get<jsonxx::String>("size");
          sub_json << "cycle" << request_json.get<jsonxx::String>("cycle");
          sub_json << "quorum_cert" << request_json.get<jsonxx::String>("quorum_cert");

          body << BGID_tochange << sub_json;

          cout << body.json();
          o = couch.put("global_membership_service", body); //Sending the JSON document back to couchdb;
          std::strcpy(outReply, return_ok.c_str());
          outActualReplySize = kReplyLength;
      }

      /*Update_Public_Keys,
        expects: BGID and json of all keys + certificate, 
        returns: OK
        Sample: {"Mode": "public_keys", "BGID":"0", "keys": {"ip":"key", "ip":"key", "certificate_msp":"certificate"}}
      */
      
      if(request_json.get<jsonxx::String>("Mode") == "public_keys"){
          std::string cert = request_json.get<jsonxx::Object>("keys").get<jsonxx::String>("certificate_msp");
          //if cert is valid then do the following: (how to check?)


          wezside::CouchDBXX couch;  // Object for couchdb
          jsonxx::Object body; // JSON object to store modified things
          jsonxx::Object o = couch.doc("global_membership_service","public_keys"); // Fetch document public_keys from global_membership_service.
          std::cout << o.json() << std::endl; // Print the data fetched from membership service on the console

          body << "_id" << "public_keys";  //id determines which document to edit
          body << "_rev" << o.get<string>("_rev"); // rev should match previous rev else the document wont be updated security by couchdb        
          
          std::string BGID_tochange = request_json.get<jsonxx::String>("BGID");
      
          // o is old data, body is new which we are building, request_json is values which we want to change
          for(auto kv : o.kv_map())
          {
            if(kv.first != "_id" && kv.first != "_rev" && kv.first != BGID_tochange) // if we dont have to change it then just copy it
            {
              // jsonxx::Object sub_json;
              cout << kv.first << '\n';
              body << kv.first << o.get<jsonxx::Object>(kv.first); // key/value for that BGID.
            }
          }

          body << BGID_tochange << request_json.get<jsonxx::Object>("keys");

          cout << body.json();
          o = couch.put("global_membership_service", body); //Sending the JSON document back to couchdb;
          std::strcpy(outReply, return_ok.c_str());
          outActualReplySize = kReplyLength;
      }

      /*Update emulators,
        expects: BGID, list of IP's,


      */

    }

    return 0;
  }

  struct State {
      // Number of modifications made.
      uint64_t stateNum = 0;
      // Register value.
      uint64_t lastValue = 0;
  };

  State *statePtr;

  uint16_t numOfClients;
  uint16_t numOfReplicas;

  concordlogger::Logger logger = concordlogger::Logger::getLogger
      ("simpletest.replica");

  bftEngine::SimpleInMemoryStateTransfer::ISimpleInMemoryStateTransfer* st = nullptr;
  
};

int main(int argc, char **argv) {
      #ifdef USE_LOG4CPP
        using namespace log4cplus;
        initialize();
        BasicConfigurator config;
        config.configure();
      #endif
        parse_params(argc, argv);

        // allows to attach debugger
        if(rp.debug) {
          std::this_thread::sleep_for(chrono::seconds(20));
        }

        ReplicaConfig replicaConfig;
        TestCommConfig testCommConfig(replicaLogger);
        testCommConfig.GetReplicaConfig(
            rp.replicaId, rp.keysFilePrefix, &replicaConfig);
        replicaConfig.numOfClientProxies = rp.numOfClients;
        replicaConfig.autoViewChangeEnabled = rp.viewChangeEnabled;
        replicaConfig.viewChangeTimerMillisec = rp.viewChangeTimeout;

      #ifdef USE_COMM_PLAIN_TCP
        PlainTcpConfig conf = testCommConfig.GetTCPConfig(true, rp.replicaId,
                                                          rp.numOfClients,
                                                          rp.numOfReplicas,
                                                          rp.configFileName);
      #elif USE_COMM_TLS_TCP
        TlsTcpConfig conf = testCommConfig.GetTlsTCPConfig(true, rp.replicaId,
                                                           rp.numOfClients,
                                                           rp.numOfReplicas,
                                                           rp.configFileName);
      #else
        PlainUdpConfig conf = testCommConfig.GetUDPConfig(true, rp.replicaId,
                                                          rp.numOfClients,
                                                          rp.numOfReplicas,
                                                          rp.configFileName);
      #endif

        LOG_DEBUG(replicaLogger, "ReplicaConfig: replicaId: "
                                 << replicaConfig.replicaId
                                 << ", fVal: " << replicaConfig.fVal
                                 << ", cVal: " << replicaConfig.cVal
                                 << ", autoViewChangeEnabled: "
                                 << replicaConfig.autoViewChangeEnabled
                                 << ", viewChangeTimerMillisec: "
                                 << rp.viewChangeTimeout);

        ICommunication* comm = bftEngine::CommFactory::create(conf);

        LOG_INFO(replicaLogger, "ReplicaParams: replicaId: "
                                << rp.replicaId
                                << ", numOfReplicas: " << rp.numOfReplicas
                                << ", numOfClients: " << rp.numOfClients
                                << ", vcEnabled: " << rp.viewChangeEnabled
                                << ", vcTimeout: " << rp.viewChangeTimeout
                                << ", debug: " << rp.debug);

        // This is the state machine that the replica will drive.
        SimpleAppState simpleAppState(rp.numOfClients, rp.numOfReplicas);

        bftEngine::SimpleInMemoryStateTransfer::ISimpleInMemoryStateTransfer* st =
          bftEngine::SimpleInMemoryStateTransfer::create(
              simpleAppState.statePtr,
              sizeof(SimpleAppState::State) * rp.numOfClients,
              replicaConfig.replicaId,
              replicaConfig.fVal,
              replicaConfig.cVal, true);

        simpleAppState.st = st;

        replica = Replica::createNewReplica(
            &replicaConfig,
            &simpleAppState,
            st,
            comm,
            nullptr);

        replica->start();

        // The replica is now running in its own thread. Block the main thread until
        // sigabort, sigkill or sigterm are not raised and then exit gracefully

        while (replica->isRunning())
          std::this_thread::sleep_for(std::chrono::seconds(1));

        return 0;
}