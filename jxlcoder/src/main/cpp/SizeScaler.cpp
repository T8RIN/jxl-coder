//
// Created by Radzivon Bartoshyk on 01/01/2023.
//

#include "SizeScaler.h"
#include <vector>
#include <string>
#include <jni.h>
#include "JniExceptions.h"
#include "stb_image_resize.h"
#include "libyuv.h"

bool RescaleImage(std::vector<uint8_t> &rgbaData,
                  JNIEnv *env,
                  int *stride,
                  bool useFloats,
                  int *imageWidthPtr, int *imageHeightPtr,
                  int scaledWidth, int scaledHeight,
                  bool alphaPremultiplied,
                  ScaleMode scaleMode) {
    int imageWidth = *imageWidthPtr;
    int imageHeight = *imageHeightPtr;
    if ((scaledHeight != 0 || scaledWidth != 0) && (scaledWidth != 0 && scaledHeight != 0)) {

        int xTranslation = 0, yTranslation = 0;
        int canvasWidth = scaledWidth;
        int canvasHeight = scaledHeight;

        if (scaleMode == Fit || scaleMode == Fill) {
            std::pair<int, int> currentSize(imageWidth, imageHeight);
            if (scaledHeight > 0 && scaledWidth < 0) {
                auto newBounds = ResizeAspectHeight(currentSize, scaledHeight,
                                                    scaledWidth == -2);
                scaledWidth = newBounds.first;
                scaledHeight = newBounds.second;
            } else if (scaledHeight < 0) {
                auto newBounds = ResizeAspectWidth(currentSize, scaledHeight,
                                                   scaledHeight == -2);
                scaledWidth = newBounds.first;
                scaledHeight = newBounds.second;
            } else {
                std::pair<int, int> dstSize;
                float scale = 1;
                if (scaleMode == Fill) {
                    std::pair<int, int> canvasSize(scaledWidth, scaledHeight);
                    dstSize = ResizeAspectFill(currentSize, canvasSize, &scale);
                } else {
                    std::pair<int, int> canvasSize(scaledWidth, scaledHeight);
                    dstSize = ResizeAspectFit(currentSize, canvasSize, &scale);
                }

                xTranslation = std::max((int) (((float) dstSize.first - (float) canvasWidth) /
                                               2.0f), 0);
                yTranslation = std::max((int) (((float) dstSize.second - (float) canvasHeight) /
                                               2.0f), 0);

                scaledWidth = dstSize.first;
                scaledHeight = dstSize.second;
            }
        }

        int imdStride = scaledWidth * 4 * (int) (useFloats ? sizeof(float) : sizeof(uint8_t));
        std::vector<uint8_t> newImageData(imdStride * scaledHeight);

        if (useFloats) {
            int result = stbir_resize_float_generic(
                    reinterpret_cast<const float *>(rgbaData.data()), (int) imageWidth,
                    (int) imageHeight, 0,
                    reinterpret_cast<float *>(newImageData.data()), scaledWidth,
                    scaledHeight, 0,
                    4, 3, alphaPremultiplied ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0,
                    STBIR_EDGE_CLAMP, STBIR_FILTER_CUBICBSPLINE, STBIR_COLORSPACE_SRGB,
                    nullptr
            );
            if (result != 1) {
                std::string s("Failed to resample an image");
                throwException(env, s);
                return static_cast<jobject>(nullptr);
            }
        } else {
            libyuv::ARGBScale(rgbaData.data(), static_cast<int>(imageWidth * 4),
                              static_cast<int>(imageWidth),
                              static_cast<int>(imageHeight),
                              newImageData.data(), scaledWidth * 4, scaledWidth, scaledHeight,
                              libyuv::kFilterBilinear);
        }

        imageWidth = scaledWidth;
        imageHeight = scaledHeight;

        if (xTranslation > 0 || yTranslation > 0) {

            int left = std::max(xTranslation, 0);
            int right = xTranslation + canvasWidth;
            int top = std::max(yTranslation, 0);
            int bottom = yTranslation + canvasHeight;

            int croppedWidth = right - left;
            int croppedHeight = bottom - top;
            int newStride =
                    croppedWidth * 4 * (int) (useFloats ? sizeof(float ) : sizeof(uint8_t));
            int srcStride = imdStride;

            std::vector<uint8_t> croppedImage(newStride * croppedHeight);

            uint8_t *dstData = croppedImage.data();
            auto srcData = reinterpret_cast<const uint8_t *>(newImageData.data());

            for (int y = top, yc = 0; y < bottom; ++y, ++yc) {
                int x = 0;
                int xc = 0;
                auto srcRow = reinterpret_cast<const uint8_t *>(srcData + srcStride * y);
                auto dstRow = reinterpret_cast<uint8_t *>(dstData + newStride * yc);
                for (x = left, xc = 0; x < right; ++x, ++xc) {
                    if (useFloats) {
                        auto dst = reinterpret_cast<float *>(dstRow + xc * 4 * sizeof(float ));
                        auto src = reinterpret_cast<const float *>(srcRow + x * 4 * sizeof(float ));
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = src[3];
                    } else {
                        auto dst = reinterpret_cast<uint32_t *>(dstRow);
                        auto src = reinterpret_cast<const uint32_t *>(srcRow);
                        dst[xc] = src[x];
                    }
                }
            }

            imageWidth = croppedWidth;
            imageHeight = croppedHeight;

            rgbaData = croppedImage;
            *stride = newStride;

        } else {
            rgbaData = newImageData;
            *stride = imdStride;
        }

        *imageWidthPtr = imageWidth;
        *imageHeightPtr = imageHeight;

    }
    return true;
}

std::pair<int, int>
ResizeAspectFit(std::pair<int, int> sourceSize, std::pair<int, int> dstSize, float *scale) {
    int sourceWidth = sourceSize.first;
    int sourceHeight = sourceSize.second;
    float xFactor = (float) dstSize.first / (float) sourceSize.first;
    float yFactor = (float) dstSize.second / (float) sourceSize.second;
    float resizeFactor = std::min(xFactor, yFactor);
    *scale = resizeFactor;
    std::pair<int, int> resultSize((int) ((float) sourceWidth * resizeFactor),
                                   (int) ((float) sourceHeight * resizeFactor));
    return resultSize;
}

std::pair<int, int>
ResizeAspectFill(std::pair<int, int> sourceSize, std::pair<int, int> dstSize, float *scale) {
    int sourceWidth = sourceSize.first;
    int sourceHeight = sourceSize.second;
    float xFactor = (float) dstSize.first / (float) sourceSize.first;
    float yFactor = (float) dstSize.second / (float) sourceSize.second;
    float resizeFactor = std::max(xFactor, yFactor);
    *scale = resizeFactor;
    std::pair<int, int> resultSize((int) ((float) sourceWidth * resizeFactor),
                                   (int) ((float) sourceHeight * resizeFactor));
    return resultSize;
}

std::pair<int, int>
ResizeAspectHeight(std::pair<int, int> sourceSize, int maxHeight, bool multipleBy2) {
    int sourceWidth = sourceSize.first;
    int sourceHeight = sourceSize.second;
    float scaleFactor = (float) maxHeight / (float) sourceSize.second;
    std::pair<int, int> resultSize((int) ((float) sourceWidth * scaleFactor),
                                   (int) ((float) sourceHeight * scaleFactor));
    if (multipleBy2) {
        resultSize.first = (resultSize.first / 2) * 2;
        resultSize.second = (resultSize.second / 2) * 2;
    }
    return resultSize;
}

std::pair<int, int>
ResizeAspectWidth(std::pair<int, int> sourceSize, int maxWidth, bool multipleBy2) {
    int sourceWidth = sourceSize.first;
    int sourceHeight = sourceSize.second;
    float scaleFactor = (float) maxWidth / (float) sourceSize.first;
    std::pair<int, int> resultSize((int) ((float) sourceWidth * scaleFactor),
                                   (int) ((float) sourceHeight * scaleFactor));
    if (multipleBy2) {
        resultSize.first = (resultSize.first / 2) * 2;
        resultSize.second = (resultSize.second / 2) * 2;
    }
    return resultSize;
}