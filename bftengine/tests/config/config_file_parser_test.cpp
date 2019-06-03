#include "config_file_parser.hpp"
#include "../../src/bftengine/assertUtils.hpp"

#include <iostream>
#include <string>

using std::cout;
using std::cin;
using std::string;
using std::vector;

int main(int argc, char **argv) {

  cout << "Enter configuration file name with a full/relative path,"
       << " or 'd' for a default:\n";

  const string default_config_file = "scripts/sample_config.txt";
  string config_file = default_config_file;
  string given_config_file;
  const string use_default_config_file = "d";
  const uint expected_replicas_num = 4;
  const uint expected_clients_num = 1;
  const string expected_replica1 = "10.0.2.5:3710";
  const string expected_replica2 = "10.0.2.6:3712";
  const string expected_replica3 = "10.0.2.7:3714";
  const string expected_replica4 = "10.0.2.8:3716";
  const string expected_client   = "10.0.2.8:4444";
  const string values_to_split = "10.23.43.1:1234:1238";
  const string expected_split_values[] = { "10.23.43.1", "1234", "1238" };
  const string values_to_split_delimiter = ":";

  cin >> given_config_file;
  if (given_config_file != use_default_config_file)
    config_file = given_config_file;
  concordlogger::Logger logger =
      concordlogger::Logger::getLogger("simpletest.test");
  ConfigFileParser parser(logger, config_file);
  if (!parser.Parse())
    return 1;

  cout << "\n";
  size_t replicas_num = parser.Count("replicas_config");
  vector<string> replicas = parser.GetValues("replicas_config");

  size_t clients_num = parser.Count("clients_config");
  vector<string> clients = parser.GetValues("clients_config");
  parser.printAll();

  vector<std::string> split_values_vector =
      parser.SplitValue(values_to_split, values_to_split_delimiter.c_str());

  if (config_file == default_config_file) {
    Assert(replicas_num == expected_replicas_num);
    Assert(clients_num == expected_clients_num);
    Assert(expected_replica1 == replicas[0]);
    Assert(expected_replica2 == replicas[1]);
    Assert(expected_replica3 == replicas[2]);
    Assert(expected_replica4 == replicas[3]);
    Assert(expected_client == clients[0]);
    Assert(split_values_vector.size() ==
             sizeof(expected_split_values) / sizeof(expected_split_values[0]));
    Assert(split_values_vector[0] == expected_split_values[0]);
    Assert(split_values_vector[1] == expected_split_values[1]);
    Assert(split_values_vector[2] == expected_split_values[2]);
  }
  return 0;
}
