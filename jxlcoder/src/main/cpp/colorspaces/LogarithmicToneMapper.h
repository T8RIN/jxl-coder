/*
 * MIT License
 *
 * Copyright (c) 2024 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 13/10/2024
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

#ifndef AVIF_LOGARITHMICTONEMAPPER_H
#define AVIF_LOGARITHMICTONEMAPPER_H

#include <vector>

class LogarithmicToneMapper {
public:
    LogarithmicToneMapper(const float primaries[3]) {
        std::copy(primaries, primaries + 3, lumaPrimaries);

        float Lmax = 1;
        float exposure = 1.f;
        den = static_cast<float>(1) / log(static_cast<float>(1 + Lmax * exposure));
    }

    void transferTone(float *inPlace, uint32_t width) const;

private:
    float lumaPrimaries[3] = {0};
    float den;
};


#endif //AVIF_LOGARITHMICTONEMAPPER_H
