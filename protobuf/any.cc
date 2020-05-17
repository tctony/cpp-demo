#include "absl/strings/str_cat.h"
#include "demo/protobuf/test.pb.h"
#include "gtest/gtest.h"

TEST(protobuf, any) {
  testproto::Envelope evp;
  evp.set_seq(123);
  for (int i = 0; i < 100; ++i) {
    if (i % 2 == 0) {
      testproto::Object1 obj;
      obj.set_info(absl::StrCat("object1, info ", i));
      obj.set_foo(absl::StrCat("foo", i));
      evp.add_objects()->PackFrom(obj);
    } else {
      testproto::Object2 obj;
      obj.set_info(absl::StrCat("object2, info ", i));
      obj.set_bar(absl::StrCat("bar", i));
      evp.add_objects()->PackFrom(obj);
    }
  }

  std::string buf = evp.SerializeAsString();

  testproto::Envelope parsed;
  parsed.ParseFromString(buf);

  EXPECT_EQ(parsed.seq(), evp.seq());
  int index = 0;
  for (const google::protobuf::Any& obj : parsed.objects()) {
    if (obj.Is<testproto::Object1>()) {
      EXPECT_EQ(index % 2, 0);
      testproto::Object1 object1;
      EXPECT_EQ(obj.UnpackTo(&object1), true);
      EXPECT_EQ(object1.info(), absl::StrCat("object1, info ", index));
      EXPECT_EQ(object1.foo(), absl::StrCat("foo", index));
    } else if (obj.Is<testproto::Object2>()) {
      EXPECT_EQ(index % 2, 1);
      testproto::Object2 object2;
      EXPECT_EQ(obj.UnpackTo(&object2), true);
      EXPECT_EQ(object2.info(), absl::StrCat("object2, info ", index));
      EXPECT_EQ(object2.bar(), absl::StrCat("bar", index));
    } else {
      FAIL();
    }
    index++;
  }
}