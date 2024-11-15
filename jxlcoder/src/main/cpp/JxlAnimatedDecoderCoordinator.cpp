/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 08/01/2024
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

#include "JxlAnimatedDecoderCoordinator.h"
#include "SizeScaler.h"
#include "Support.h"
#include "JniExceptions.h"
#include <jni.h>
#include "interop/JxlAnimatedDecoder.hpp"
#include "colorspaces/colorspace.h"
#include "android/bitmap.h"
#include "ReformatBitmap.h"
#include "hwy/highway.h"
#include "colorspaces/ColorSpaceProfile.h"
#include "imagebit/CopyUnalignedRGBA.h"

using namespace std;

extern "C"
JNIEXPORT jlong JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_createCoordinator(JNIEnv *env, jobject thiz,
                                                            jobject byteBuffer,
                                                            jint javaPreferredColorConfig,
                                                            jint javaScaleMode,
                                                            jint javaJxlResizeSampler,
                                                            jint javaToneMapper) {
  ScaleMode scaleMode;
  PreferredColorConfig preferredColorConfig;
  XSampler sampler;
  CurveToneMapper toneMapper;
  if (!checkDecodePreconditions(env, javaPreferredColorConfig, &preferredColorConfig,
                                javaScaleMode, &scaleMode, javaJxlResizeSampler, &sampler,
                                javaToneMapper, &toneMapper)) {
    return 0;
  }

//    if (scaleWidth < 1 || scaleHeight < 1) {
//        std::string errorString =
//                "Invalid size provided, all sizes must be more than 0! Width: " +
//                std::to_string(scaleWidth) + ", height: " + std::to_string(scaleHeight);
//        throwException(env, errorString);
//        return 0;
//    }
  try {
    auto bufferAddress = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(byteBuffer));
    int length = (int) env->GetDirectBufferCapacity(byteBuffer);
    if (!bufferAddress || length <= 0) {
      std::string errorString = "Only direct byte buffers are supported";
      throwException(env, errorString);
      return 0;
    }
    vector<uint8_t> srcBuffer(length);
    copy(bufferAddress, bufferAddress + length, srcBuffer.begin());
    auto decoder = new JxlAnimatedDecoder(srcBuffer);
    auto coordinator = new JxlAnimatedDecoderCoordinator(
        decoder, scaleMode, preferredColorConfig, sampler, toneMapper
    );
    return reinterpret_cast<jlong >(coordinator);
  } catch (AnimatedDecoderError &err) {
    std::string errorString = err.what();
    throwException(env, errorString);
    return 0;
  } catch (std::bad_alloc &err) {
    std::string errorString = "OOM: " + string(err.what());
    throwException(env, errorString);
    return 0;
  }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_createCoordinatorByteArray(JNIEnv *env, jobject thiz,
                                                                     jbyteArray byteArray,
                                                                     jint javaPreferredColorConfig,
                                                                     jint javaScaleMode,
                                                                     jint javaJxlResizeSampler,
                                                                     jint javaToneMapper) {
  ScaleMode scaleMode;
  PreferredColorConfig preferredColorConfig;
  XSampler sampler;
  CurveToneMapper toneMapper;
  if (!checkDecodePreconditions(env, javaPreferredColorConfig, &preferredColorConfig,
                                javaScaleMode, &scaleMode, javaJxlResizeSampler, &sampler,
                                javaToneMapper, &toneMapper)) {
    return 0;
  }

  try {
    auto length = env->GetArrayLength(byteArray);
    vector<uint8_t> srcBuffer(length);
    env->GetByteArrayRegion(byteArray, 0, length,
                            reinterpret_cast<jbyte *>(srcBuffer.data()));
    auto decoder = new JxlAnimatedDecoder(srcBuffer);
    auto coordinator = new JxlAnimatedDecoderCoordinator(
        decoder, scaleMode, preferredColorConfig, sampler, toneMapper
    );
    return reinterpret_cast<jlong >(coordinator);
  } catch (AnimatedDecoderError &err) {
    std::string errorString = err.what();
    throwException(env, errorString);
    return 0;
  } catch (std::bad_alloc &err) {
    std::string errorString = "OOM: " + string(err.what());
    throwException(env, errorString);
    return 0;
  }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_closeAndReleaseAnimatedImage(JNIEnv *env, jobject thiz,
                                                                       jlong coordinatorPtr) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  delete coordinator;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getNumberOfFrames(JNIEnv *env, jobject thiz,
                                                            jlong coordinatorPtr) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  return coordinator->numberOfFrames();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getFrameDurationImpl(JNIEnv *env, jobject thiz,
                                                               jlong coordinatorPtr, jint frame) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  return coordinator->frameDuration(frame);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getLoopsCount(JNIEnv *env, jobject thiz,
                                                        jlong coordinatorPtr) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  return coordinator->loopsCount();
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getFrameImpl(JNIEnv *env, jobject thiz,
                                                       jlong coordinatorPtr, jint frameIndex,
                                                       jint scaleWidth, jint scaleHeight) {
  try {
    auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);

    JxlFrame frame = coordinator->getFrame(frameIndex);
    vector<uint8_t> iccProfile = frame.iccProfile;
    vector<uint8_t> rgbaPixels = frame.pixels;
    // Currently always in 8bpp;
    bool useFloat16 = false;
    const uint32_t bitDepth = 8;
    const bool alphaPremultiplied = coordinator->isAlphaAttenuated();

    auto preferEncoding = frame.preferColorEncoding;
    auto colorEncoding = frame.colorEncoding;

    uint32_t stride = coordinator->getWidth() * 4 * static_cast<uint32_t>(useFloat16 ? sizeof(uint16_t) : sizeof(uint8_t));

    if (preferEncoding && (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_PQ ||
        colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_HLG ||
        colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_DCI ||
        colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_709 ||
        colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_GAMMA ||
        colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_SRGB)
        && colorEncoding.color_space == JXL_COLOR_SPACE_RGB) {
      Eigen::Matrix3f sourceProfile;
      TransferFunction transferFunction = TransferFunction::Srgb;
      CurveToneMapper toneMapper = CurveToneMapper::TONE_SKIP;
      bool useChromaticAdaptation = false;
      float gamma = 2.2f;
      if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_HLG) {
        transferFunction = TransferFunction::Hlg;
      } else if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_DCI) {
        toneMapper = TONE_SKIP;
        transferFunction = TransferFunction::Smpte428;
      } else if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_PQ) {
        transferFunction = TransferFunction::Pq;
      } else if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_GAMMA) {
        toneMapper = TONE_SKIP;
        // Make real gamma
        transferFunction = TransferFunction::Gamma2p2;
        gamma = 1.f / colorEncoding.gamma;
      } else if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_709) {
        toneMapper = TONE_SKIP;
        transferFunction = TransferFunction::Itur709;
      } else if (colorEncoding.transfer_function == JXL_TRANSFER_FUNCTION_SRGB) {
        toneMapper = TONE_SKIP;
        transferFunction = TransferFunction::Srgb;
      }

      Eigen::Matrix<float, 3, 2> primaries;
      Eigen::Vector2f whitePoint;

      if (colorEncoding.primaries == JXL_PRIMARIES_2100) {
        sourceProfile = GamutRgbToXYZ(getRec2020Primaries(), getIlluminantD65());
        primaries << getRec2020Primaries();
        whitePoint << getIlluminantD65();
      } else if (colorEncoding.primaries == JXL_PRIMARIES_P3) {
        sourceProfile = GamutRgbToXYZ(getDisplayP3Primaries(), getIlluminantD65());
        primaries << getDisplayP3Primaries();
        whitePoint << getIlluminantD65();
      } else if (colorEncoding.primaries == JXL_PRIMARIES_SRGB) {
        sourceProfile = GamutRgbToXYZ(getSRGBPrimaries(), getIlluminantD65());
        primaries << getSRGBPrimaries();
        whitePoint << getIlluminantD65();
      } else {
        primaries << static_cast<float>(colorEncoding.primaries_red_xy[0]),
            static_cast<float>(colorEncoding.primaries_red_xy[1]),
            static_cast<float>(colorEncoding.primaries_green_xy[0]),
            static_cast<float>(colorEncoding.primaries_green_xy[1]),
            static_cast<float>(colorEncoding.primaries_blue_xy[0]),
            static_cast<float>(colorEncoding.primaries_blue_xy[1]);
        whitePoint << static_cast<float>(colorEncoding.white_point_xy[0]),
            static_cast<float>(colorEncoding.white_point_xy[1]);
        if (whitePoint != getIlluminantD65()) {
          useChromaticAdaptation = true;
        }
        sourceProfile = GamutRgbToXYZ(primaries, whitePoint);
      }

      Eigen::Matrix3f dstProfile = GamutRgbToXYZ(getRec709Primaries(), getIlluminantD65());
      Eigen::Matrix3f conversion = dstProfile.inverse() * sourceProfile;

      ITURColorCoefficients coeffs = colorPrimariesComputeYCoeffs(primaries, whitePoint);

      const float matrix[9] = {
          conversion(0, 0), conversion(0, 1), conversion(0, 2),
          conversion(1, 0), conversion(1, 1), conversion(1, 2),
          conversion(2, 0), conversion(2, 1), conversion(2, 2),
      };

      applyColorMatrix(reinterpret_cast<uint8_t *>(rgbaPixels.data()),
                       stride,
                       (uint32_t) coordinator->getWidth(),
                       (uint32_t) coordinator->getHeight(),
                       matrix,
                       transferFunction,
                       TransferFunction::Srgb,
                       toneMapper,
                       coeffs, 255.);
    }

    if (!iccProfile.empty() && !frame.preferColorEncoding) {
      convertUseDefinedColorSpace(rgbaPixels,
                                  stride,
                                  (uint32_t) coordinator->getWidth(),
                                  (uint32_t) coordinator->getHeight(), iccProfile.data(),
                                  iccProfile.size(),
                                  useFloat16);
    }
    uint32_t scaledWidth = scaleWidth;
    uint32_t scaledHeight = scaleHeight;
    bool useSampler = (scaledWidth > 0 || scaledHeight > 0) && (scaledWidth != 0 && scaledHeight != 0);

    uint32_t finalWidth = coordinator->getWidth();
    uint32_t finalHeight = coordinator->getHeight();

    if (useSampler && scaledHeight > 0 && scaledWidth > 0) {
      auto scaleResult = RescaleImage(rgbaPixels, env, &stride, useFloat16,
                                      reinterpret_cast<uint32_t *>(&finalWidth),
                                      reinterpret_cast<uint32_t *>(&finalHeight),
                                      scaledWidth, scaledHeight, alphaPremultiplied,
                                      bitDepth,
                                      coordinator->getScaleMode(),
                                      coordinator->getSampler(), frame.hasAlphaInOrigin);
      if (!scaleResult) {
        return nullptr;
      }
    }

    std::string bitmapPixelConfig = useFloat16 ? "RGBA_F16" : "ARGB_8888";
    jobject hwBuffer = nullptr;
    ReformatColorConfig(env, rgbaPixels, bitmapPixelConfig,
                        coordinator->getPreferredColorConfig(), 8,
                        finalWidth, finalHeight, &stride, &useFloat16,
                        &hwBuffer, alphaPremultiplied, frame.hasAlphaInOrigin);

    if (bitmapPixelConfig == "HARDWARE") {
      jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
      jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass,
                                                              "wrapHardwareBuffer",
                                                              "(Landroid/hardware/HardwareBuffer;Landroid/graphics/ColorSpace;)Landroid/graphics/Bitmap;");
      jobject emptyObject = nullptr;
      jobject bitmapObj = env->CallStaticObjectMethod(bitmapClass,
                                                      createBitmapMethodID,
                                                      hwBuffer, emptyObject);
      return bitmapObj;
    }

    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgba8888FieldID = env->GetStaticFieldID(bitmapConfig,
                                                     bitmapPixelConfig.c_str(),
                                                     "Landroid/graphics/Bitmap$Config;");
    jobject rgba8888Obj = env->GetStaticObjectField(bitmapConfig, rgba8888FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jobject bitmapObj = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID,
                                                    static_cast<jint>(finalWidth),
                                                    static_cast<jint>(finalHeight),
                                                    rgba8888Obj);

    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmapObj, &info) < 0) {
      throwPixelsException(env);
      return static_cast<jbyteArray>(nullptr);
    }

    void *addr;
    if (AndroidBitmap_lockPixels(env, bitmapObj, &addr) != 0) {
      throwPixelsException(env);
      return static_cast<jobject>(nullptr);
    }

    if (bitmapPixelConfig == "RGB_565") {
      coder::CopyUnaligned(reinterpret_cast<const uint16_t *>(rgbaPixels.data()), stride,
                           reinterpret_cast<uint16_t *>(addr), info.stride,
                           info.width,
                           info.height);
    } else {
      if (useFloat16) {
        coder::CopyUnaligned(reinterpret_cast<const uint16_t *>(rgbaPixels.data()), stride,
                             reinterpret_cast<uint16_t *>(addr), (uint32_t) info.stride,
                             (uint32_t) info.width * 4,
                             (uint32_t) info.height);
      } else {
        coder::CopyUnaligned(reinterpret_cast<const uint8_t *>(rgbaPixels.data()), stride,
                             reinterpret_cast<uint8_t *>(addr), (uint32_t) info.stride,
                             (uint32_t) info.width * 4,
                             (uint32_t) info.height);
      }
    }

    if (AndroidBitmap_unlockPixels(env, bitmapObj) != 0) {
      throwPixelsException(env);
      return static_cast<jobject>(nullptr);
    }

    rgbaPixels.clear();

    return bitmapObj;
  } catch (std::bad_alloc &err) {
    std::string errorString = "OOM: " + string(err.what());
    throwException(env, errorString);
    return nullptr;
  } catch (AnimatedDecoderError &err) {
    std::string errorString = err.what();
    throwException(env, errorString);
    return nullptr;
  } catch (std::runtime_error &err) {
    std::string errorString = "Error: " + string(err.what());
    throwException(env, errorString);
    return nullptr;
  }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getHeightImpl(JNIEnv *env, jobject thiz,
                                                        jlong coordinatorPtr) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  return coordinator->getHeight();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_awxkee_jxlcoder_JxlAnimatedImage_getWidthImpl(JNIEnv *env, jobject thiz,
                                                       jlong coordinatorPtr) {
  auto coordinator = reinterpret_cast<JxlAnimatedDecoderCoordinator *>(coordinatorPtr);
  return coordinator->getWidth();
}