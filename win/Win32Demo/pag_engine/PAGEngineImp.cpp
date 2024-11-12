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

#include "PAGEngineImp.h"
#include "framework.h"
#include <iostream>
#include <string>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <filesystem>

namespace pagengine {
PAGEngineImpl::PAGEngineImpl() {}

PAGEngineImpl::~PAGEngineImpl() {}

bool PAGEngineImpl::InitOnscreenRender(PAGEngineInitData init_data) {
  pag_engine_callback_ = init_data.pag_engine_callback_;
  render_window_ = init_data.hwnd;

  auto drawable = pag::GPUDrawable::FromWindow(render_window_);
  pag_drawable_ = std::shared_ptr<pag::GPUDrawable>(drawable);

  pag_surface_ = pag::PAGSurface::MakeFrom(pag_drawable_);

  pag_player_ = std::make_shared<pag::PAGPlayer>();
  pag_player_->setSurface(pag_surface_);

  InitHighResolutionTime();

  return true;
}
bool PAGEngineImpl::SetPagFile(const char* file_path, int path_length) {
  if (!pag_player_) {
    return false;
  }

  std::string pag_file_path = std::string(file_path, path_length);

  if (pag_file_path.size() == 0) {
    return false;
  }

  auto byte_data = pag::ByteData::FromPath(pag_file_path);
  if (byte_data == nullptr) {
    return false;
  }

  auto file = pag::PAGFile::Load(byte_data->data(), byte_data->length());
  if (file == nullptr) {
    return false;
  }

  pag_file_ = file;

  pag_player_->setComposition(pag_file_);

  return true;
}

bool PAGEngineImpl::SetPagFileBuffer(const byte* file_buffer, int buffer_length) {
  auto file = pag::PAGFile::Load(file_buffer, buffer_length);
  if (file == nullptr) {
    return false;
  }

  //RemoveEmo(file);

  auto background_file = file->copyOriginal();
  auto forgroud_file = file->copyOriginal();
  RemoveEmo(background_file);
  RemoveEmo(forgroud_file);

  auto numVideo = file->numVideos();

  if (numVideo > 0) {
    background_file->removeLayerAt(0);
    forgroud_file->removeLayerAt(1);

    DumpFrames(background_file,true);
    DumpFrames(forgroud_file,false);
  }
  pag_file_ = file;
  pag_player_->setComposition(pag_file_);
  return false;
}

void PAGEngineImpl::RemoveEmo(std::shared_ptr<pag::PAGFile>& file) {
  auto imageLayers = file->getEditableIndices(pag::LayerType::Image);

  for (size_t i = 0; i < imageLayers.size(); i++) {
    auto layers = file->getLayersByEditableIndex(i, pag::LayerType::Image);
    for (size_t j = 0; j < layers.size(); j++) {
      auto bounds = layers[j]->getBounds();
      VSLog("Image Bounds , left : %f,top : %f,right : %f,bottom : %f", bounds.left,bounds.top,bounds.right,bounds.bottom);
      auto matrix = layers[j]->getTotalMatrix();
      file->removeLayer(layers[j]);
    }
  }
}

std::string GetFileNameWithoutExtension(const std::string& filePath) {
  std::filesystem::path path(filePath);
  return path.stem().string();
}

void PAGEngineImpl::DumpFrames(std::shared_ptr<pag::PAGFile>& file,
                               bool isBackground) {
  auto layer = file->getLayerAt(0)->parent();
  VSLog("video name:", file->layerName().c_str());
  auto decoder = pag::PAGDecoder::MakeFrom(layer);
  const int rowSize = decoder->width() * 4;
  const int size = decoder->height() * rowSize;
  std::vector<uint8_t> bytes(size);
  auto fileNameStr = file->path();
  std::filesystem::path filePath(fileNameStr);
  auto parentDir = filePath.parent_path();
  std::filesystem::path newDir;
  if (isBackground) {
    newDir = parentDir / "background";
  } else {
    newDir = parentDir / "foreground";
  }
  std::filesystem::create_directory(newDir);

  for (size_t i = 0; i < decoder->numFrames(); i++) {
    decoder->readFrame(i, (void*)bytes.data(), rowSize);
    stbi_write_png((newDir / (std::to_string(i) + ".png")).string().c_str(), decoder->width(),
                   decoder->height(), 4,
                   bytes.data(),
                   rowSize);
  }
}

bool PAGEngineImpl::IsPagFileValid() {
  if (pag_file_) {
    return true;
  }
  return false;
}

bool PAGEngineImpl::SetProgress(double percent) {
  if (!pag_player_) {
    return false;
  }
  pag_player_->setProgress(percent);
  return true;
}

bool PAGEngineImpl::Flush(bool animating) { return DrawPAG(animating); }

bool PAGEngineImpl::Resize(int width, int height) {
  if (pag_surface_) {
    pag_surface_->updateSize();
  }
  return true;
}

PagSize PAGEngineImpl::GetPagFileSize() {
  if (!pag_file_) {
    return {0, 0};
  }
  return PagSize(pag_file_->width(), pag_file_->height());
}

unsigned int PAGEngineImpl::GetPagFileDuration() {
  if (!pag_file_) {
    return 0;
  }
  return pag_file_->duration();
}

void PAGEngineImpl::SetLoop(bool loop) { is_loop_ = loop; }

bool PAGEngineImpl::DrawPAG(bool animating) {
  if (animating && pag_file_) {
    if (is_loop_ && current_progress_ == 1) {
      return false;
    }

    float frame_rate = pag_file_->frameRate();
    int64_t duration = pag_file_->duration();
    int elapse_ms = ElapseMsFromLastRenderTime(true);
    double elapse_s = elapse_ms / 1000.0;
    auto progress = current_progress_ + elapse_ms / static_cast<double>(duration);
    if (progress > 1) {
      if (!is_loop_) {
        progress = 1;
      } else {
        progress = 0;
      }
    }

    std::cout << " progress : " << progress << std::endl;
    pag_player_->setProgress(progress);
    current_progress_ = progress;
    if (!is_loop_ && current_progress_ == 1) {
      if (pag_engine_callback_) {
        pag_engine_callback_->OnPagPlayEnd();
      }
    }
  }

  return pag_player_->flush();
}

void PAGEngineImpl::InitHighResolutionTime() {
  QueryPerformanceFrequency(&frequency_);
  QueryPerformanceCounter(&last_render_time_);
}

int PAGEngineImpl::ElapseMsFromLastRenderTime(bool update) {
  LARGE_INTEGER current_time = {0};
  LARGE_INTEGER elapse = {0};
  QueryPerformanceCounter(&current_time);
  elapse.QuadPart = current_time.QuadPart - last_render_time_.QuadPart;
  elapse.QuadPart *= 1000000;
  elapse.QuadPart /= frequency_.QuadPart;

  if (update) {
    last_render_time_ = current_time;
  }

  return elapse.QuadPart;
}

}  // namespace pagengine
