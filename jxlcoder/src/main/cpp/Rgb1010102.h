//
// Created by Radzivon Bartoshyk on 05/09/2023.
//

#ifndef AVIF_RGB1010102_H
#define AVIF_RGB1010102_H

#include <vector>

void RGBA1010102ToU16(const uint8_t *src, int srcStride, uint16_t *dst, int dstStride,
                      int width, int height);

void RGBA1010102ToU8(const uint8_t *src, int srcStride, uint8_t *dst, int dstStride,
                     int width, int height);

namespace coder {
    void
    F16ToRGBA1010102(const uint16_t *source, int srcStride, uint8_t *destination, int dstStride,
                     int width,
                     int height);

    void F32ToRGBA1010102(const float *source, int srcStride, uint8_t *destination, int dstStride,
                          int width,
                          int height);

    void
    Rgba8ToRGBA1010102(const uint8_t *source,
                       int srcStride,
                       uint8_t *destination,
                       int dstStride,
                       int width,
                       int height);
}

#endif //AVIF_RGB1010102_H
