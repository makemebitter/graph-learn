/* Copyright 2020 Alibaba Group Holding Limited. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <fstream>
#include "graphlearn/common/base/errors.h"
#include "graphlearn/common/base/log.h"
#include "graphlearn/core/io/edge_loader.h"
#include "graphlearn/platform/env.h"
#include "gtest/gtest.h"

using namespace graphlearn;  // NOLINT [build/namespaces]
using namespace graphlearn::io;  // NOLINT [build/namespaces]

class EdgeLoaderTest : public ::testing::Test {
public:
  EdgeLoaderTest() {
    InitGoogleLogging();
  }
  ~EdgeLoaderTest() {
    UninitGoogleLogging();
  }

protected:
  void SetUp() override {
  }

  void TearDown() override {
  }

  void GenTestData(const char* file_name, int32_t format) {
    SideInfo info;
    info.format = format;

    std::ofstream out(file_name);

    // write title
    if (info.IsWeighted() && info.IsLabeled() && info.IsAttributed()) {
      const char* title = "src_id:int64\tdst_id:int64\tedge_weight:float\tlabel:int32\tattribute:string\n";
      out.write(title, strlen(title));
    } else if (info.IsWeighted() && info.IsLabeled()) {
      const char* title = "src_id:int64\tdst_id:int64\tedge_weight:float\tlabel:int32\n";
      out.write(title, strlen(title));
    } else if (info.IsWeighted() && info.IsAttributed()) {
      const char* title = "src_id:int64\tdst_id:int64\tedge_weight:float\tattribute:string\n";
      out.write(title, strlen(title));
    } else if (info.IsLabeled() && info.IsAttributed()) {
      const char* title = "src_id:int64\tdst_id:int64\tlabel:int32\tattribute:string\n";
      out.write(title, strlen(title));
    } else if (info.IsWeighted()) {
      const char* title = "src_id:int64\tdst_id:int64\tedge_weight:float\n";
      out.write(title, strlen(title));
    } else if (info.IsLabeled()) {
      const char* title = "src_id:int64\tdst_id:int64\tlabel:int32\n";
      out.write(title, strlen(title));
    } else {
      const char* title = "src_id:int64\tdst_id:int64\tattribute:string\n";
      out.write(title, strlen(title));
    }

    // write data
    int size = 0;
    char buffer[64];
    for (int32_t i = 0; i < 100; ++i) {
      if (info.IsWeighted() && info.IsLabeled() && info.IsAttributed()) {
        size = snprintf(buffer, sizeof(buffer),
                        "%d\t%d\t%f\t%d\t%d:%f:%c\n",
                        i, i, float(i), i, i, float(i), char(i % 26 + 'A'));
      } else if (info.IsWeighted() && info.IsLabeled()) {
        size = snprintf(buffer, sizeof(buffer),
                        "%d\t%d\t%f\t%d\n", i, i, float(i), i);
      } else if (info.IsWeighted() && info.IsAttributed()) {
        size = snprintf(buffer, sizeof(buffer),
                        "%d\t%d\t%f\t%d:%f:%c\n",
                        i, i, float(i), i, float(i), char(i % 26 + 'A'));
      } else if (info.IsLabeled() && info.IsAttributed()) {
        size = snprintf(buffer, sizeof(buffer),
                        "%d\t%d\t%d\t%d:%f:%c\n",
                        i, i, i, i, float(i), char(i % 26 + 'A'));
      } else if (info.IsWeighted()) {
        size = snprintf(buffer, sizeof(buffer), "%d\t%d\t%f\n", i, i, float(i));
      } else if (info.IsLabeled()) {
        size = snprintf(buffer, sizeof(buffer), "%d\t%d\t%d\n", i, i, i);
      } else {
        size = snprintf(buffer, sizeof(buffer),
                        "%d\t%d\t%d:%f:%c\n",
                        i, i, i, float(i), char(i % 26 + 'A'));
      }
      out.write(buffer, size);
    }
    out.close();
  }

  void GenEdgeSource(EdgeSource* source, int32_t format,
                     const std::string& file_name,
                     const std::string& edge_type,
                     const std::string& src_type,
                     const std::string& dst_type) {
    source->path = file_name;
    source->edge_type = edge_type;
    source->src_id_type = src_type;
    source->dst_id_type = dst_type;
    source->format = format;
    source->ignore_invalid = false;
    if (format & kAttributed) {
      source->delimiter = ":";
      source->types = {DataType::kInt32, DataType::kFloat, DataType::kString};
      source->hash_buckets = {0 ,0, 0};
    }
  }

  void TestEdgeLoader(EdgeLoader* loader, int32_t format,
                      const std::string& edge_type,
                      const std::string& src_type,
                      const std::string& dst_type,
                      int32_t from, int32_t to) {
    const SideInfo* info = loader->GetSideInfo();
    EXPECT_EQ(info->format, format);
    EXPECT_EQ(info->type, edge_type);
    EXPECT_EQ(info->src_type, src_type);
    EXPECT_EQ(info->dst_type, dst_type);

    Status s;
    EdgeValue value;
    int64_t index = from;
    while (index < to) {
      s = loader->Read(&value);
      EXPECT_TRUE(s.ok());
      Check(value, index, info);
      ++index;
    }
    s = loader->Read(&value);
    EXPECT_TRUE(error::IsOutOfRange(s));
  }

  void Check(EdgeValue& value, int32_t index, const SideInfo* info) {
    EXPECT_EQ(value.src_id, index);
    EXPECT_EQ(value.dst_id, index);
    if (info->IsWeighted()) {
      EXPECT_FLOAT_EQ(value.weight, float(index));
    }
    if (info->IsLabeled()) {
      EXPECT_EQ(value.label, index);
    }
    if (info->IsAttributed()) {
      EXPECT_EQ(value.i_attrs[0], index);
      EXPECT_FLOAT_EQ(value.f_attrs[0], float(index));
      EXPECT_EQ(value.s_attrs[0].length(), 1);
      EXPECT_EQ(value.s_attrs[0][0], char('A' + index % 26));
    }
  }
};

TEST_F(EdgeLoaderTest, ReadMultiFiles) {
  const char* w_file = "weighted_file";
  const char* l_file = "labeled_file";
  const char* a_file = "attributed_file";

  GenTestData(w_file, kWeighted);
  GenTestData(l_file, kLabeled);
  GenTestData(a_file, kAttributed);

  std::vector<EdgeSource> source(3);
  GenEdgeSource(&source[0], kWeighted, w_file, "click", "user", "item");
  GenEdgeSource(&source[1], kLabeled, l_file, "buy", "user", "item");
  GenEdgeSource(&source[2], kAttributed, a_file, "watch", "user", "movie");

  EdgeLoader* loader = new EdgeLoader(source, Env::Default(), 0, 1);

  // check the first file
  Status s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted, "click", "user", "item", 0, 100);

  // check the second file
  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kLabeled, "buy", "user", "item", 0, 100);

  // check the third file
  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kAttributed, "watch", "user", "movie", 0, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));

  delete loader;
}

TEST_F(EdgeLoaderTest, ReadWeightedLabeled) {
  const char* file = "wl_file";
  GenTestData(file, kWeighted | kLabeled);

  std::vector<EdgeSource> source(1);
  GenEdgeSource(&source[0], kWeighted | kLabeled,
                file, "click", "user", "item");

  // read all
  EdgeLoader* loader = new EdgeLoader(source, Env::Default(), 0, 1);

  Status s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kLabeled, "click", "user", "item", 0, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;

  // read part
  loader = new EdgeLoader(source, Env::Default(), 1, 2);

  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kLabeled, "click", "user", "item", 50, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;
}

TEST_F(EdgeLoaderTest, ReadWeightedAttributed) {
  const char* file = "wa_file";
  GenTestData(file, kWeighted | kAttributed);

  std::vector<EdgeSource> source(1);
  GenEdgeSource(&source[0], kWeighted | kAttributed,
                file, "click", "user", "item");

  // read all
  EdgeLoader* loader = new EdgeLoader(source, Env::Default(), 0, 1);

  Status s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kAttributed, "click", "user", "item", 0, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;

  // read part
  loader = new EdgeLoader(source, Env::Default(), 1, 2);

  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kAttributed, "click", "user", "item", 50, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;
}

TEST_F(EdgeLoaderTest, ReadLabeledAttributed) {
  const char* file = "la_file";
  GenTestData(file, kLabeled | kAttributed);

  std::vector<EdgeSource> source(1);
  GenEdgeSource(&source[0], kLabeled | kAttributed,
                file, "click", "user", "item");

  // read all
  EdgeLoader* loader = new EdgeLoader(source, Env::Default(), 0, 1);

  Status s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kLabeled | kAttributed, "click", "user", "item", 0, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;

  // read part
  loader = new EdgeLoader(source, Env::Default(), 1, 2);

  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kLabeled | kAttributed, "click", "user", "item", 50, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;
}

TEST_F(EdgeLoaderTest, ReadWeightedLabeledAttributed) {
  const char* file = "wla_file";
  GenTestData(file, kWeighted | kLabeled | kAttributed);

  std::vector<EdgeSource> source(1);
  GenEdgeSource(&source[0], kWeighted | kLabeled | kAttributed,
                file, "click", "user", "item");

  // read all
  EdgeLoader* loader = new EdgeLoader(source, Env::Default(), 0, 1);

  Status s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kLabeled | kAttributed,
                 "click", "user", "item", 0, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;

  // read part
  loader = new EdgeLoader(source, Env::Default(), 1, 2);

  s = loader->BeginNextFile();
  EXPECT_TRUE(s.ok());
  TestEdgeLoader(loader, kWeighted | kLabeled | kAttributed,
                 "click", "user", "item", 50, 100);

  s = loader->BeginNextFile();
  EXPECT_TRUE(error::IsOutOfRange(s));
  delete loader;
}
