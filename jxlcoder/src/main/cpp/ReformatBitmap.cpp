/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 16/09/2023
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ReformatBitmap.h"
#include <string>
#include "Support.h"
#include "conversion/HalfFloats.h"
#include <android/bitmap.h>
#include <HardwareBuffersCompat.h>
#include <mutex>
#include "JniExceptions.h"
#include "imagebit/RGBAlpha.h"
#include "imagebit/RgbaF16bitNBitU8.h"
#include "imagebit/Rgba8ToF16.h"
#include "imagebit/Rgb565.h"
#include "imagebit/Rgb1010102.h"
#include "imagebit/CopyUnalignedRGBA.h"
#include "imagebit/Rgba16.h"
#include "imagebit/RgbaU16toHF.h"

void
ReformatColorConfig(JNIEnv *env, std::vector<uint8_t> &imageData, std::string &imageConfig,
                    PreferredColorConfig preferredColorConfig, uint32_t depth,
                    uint32_t imageWidth, uint32_t imageHeight, uint32_t *stride, bool *useFloats,
                    jobject *hwBuffer, bool alphaPremultiplied, const bool hasAlphaInOrigin) {
  *hwBuffer = nullptr;
  if (preferredColorConfig == Default) {
    int osVersion = androidOSVersion();
    if (depth > 8 && osVersion >= 26) {
      if (osVersion >= 33 && !hasAlphaInOrigin) {
        preferredColorConfig = Rgba_1010102;
      } else {
        preferredColorConfig = Rgba_F16;
      }
    } else {
      preferredColorConfig = Rgba_8888;
    }
  }

  if (!alphaPremultiplied && hasAlphaInOrigin) {
    if (!(*useFloats)) {
      coder::AssociateAlphaRgba8(imageData.data(), *stride,
                                 imageData.data(), *stride,
                                 imageWidth,
                                 imageHeight);
    } else {
      coder::AssociateAlphaRgba16(reinterpret_cast<uint16_t *>(imageData.data()), *stride,
                                  reinterpret_cast<uint16_t *>(imageData.data()), *stride,
                                  imageWidth,
                                  imageHeight, depth);
    }
  }

  switch (preferredColorConfig) {
    case Rgba_8888:
      if (*useFloats) {
        uint32_t lineWidth = imageWidth * 4 * (uint32_t)sizeof(uint8_t);
        std::vector<uint8_t> rgba8888Data(lineWidth * imageHeight);
        coder::Rgba16ToRgba8(reinterpret_cast<const uint16_t *>(imageData.data()),
                             *stride, rgba8888Data.data(), lineWidth, imageWidth,
                             imageHeight, depth);
        *stride = lineWidth;
        *useFloats = false;
        imageConfig = "ARGB_8888";
        imageData = rgba8888Data;
      }
      break;
    case Rgba_F16:
      if (*useFloats) {
        coder::RgbaU16ToF(reinterpret_cast<const uint16_t *>(imageData.data()),
                          *stride,
                          reinterpret_cast<uint16_t *>(imageData.data()),
                          *stride,
                          imageWidth, imageHeight,
                          depth);
      } else {
        uint32_t
            lineWidth = imageWidth * 4 * (uint32_t)
        sizeof(uint16_t);
        uint32_t alignment = 64;
        uint32_t padding = (alignment - (lineWidth % alignment)) % alignment;
        uint32_t dstStride = lineWidth + padding;
        std::vector<uint8_t> rgbaF16Data(dstStride * imageHeight);
        coder::Rgba8ToF16(imageData.data(), *stride,
                          reinterpret_cast<uint16_t *>(rgbaF16Data.data()), dstStride,
                          imageWidth, imageHeight, !alphaPremultiplied);
        *stride = dstStride;
        *useFloats = true;
        imageConfig = "RGBA_F16";
        imageData = rgbaF16Data;
      }
      break;
    case Rgb_565:
      if (*useFloats) {
        uint32_t
            lineWidth = imageWidth * (uint32_t)
        sizeof(uint16_t);
        uint32_t alignment = 64;
        uint32_t padding = (alignment - (lineWidth % alignment)) % alignment;
        uint32_t dstStride = lineWidth + padding;
        std::vector<uint8_t> rgb565Data(dstStride * imageHeight);
        coder::Rgba16To565(reinterpret_cast<const uint16_t *>(imageData.data()),
                           *stride,
                           reinterpret_cast<uint16_t *>(rgb565Data.data()), dstStride,
                           imageWidth, imageHeight, depth);
        *stride = dstStride;
        *useFloats = false;
        imageConfig = "RGB_565";
        imageData = rgb565Data;
        break;
      } else {
        uint32_t
            lineWidth = imageWidth * (uint32_t)
        sizeof(uint16_t);
        uint32_t alignment = 64;
        uint32_t padding = (alignment - (lineWidth % alignment)) % alignment;
        uint32_t dstStride = lineWidth + padding;
        std::vector<uint8_t> rgb565Data(dstStride * imageHeight);
        coder::Rgba8To565(imageData.data(), *stride,
                          reinterpret_cast<uint16_t *>(rgb565Data.data()), dstStride,
                          imageWidth, imageHeight,
                          !alphaPremultiplied);
        *stride = dstStride;
        *useFloats = false;
        imageConfig = "RGB_565";
        imageData = rgb565Data;
      }
      break;
    case Rgba_1010102:
      if (*useFloats) {
        uint32_t
            lineWidth = imageWidth * (uint32_t)
        sizeof(uint32_t);
        uint32_t alignment = 64;
        uint32_t padding = (alignment - (lineWidth % alignment)) % alignment;
        uint32_t dstStride = lineWidth + padding;
        std::vector<uint8_t> rgba1010102Data(dstStride * imageHeight);
        coder::Rgba16ToRGBA1010102(reinterpret_cast<const uint16_t *>(imageData.data()),
                                   *stride,
                                   reinterpret_cast<uint8_t *>(rgba1010102Data.data()),
                                   dstStride,
                                   imageWidth, imageHeight, depth);
        *stride = dstStride;
        *useFloats = false;
        imageConfig = "RGBA_1010102";
        imageData = rgba1010102Data;
        break;
      } else {
        uint32_t
            lineWidth = imageWidth * (uint32_t)
        sizeof(uint32_t);
        uint32_t alignment = 64;
        uint32_t padding = (alignment - (lineWidth % alignment)) % alignment;
        uint32_t dstStride = lineWidth + padding;
        std::vector<uint8_t> rgba1010102Data(dstStride * imageHeight);
        coder::Rgba8ToRGBA1010102(reinterpret_cast<const uint8_t *>(imageData.data()),
                                  *stride,
                                  reinterpret_cast<uint8_t *>(rgba1010102Data.data()),
                                  dstStride,
                                  imageWidth, imageHeight,
                                  !alphaPremultiplied);
        *stride = dstStride;
        *useFloats = false;
        imageConfig = "RGBA_1010102";
        imageData = rgba1010102Data;
        break;
      }
      break;
    case Hardware: {
      if (!loadAHardwareBuffersAPI()) {
        std::string err = "Cannot load hardware buffers API";
        throw std::runtime_error(err);
      }
      AHardwareBuffer_Desc bufferDesc = {0};
      bufferDesc.width = imageWidth;
      bufferDesc.height = imageHeight;
      bufferDesc.layers = 1;
      bufferDesc.format = (*useFloats) ? AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT
                                       : AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

      bufferDesc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;

      AHardwareBuffer *hardwareBuffer = nullptr;

      int status = AHardwareBuffer_allocate_compat(&bufferDesc, &hardwareBuffer);
      if (status != 0) {
        std::string err = "Cannot allocate Hardware Buffer";
        throw std::runtime_error(err);
      }
      ARect rect = {0, 0, static_cast<int>(imageWidth), static_cast<int>(imageHeight)};
      uint8_t *buffer = nullptr;

      status = AHardwareBuffer_lock_compat(hardwareBuffer,
                                           AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1,
                                           &rect, reinterpret_cast<void **>(&buffer));
      if (status != 0) {
        AHardwareBuffer_release_compat(hardwareBuffer);
        std::string err = "Cannot lock hardware buffer for write";
        throw std::runtime_error(err);
      }

      AHardwareBuffer_describe_compat(hardwareBuffer, &bufferDesc);

      if (*useFloats) {
        coder::RgbaU16ToF(reinterpret_cast<const uint16_t *>(imageData.data()),
                          *stride,
                          reinterpret_cast<uint16_t *>(buffer),
                          (uint32_t) bufferDesc.stride * 4 * sizeof(uint16_t),
                          imageWidth, imageHeight,
                          depth);
      } else {
        coder::CopyUnaligned(reinterpret_cast<uint8_t *>(imageData.data()),
                             *stride,
                             reinterpret_cast<uint8_t *>(buffer),
                             (uint32_t) bufferDesc.stride * 4 * sizeof(uint8_t),
                             (uint32_t) bufferDesc.width * 4,
                             (uint32_t) bufferDesc.height);
      }

      status = AHardwareBuffer_unlock_compat(hardwareBuffer, nullptr);
      if (status != 0) {
        AHardwareBuffer_release_compat(hardwareBuffer);
        std::string err = "Cannot unlock hardware buffer";
        throw std::runtime_error(err);
      }

      jobject buf = AHardwareBuffer_toHardwareBuffer_compat(env, hardwareBuffer);

      AHardwareBuffer_release_compat(hardwareBuffer);

      *hwBuffer = buf;
      imageConfig = "HARDWARE";
    }
      break;
    default:break;
  }
}