#include <iostream>

#include "demo/protobuf/test.pb.h"
#include "google/protobuf/util/json_util.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testproto;

using google::protobuf::util::JsonParseOptions;
using google::protobuf::util::JsonPrintOptions;
using google::protobuf::util::JsonStringToMessage;
using google::protobuf::util::MessageToJsonString;

TEST(to_from_json, simple) {
  Student s;
  s.set_name("tony");
  s.set_age(18);
  s.set_grade(Three);

  std::string json{};
  MessageToJsonString(s, &json);
  cout << "json str: " << json << endl;

  Student parsed;
  JsonStringToMessage(json, &parsed);
  cout << "parsed message: {\n" << parsed.DebugString() << "}\n";
}

TEST(to_from_json, print_options) {
  Student s;
  s.set_name("tony");

  static JsonPrintOptions printOptions;
  printOptions.add_whitespace = true;
  printOptions.always_print_primitive_fields = true;
  printOptions.always_print_enums_as_ints = true;
  printOptions.preserve_proto_field_names = true;

  std::string json{};
  MessageToJsonString(s, &json, printOptions);
  cout << "json str: " << json << endl;
}

TEST(to_from_json, parse_otions) {
  Student s;
  s.set_name("tony");
  s.set_age(18);
  s.set_grade(Three);

  std::string json{};
  MessageToJsonString(s, &json);
  // add a field
  json.replace(json.end() - 1, json.end(), ",\"other\":123}");
  cout << "json str: " << json << endl;

  static JsonParseOptions parseOptions;
  parseOptions.ignore_unknown_fields = true;
  // parseOptions.case_insensitive_enum_parsing = true;

  Student parsed;
  JsonStringToMessage(json, &parsed, parseOptions);
  cout << "parsed message: {\n" << parsed.DebugString() << "}\n";
}