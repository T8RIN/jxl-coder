#include <jni.h>
#include <android/bitmap.h>
#include <vector>
#include "JniExceptions.h"
#include "XScaler.h"
#include "Rgb1010102toF16.h"
#include "RGBAlpha.h"
#include "libyuv/convert_argb.h"
#include "libyuv/convert_from_argb.h"
#include "CopyUnaligned.h"

using namespace std;

extern "C"
JNIEXPORT jobject JNICALL
Java_com_t8rin_bitmapscaler_BitmapScaler_scaleImpl(JNIEnv *env, jobject thiz, jobject bitmap,
                                                   jint dst_width, jint dst_height,
                                                   jint scale_mode) {
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        throwPixelsException(env);
        return static_cast<jbyteArray>(nullptr);
    }

    void *addr;
    if (AndroidBitmap_lockPixels(env, bitmap, &addr) != 0) {
        throwPixelsException(env);
        return static_cast<jbyteArray>(nullptr);
    }

    vector<uint8_t> rgbaPixels(info.stride * info.height);
    memcpy(rgbaPixels.data(), addr, info.stride * info.height);

    if (AndroidBitmap_unlockPixels(env, bitmap) != 0) {
        string exc = "Unlocking pixels has failed";
        throwException(env, exc);
        return static_cast<jbyteArray>(nullptr);
    }

    int imageStride = (int) info.stride;
    int dstStride = (int) info.stride;

    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_1010102) {
        imageStride = (int) info.width * 4 * (int) sizeof(uint16_t);
        dstStride = (int) dst_width * 4 * (int) sizeof(uint16_t);
        vector<uint8_t> halfFloatPixels(imageStride * info.height);
        coder::ConvertRGBA1010102toF16(reinterpret_cast<const uint8_t *>(rgbaPixels.data()),
                                       (int) info.stride,
                                       reinterpret_cast<uint16_t *>(halfFloatPixels.data()),
                                       (int) imageStride,
                                       (int) info.width,
                                       (int) info.height);
        rgbaPixels = halfFloatPixels;
    } else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
        dstStride = (int) dst_width * 4 * (int) sizeof(uint8_t);
        int newStride = (int) info.width * 4 * (int) sizeof(uint8_t);
        std::vector<uint8_t> rgba8888Pixels(newStride * info.height);
        libyuv::RGB565ToARGB(rgbaPixels.data(), (int) info.stride,
                             rgba8888Pixels.data(), newStride,
                             (int) info.width, (int) info.height);
        libyuv::ARGBToABGR(rgba8888Pixels.data(), newStride,
                           rgba8888Pixels.data(), newStride,
                           (int) info.width, (int) info.height);
        imageStride = newStride;
        rgbaPixels = rgba8888Pixels;
    } else if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        coder::UnpremultiplyRGBA(rgbaPixels.data(), imageStride,
                                 rgbaPixels.data(), imageStride,
                                 (int) info.width,
                                 (int) info.height);
    }

    bool useFloat16 = info.format == ANDROID_BITMAP_FORMAT_RGBA_F16 ||
                      info.format == ANDROID_BITMAP_FORMAT_RGBA_1010102;

    std::vector<uint8_t> rgbPixels;
    int requiredStride = (int) info.width * 4 *
                         (int) (useFloat16 ? sizeof(uint16_t) : sizeof(uint8_t));

    if (requiredStride == info.stride) {
        rgbPixels = rgbaPixels;
    } else {
        rgbPixels.resize(requiredStride * (int) info.height);
        coder::CopyUnalignedRGBA(rgbaPixels.data(), imageStride, rgbPixels.data(),
                                 requiredStride, (int) info.width, (int) info.height,
                                 (int) (useFloat16 ? sizeof(uint16_t)
                                                   : sizeof(uint8_t)));
    }
    imageStride = requiredStride;

    vector<uint8_t> output(dstStride * dst_height);


    if (useFloat16) {
        scaleImageFloat16(
                reinterpret_cast<uint16_t *>(rgbPixels.data()),
                imageStride,
                info.width,
                info.height,
                reinterpret_cast<uint16_t *>(output.data()),
                dstStride,
                dst_width,
                dst_height,
                4,
                static_cast<XSampler>(scale_mode)
        );
    } else {
        scaleImageU8(
                rgbPixels.data(),
                imageStride,
                info.width,
                info.height,
                output.data(),
                dstStride,
                dst_width,
                dst_height,
                4,
                8,
                static_cast<XSampler>(scale_mode)
        );
    }

    rgbPixels.clear();

    std::string bitmapPixelConfig = useFloat16 ? "RGBA_F16" : "ARGB_8888";
    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgba8888FieldID = env->GetStaticFieldID(bitmapConfig,
                                                     bitmapPixelConfig.c_str(),
                                                     "Landroid/graphics/Bitmap$Config;");
    jobject rgba8888Obj = env->GetStaticObjectField(bitmapConfig, rgba8888FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jobject bitmapObj = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID,
                                                    static_cast<jint>(dst_width),
                                                    static_cast<jint>(dst_height),
                                                    rgba8888Obj);


    if (AndroidBitmap_getInfo(env, bitmapObj, &info) < 0) {
        throwPixelsException(env);
        return static_cast<jbyteArray>(nullptr);
    }

    addr = nullptr;

    if (AndroidBitmap_lockPixels(env, bitmapObj, &addr) != 0) {
        throwPixelsException(env);
        return static_cast<jobject>(nullptr);
    }

    if (bitmapPixelConfig == "RGB_565") {
        coder::CopyUnalignedRGB565(reinterpret_cast<const uint8_t *>(output.data()), dstStride,
                                   reinterpret_cast<uint8_t *>(addr), (int) info.stride,
                                   (int) info.width,
                                   (int) info.height);
    } else {
        coder::CopyUnalignedRGBA(reinterpret_cast<const uint8_t *>(output.data()), dstStride,
                                 reinterpret_cast<uint8_t *>(addr), (int) info.stride,
                                 (int) info.width,
                                 (int) info.height, useFloat16 ? 2 : 1);
    }


    if (AndroidBitmap_unlockPixels(env, bitmapObj) != 0) {
        throwPixelsException(env);
        return static_cast<jobject>(nullptr);
    }

    rgbaPixels.clear();

    return bitmapObj;
}