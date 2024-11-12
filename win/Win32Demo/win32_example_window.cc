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

#include "win32_example_window.h"

Win32ExampleWindow::Win32ExampleWindow() {}

Win32ExampleWindow::~Win32ExampleWindow() {}

void Win32ExampleWindow::SetTimer(int timer_id, int elapse) {
  ::SetTimer(window_handle_, timer_id, elapse, nullptr);
}

void Win32ExampleWindow::OnCreate() {

    // Create the menu
  HMENU hMenu = CreateMenu();
  HMENU hFileMenu = CreateMenu();

  AppendMenu(hFileMenu, MF_STRING, 1, L"Open");
  AppendMenu(hFileMenu, MF_STRING, 2, L"Exit");

  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

  SetMenu(window_handle_, hMenu);

  Size size = GetWindowSize();
  pag_engine_ = pagengine::CreatePAGEngine();
  pagengine::PAGEngineInitData init_data;
  init_data.hwnd = window_handle_;
  init_data.pag_engine_callback_ = this;
  init_data.width = size.width;
  init_data.height = size.height;
  pag_engine_->InitOnscreenRender(init_data);
  std::vector<std::string> fontPaths = {"../../resources/font/NotoSansSC-Regular.otf",
                                         "../../resources/font/NotoColorEmoji.ttf"};
  std::vector<int> ttcIndices = {0, 0};
  pag::PAGFont::SetFallbackFontPaths(fontPaths, ttcIndices);
  std::string test_pag_file_path = "../../assets/test2.pag";
  OnFileChange(test_pag_file_path);
}

void Win32ExampleWindow::OnFileChange(std::string& test_pag_file_path) {
  auto byte_data = pag::ByteData::FromPath(test_pag_file_path);
  if (byte_data == nullptr) {
    return;
  }
  pag_engine_->SetPagFileBuffer(byte_data->data(), byte_data->length());
  SetTimer(kRenderTimerId, 20);
}

void Win32ExampleWindow::OnDestroy() { PostQuitMessage(0); }

void Win32ExampleWindow::DoPaint(HDC hdc) {
  bool ret = pag_engine_->Flush(true);
}

void Win32ExampleWindow::OnTimer(int timer_id) {
  if (timer_id == kRenderTimerId) {
    RECT client_rect = {0};
    ::GetClientRect(window_handle_, &client_rect);
    ::InvalidateRect(window_handle_, &client_rect, TRUE);
  }
}

void Win32ExampleWindow::OnSize(int w, int h) {
  if (pag_engine_) {
    pag_engine_->Resize(w, h);
  }
}

void Win32ExampleWindow::OnPagPlayEnd() {}


void Win32ExampleWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    case 1:  // Open
      OpenFile();
      break;
    case 2:  // Exit
      PostQuitMessage(0);
      break;
  }
}

void Win32ExampleWindow::OpenFile() {
  OPENFILENAME ofn;
  wchar_t szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = window_handle_;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = L"Pag\0*.pag\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileName(&ofn) == TRUE) {
    // Convert wchar_t to std::string
    std::wstring ws(szFile);
    std::string filePath(ws.begin(), ws.end());
    // Handle the file open logic here
    OnFileChange(filePath);
  }
}