/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#define WIN32_LEAN_AND_MEAN  // 从 Windows 头文件中排除极少使用的内容

// C 运行时头文件
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <tchar.h>
#include <vector>
// Windows 头文件
#include <windows.h>

#include "targetver.h"

inline void VSLog(const char* format, ...) {
  va_list args;
  va_start(args, format);

  int length = _vscprintf(format, args) + 1;
  std::vector<char> buffer(length);
  vsnprintf(buffer.data(), length, format, args);

  va_end(args);

  OutputDebugStringA(buffer.data());
}
