/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 15/09/2023
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

#ifndef AVIF_RGB565_H
#define AVIF_RGB565_H

#include <cstdint>

namespace coder {

void Rgb565ToRgba16(const uint16_t *sourceData, uint32_t srcStride,
                    uint16_t *dst, uint32_t dstStride, uint16_t bitDepth,
                    uint32_t width,
                    uint32_t height, uint16_t bgColor);

void Rgb565ToUnsigned8(const uint16_t *sourceData, uint32_t srcStride,
                       uint8_t *dst, uint32_t dstStride, uint32_t width,
                       uint32_t height, uint8_t bgColor);

void Rgba8To565(const uint8_t *sourceData, uint32_t srcStride,
                uint16_t *dst, uint32_t dstStride, uint32_t width,
                uint32_t height, bool attenuateAlpha);

void RGBAF16To565(const uint16_t *sourceData, int srcStride,
                  uint16_t *dst, int dstStride, int width,
                  int height);

void Rgba16To565(const uint16_t *sourceData, uint32_t srcStride,
                 uint16_t *destination, uint32_t dstStride, uint32_t width,
                 uint32_t height, uint32_t bitDepth);
}

#endif //AVIF_RGB565_H
