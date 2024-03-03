/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 04/09/2023
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

#ifndef JXLCODER_RGBA2RGB_H
#define JXLCODER_RGBA2RGB_H

#include <cstdint>

namespace coder {
    void rgb8bit2RGB(const uint8_t *src, int srcStride, uint8_t *dst, int dstStride, int height,
                     int width);

    void Rgba16bit2RGB(const uint16_t *src, int srcStride, uint16_t *dst, int dstStride, int height,
                       int width);
}

#if HAVE_NEON
void rgba8bit2RgbNEON(const uint8_t* src, uint8_t* dst, int numPixels);
void rgba16Bit2RgbNEON(const uint16_t* src, int srcStride, uint16_t* dst, int dstStride, int height, int width);
#endif

#endif //JXLCODER_RGBA2RGB_H
