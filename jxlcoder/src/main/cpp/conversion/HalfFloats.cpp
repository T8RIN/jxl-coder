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

#include "HalfFloats.h"
#include <cstdint>
#include <vector>
#include <thread>
#include "conversion/half.hpp"
#include "concurrency.hpp"
#if __aarch64__
#include <sys/auxv.h>
#include <asm/hwcap.h>
#endif

bool has_fphp() {
#if __aarch64__
  unsigned long hwcap = getauxval(AT_HWCAP);

  // Check if FPHP (Half Precision Floating Point) is supported
  return hwcap & HWCAP_ASIMDHP;  // FPHP is indicated by the HWCAP_ASIMDHP bit
#else
  return false;
#endif
}

using namespace std;

uint as_uint(const float x) {
  return *(uint *) &x;
}

float as_float(const uint x) {
  return *(float *) &x;
}

uint16_t float_to_half(
    const float x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
  const uint b =
      as_uint(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
  const uint e = (b & 0x7F800000) >> 23; // exponent
  const uint m = b &
      0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
  return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
      ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
      (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
}

float half_to_float(
    const uint16_t x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
  const uint e = (x & 0x7C00) >> 10; // exponent
  const uint m = (x & 0x03FF) << 13; // mantissa
  const uint v = as_uint((float) m)
      >> 23; // evil log2 bit hack to count leading zeros in denormalized format
  return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) *
      ((v - 37) << 23 |
          ((m << (150 - v)) &
              0x007FE000))); // sign : normalized : denormalized
}