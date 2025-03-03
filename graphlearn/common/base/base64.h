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

#ifndef GRAPHLEARN_COMMON_BASE_BASE64_H_
#define GRAPHLEARN_COMMON_BASE_BASE64_H_

#include <string>
#include "graphlearn/common/string/lite_string.h"

namespace graphlearn {

bool Base64Encode(const LiteString& input, std::string* output);
bool Base64Encode(const LiteString& input, char* output, size_t* len);

bool Base64Decode(const LiteString& input, std::string* output);
bool Base64Decode(const LiteString& input, char* output, size_t* len);

}  // namespace graphlearn

#endif  // GRAPHLEARN_COMMON_BASE_BASE64_H_
