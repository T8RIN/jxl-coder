/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 05/09/2023
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

#include "Rgb1010102.h"
#include <vector>
#include <algorithm>
#include "HalfFloats.h"
#include <thread>

using namespace std;

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Rgb1010102.cpp"

#include "hwy/foreach_target.h"
#include "hwy/highway.h"
#include "imagebit/attenuate-inl.h"
#include "algo/math-inl.h"

HWY_BEFORE_NAMESPACE();

namespace coder::HWY_NAMESPACE {

    using hwy::HWY_NAMESPACE::FixedTag;
    using hwy::HWY_NAMESPACE::Rebind;
    using hwy::HWY_NAMESPACE::Vec;
    using hwy::HWY_NAMESPACE::Set;
    using hwy::HWY_NAMESPACE::Zero;
    using hwy::HWY_NAMESPACE::LoadU;
    using hwy::HWY_NAMESPACE::Mul;
    using hwy::HWY_NAMESPACE::And;
    using hwy::HWY_NAMESPACE::Add;
    using hwy::HWY_NAMESPACE::BitCast;
    using hwy::HWY_NAMESPACE::ShiftRight;
    using hwy::HWY_NAMESPACE::ShiftLeft;
    using hwy::HWY_NAMESPACE::ExtractLane;
    using hwy::HWY_NAMESPACE::DemoteTo;
    using hwy::HWY_NAMESPACE::ConvertTo;
    using hwy::HWY_NAMESPACE::PromoteTo;
    using hwy::HWY_NAMESPACE::LoadInterleaved4;
    using hwy::HWY_NAMESPACE::Or;
    using hwy::HWY_NAMESPACE::ShiftLeft;
    using hwy::HWY_NAMESPACE::StoreU;
    using hwy::HWY_NAMESPACE::Round;
    using hwy::HWY_NAMESPACE::ClampRound;
    using hwy::float16_t;

    void
    F16ToRGBA1010102HWYRow(const uint16_t *HWY_RESTRICT data, uint32_t *HWY_RESTRICT dst,
                           int width,
                           const int *permuteMap) {
        float range10 = powf(2, 10) - 1;
        const FixedTag<float, 4> df;
        const FixedTag<float16_t, 8> df16;
        const FixedTag<uint16_t, 8> du16x8;
        const Rebind<int32_t, FixedTag<float, 4>> di32;
        const FixedTag<uint32_t, 4> du;
        using V = Vec<decltype(df)>;
        using V16 = Vec<decltype(df16)>;
        using VU16x8 = Vec<decltype(du16x8)>;
        using VU = Vec<decltype(du)>;
        const auto vRange10 = Set(df, range10);
        const auto zeros = Zero(df);
        const auto alphaMax = Set(df, 3.0);

        const Rebind<float16_t, FixedTag<uint16_t, 8>> rbfu16;
        const Rebind<float, FixedTag<float16_t, 4>> rbf32;

        const int permute0Value = permuteMap[0];
        const int permute1Value = permuteMap[1];
        const int permute2Value = permuteMap[2];
        const int permute3Value = permuteMap[3];

        int x = 0;
        auto dst32 = reinterpret_cast<uint32_t *>(dst);
        int pixels = 8;
        for (; x + pixels < width; x += pixels) {
            VU16x8 upixels1;
            VU16x8 upixels2;
            VU16x8 upixels3;
            VU16x8 upixels4;
            LoadInterleaved4(du16x8, reinterpret_cast<const uint16_t *>(data),
                             upixels1,
                             upixels2,
                             upixels3, upixels4);
            V16 fpixels1 = BitCast(rbfu16, upixels1);
            V16 fpixels2 = BitCast(rbfu16, upixels2);
            V16 fpixels3 = BitCast(rbfu16, upixels3);
            V16 fpixels4 = BitCast(rbfu16, upixels4);
            V pixelsLow1 = ClampRound(rbf32, Mul(PromoteLowerTo(rbf32, fpixels1), vRange10), zeros,
                                      vRange10);
            V pixelsLow2 = ClampRound(rbf32, Mul(PromoteLowerTo(rbf32, fpixels2), vRange10), zeros,
                                      vRange10);
            V pixelsLow3 = ClampRound(rbf32, Mul(PromoteLowerTo(rbf32, fpixels3), vRange10), zeros,
                                      vRange10);
            V pixelsLow4 = ClampRound(rbf32, Mul(PromoteLowerTo(rbf32, fpixels4), alphaMax), zeros,
                                      alphaMax);
            VU pixelsuLow1 = BitCast(du, ConvertTo(di32, pixelsLow1));
            VU pixelsuLow2 = BitCast(du, ConvertTo(di32, pixelsLow2));
            VU pixelsuLow3 = BitCast(du, ConvertTo(di32, pixelsLow3));
            VU pixelsuLow4 = BitCast(du, ConvertTo(di32, pixelsLow4));

            V pixelsHigh1 = ClampRound(rbf32, Mul(PromoteUpperTo(rbf32, fpixels1), vRange10), zeros,
                                       vRange10);
            V pixelsHigh2 = ClampRound(rbf32, Mul(PromoteUpperTo(rbf32, fpixels2), vRange10), zeros,
                                       vRange10);
            V pixelsHigh3 = ClampRound(rbf32, Mul(PromoteUpperTo(rbf32, fpixels3), vRange10), zeros,
                                       vRange10);
            V pixelsHigh4 = ClampRound(rbf32, Mul(PromoteUpperTo(rbf32, fpixels4), alphaMax), zeros,
                                       alphaMax);
            VU pixelsuHigh1 = BitCast(du, ConvertTo(di32, pixelsHigh1));
            VU pixelsuHigh2 = BitCast(du, ConvertTo(di32, pixelsHigh2));
            VU pixelsuHigh3 = BitCast(du, ConvertTo(di32, pixelsHigh3));
            VU pixelsuHigh4 = BitCast(du, ConvertTo(di32, pixelsHigh4));

            VU pixelsHighStore[4] = {pixelsuHigh1, pixelsuHigh2, pixelsuHigh3, pixelsuHigh4};
            VU AV = pixelsHighStore[permute0Value];
            VU RV = pixelsHighStore[permute1Value];
            VU GV = pixelsHighStore[permute2Value];
            VU BV = pixelsHighStore[permute3Value];
            VU upper = Or(ShiftLeft<30>(AV), ShiftLeft<20>(RV));
            VU lower = Or(ShiftLeft<10>(GV), BV);
            VU final = Or(upper, lower);
            StoreU(final, du, dst32 + 4);

            VU pixelsLowStore[4] = {pixelsuLow1, pixelsuLow2, pixelsuLow3, pixelsuLow4};
            AV = pixelsLowStore[permute0Value];
            RV = pixelsLowStore[permute1Value];
            GV = pixelsLowStore[permute2Value];
            BV = pixelsLowStore[permute3Value];
            upper = Or(ShiftLeft<30>(AV), ShiftLeft<20>(RV));
            lower = Or(ShiftLeft<10>(GV), BV);
            final = Or(upper, lower);
            StoreU(final, du, dst32);

            data += pixels * 4;
            dst32 += pixels;
        }

        for (; x < width; ++x) {
            auto A16 = (float) half_to_float(data[permute0Value]);
            auto R16 = (float) half_to_float(data[permute1Value]);
            auto G16 = (float) half_to_float(data[permute2Value]);
            auto B16 = (float) half_to_float(data[permute3Value]);
            auto R10 = (uint32_t) (clamp(round(R16 * range10), 0.0f, (float) range10));
            auto G10 = (uint32_t) (clamp(round(G16 * range10), 0.0f, (float) range10));
            auto B10 = (uint32_t) (clamp(round(B16 * range10), 0.0f, (float) range10));
            auto A10 = (uint32_t) clamp(round(A16 * 3.f), 0.0f, 3.0f);

            dst32[0] = (A10 << 30) | (R10 << 20) | (G10 << 10) | B10;

            data += 4;
            dst32 += 1;
        }
    }

    void
    F32ToRGBA1010102HWYRow(const float *HWY_RESTRICT data, uint32_t *HWY_RESTRICT dst,
                           int width,
                           const int *permuteMap) {
        float range10 = powf(2, 10) - 1;
        const FixedTag<float, 4> df;
        const Rebind<int32_t, FixedTag<float, 4>> di32;
        const FixedTag<uint32_t, 4> du;
        using V = Vec<decltype(df)>;
        using VU = Vec<decltype(du)>;
        const auto vRange10 = Set(df, range10);
        const auto zeros = Zero(df);
        const auto alphaMax = Set(df, 3.0);

        const int permute0Value = permuteMap[0];
        const int permute1Value = permuteMap[1];
        const int permute2Value = permuteMap[2];
        const int permute3Value = permuteMap[3];

        int x = 0;
        auto dst32 = reinterpret_cast<uint32_t *>(dst);
        int pixels = 4;
        for (; x + pixels < width; x += pixels) {
            V pixels1;
            V pixels2;
            V pixels3;
            V pixels4;
            LoadInterleaved4(df, reinterpret_cast<const float *>(data), pixels1, pixels2,
                             pixels3, pixels4);
            pixels1 = ClampRound(df, Mul(pixels1, vRange10), zeros,
                                 vRange10);
            pixels2 = ClampRound(df, Mul(pixels2, vRange10), zeros,
                                 vRange10);
            pixels3 = ClampRound(df, Mul(pixels3, vRange10), zeros,
                                 vRange10);
            pixels4 = ClampRound(df, Mul(pixels4, alphaMax), zeros,
                                 alphaMax);
            VU pixelsu1 = BitCast(du, ConvertTo(di32, pixels1));
            VU pixelsu2 = BitCast(du, ConvertTo(di32, pixels2));
            VU pixelsu3 = BitCast(du, ConvertTo(di32, pixels3));
            VU pixelsu4 = BitCast(du, ConvertTo(di32, pixels4));

            VU pixelsStore[4] = {pixelsu1, pixelsu2, pixelsu3, pixelsu4};
            VU AV = pixelsStore[permute0Value];
            VU RV = pixelsStore[permute1Value];
            VU GV = pixelsStore[permute2Value];
            VU BV = pixelsStore[permute3Value];
            VU upper = Or(ShiftLeft<30>(AV), ShiftLeft<20>(RV));
            VU lower = Or(ShiftLeft<10>(GV), BV);
            VU final = Or(upper, lower);
            StoreU(final, du, dst32);
            data += pixels * 4;
            dst32 += pixels;
        }

        for (; x < width; ++x) {
            auto A16 = (float) data[permute0Value];
            auto R16 = (float) data[permute1Value];
            auto G16 = (float) data[permute2Value];
            auto B16 = (float) data[permute3Value];
            auto R10 = (uint32_t) (clamp(R16 * range10, 0.0f, (float) range10));
            auto G10 = (uint32_t) (clamp(G16 * range10, 0.0f, (float) range10));
            auto B10 = (uint32_t) (clamp(B16 * range10, 0.0f, (float) range10));
            auto A10 = (uint32_t) clamp(roundf(A16 * 3.f), 0.0f, 3.0f);

            dst32[0] = (A10 << 30) | (R10 << 20) | (G10 << 10) | B10;

            data += 4;
            dst32 += 1;
        }
    }


    template<typename D, typename R, typename T = Vec<D>, typename I = Vec<R>>
    inline __attribute__((flatten)) Vec<D>
    ConvertPixelsTo(D d, R rd, I r, I g, I b, I a, const int *permuteMap) {
        using RD = Vec<R>;
        const T MulBy4Const = Set(d, 4);
        const T MulBy3Const = Set(d, 3);
        const T O127Const = Set(d, 127);
        T pixelsu1 = Mul(BitCast(d, r), MulBy4Const);
        T pixelsu2 = Mul(BitCast(d, g), MulBy4Const);
        T pixelsu3 = Mul(BitCast(d, b), MulBy4Const);
        T pixelsu4 = ShiftRight<8>(Add(Mul(BitCast(d, a), MulBy3Const), O127Const));

        const int permute0Value = permuteMap[0];
        const int permute1Value = permuteMap[1];
        const int permute2Value = permuteMap[2];
        const int permute3Value = permuteMap[3];

        T pixelsStore[4] = {pixelsu1, pixelsu2, pixelsu3, pixelsu4};
        T AV = pixelsStore[permute0Value];
        T RV = pixelsStore[permute1Value];
        T GV = pixelsStore[permute2Value];
        T BV = pixelsStore[permute3Value];
        T upper = Or(ShiftLeft<30>(AV), ShiftLeft<20>(RV));
        T lower = Or(ShiftLeft<10>(GV), BV);
        T final = Or(upper, lower);
        return final;
    }

    void
    Rgba8ToRGBA1010102HWYRow(const uint8_t *HWY_RESTRICT data, uint32_t *HWY_RESTRICT dst,
                             int width,
                             const int *permuteMap,
                             const bool attenuateAlpha) {
        const FixedTag<uint8_t, 16> du8x16;
        const FixedTag<uint16_t, 8> du16x4;
        const Rebind<int32_t, FixedTag<uint8_t, 4>> di32;
        const FixedTag<uint32_t, 4> du;
        using VU8x16 = Vec<decltype(du8x16)>;
        using VU = Vec<decltype(du)>;

        const int permute0Value = permuteMap[0];
        const int permute1Value = permuteMap[1];
        const int permute2Value = permuteMap[2];
        const int permute3Value = permuteMap[3];

        int x = 0;
        auto dst32 = reinterpret_cast<uint32_t *>(dst);
        int pixels = 16;
        for (; x + pixels < width; x += pixels) {
            VU8x16 upixels1;
            VU8x16 upixels2;
            VU8x16 upixels3;
            VU8x16 upixels4;
            LoadInterleaved4(du8x16, reinterpret_cast<const uint8_t *>(data),
                             upixels1,
                             upixels2,
                             upixels3,
                             upixels4);

            if (attenuateAlpha) {
                upixels1 = AttenuateVec(du8x16, upixels1, upixels4);
                upixels2 = AttenuateVec(du8x16, upixels2, upixels4);
                upixels3 = AttenuateVec(du8x16, upixels3, upixels4);
            }

            VU final = ConvertPixelsTo(du, di32,
                                       PromoteUpperTo(di32, PromoteUpperTo(du16x4, upixels1)),
                                       PromoteUpperTo(di32, PromoteUpperTo(du16x4, upixels2)),
                                       PromoteUpperTo(di32, PromoteUpperTo(du16x4, upixels3)),
                                       PromoteUpperTo(di32, PromoteUpperTo(du16x4, upixels4)),
                                       permuteMap);
            StoreU(final, du, dst32 + 12);

            final = ConvertPixelsTo(du, di32,
                                    PromoteLowerTo(di32, PromoteUpperTo(du16x4, upixels1)),
                                    PromoteLowerTo(di32, PromoteUpperTo(du16x4, upixels2)),
                                    PromoteLowerTo(di32, PromoteUpperTo(du16x4, upixels3)),
                                    PromoteLowerTo(di32, PromoteUpperTo(du16x4, upixels4)),
                                    permuteMap);
            StoreU(final, du, dst32 + 8);

            final = ConvertPixelsTo(du, di32,
                                    PromoteUpperTo(di32, PromoteLowerTo(du16x4, upixels1)),
                                    PromoteUpperTo(di32, PromoteLowerTo(du16x4, upixels2)),
                                    PromoteUpperTo(di32, PromoteLowerTo(du16x4, upixels3)),
                                    PromoteUpperTo(di32, PromoteLowerTo(du16x4, upixels4)),
                                    permuteMap);
            StoreU(final, du, dst32 + 4);

            final = ConvertPixelsTo(du, di32,
                                    PromoteLowerTo(di32, PromoteLowerTo(du16x4, upixels1)),
                                    PromoteLowerTo(di32, PromoteLowerTo(du16x4, upixels2)),
                                    PromoteLowerTo(di32, PromoteLowerTo(du16x4, upixels3)),
                                    PromoteLowerTo(di32, PromoteLowerTo(du16x4, upixels4)),
                                    permuteMap);
            StoreU(final, du, dst32);

            data += pixels * 4;
            dst32 += pixels;
        }

        for (; x < width; ++x) {
            uint8_t alpha = data[permute0Value];
            uint8_t r = data[permute1Value];
            uint8_t g = data[permute2Value];
            uint8_t b = data[permute3Value];

            if (attenuateAlpha) {
                r = (r * alpha + 127) / 255;
                g = (g * alpha + 127) / 255;
                b = (b * alpha + 127) / 255;
            }

            auto A16 = ((uint32_t) alpha * 3 + 127) >> 8;
            auto R16 = (uint32_t) r * 4;
            auto G16 = (uint32_t) g * 4;
            auto B16 = (uint32_t) b * 4;

            dst32[0] = (A16 << 30) | (R16 << 20) | (G16 << 10) | B16;
            data += 4;
            dst32 += 1;
        }
    }

    void
    Rgba8ToRGBA1010102HWY(const uint8_t *source,
                          const int srcStride,
                          uint8_t *destination,
                          const int dstStride,
                          int width,
                          int height,
                          const bool attenuateAlpha) {
        int permuteMap[4] = {3, 2, 1, 0};

        auto src = reinterpret_cast<const uint8_t *>(source);

        int threadCount = clamp(min(static_cast<int>(std::thread::hardware_concurrency()),
                                    width * height / (256 * 256)), 1, 12);
        vector<thread> workers;

        int segmentHeight = height / threadCount;

        for (int i = 0; i < threadCount; i++) {
            int start = i * segmentHeight;
            int end = (i + 1) * segmentHeight;
            if (i == threadCount - 1) {
                end = height;
            }
            workers.emplace_back(
                    [start, end, src, destination, srcStride, dstStride, width, permuteMap, attenuateAlpha]() {
                        for (int y = start; y < end; ++y) {
                            Rgba8ToRGBA1010102HWYRow(
                                    reinterpret_cast<const uint8_t *>(src + srcStride * y),
                                    reinterpret_cast<uint32_t *>(destination + dstStride * y),
                                    width, &permuteMap[0], attenuateAlpha);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }

    void
    F16ToRGBA1010102HWY(const uint16_t *source,
                        int srcStride,
                        uint8_t *destination,
                        int dstStride,
                        int width,
                        int height) {
        int permuteMap[4] = {3, 2, 1, 0};
        auto src = reinterpret_cast<const uint8_t *>(source);

        int threadCount = clamp(min(static_cast<int>(std::thread::hardware_concurrency()),
                                    width * height / (256 * 256)), 1, 12);
        std::vector<std::thread> workers;

        int segmentHeight = height / threadCount;

        for (int i = 0; i < threadCount; i++) {
            int start = i * segmentHeight;
            int end = (i + 1) * segmentHeight;
            if (i == threadCount - 1) {
                end = height;
            }
            workers.emplace_back(
                    [start, end, src, destination, srcStride, dstStride, width, permuteMap]() {
                        for (int y = start; y < end; ++y) {
                            F16ToRGBA1010102HWYRow(
                                    reinterpret_cast<const uint16_t *>(src + srcStride * y),
                                    reinterpret_cast<uint32_t *>(destination + dstStride * y),
                                    width, &permuteMap[0]);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }

    void
    F32ToRGBA1010102HWY(const float *source,
                        int srcStride,
                        uint8_t *destination,
                        int dstStride,
                        int width,
                        int height) {
        int permuteMap[4] = {3, 2, 1, 0};
        auto src = reinterpret_cast<const uint8_t *>(source);
        auto dst = reinterpret_cast<uint8_t *>(destination);

        int threadCount = clamp(min(static_cast<int>(thread::hardware_concurrency()),
                                    width * height / (256 * 256)), 1, 12);
        vector<thread> workers;

        int segmentHeight = height / threadCount;

        for (int i = 0; i < threadCount; i++) {
            int start = i * segmentHeight;
            int end = (i + 1) * segmentHeight;
            if (i == threadCount - 1) {
                end = height;
            }
            workers.emplace_back(
                    [start, end, src, dst, srcStride, dstStride, width, permuteMap]() {
                        for (int y = start; y < end; ++y) {
                            F32ToRGBA1010102HWYRow(
                                    reinterpret_cast<const float *>(src + srcStride * y),
                                    reinterpret_cast<uint32_t *>(dst + dstStride * y),
                                    width, &permuteMap[0]);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }

// NOLINTNEXTLINE(google-readability-namespace-comments)
}

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

#if HAVE_NEON

#include <arm_neon.h>

void convertRGBA1010102ToU8_NEON(const uint8_t *src, int srcStride, uint8_t *dst, int dstStride,
                                 int width, int height) {
    uint32x4_t mask = vdupq_n_u32(0x3FF); // Create a vector with 10 bits set
    auto maxColors = vdupq_n_u32(255);
    auto minColors = vdupq_n_u32(0);

    uint32_t testValue = 0x01020304;
    auto testBytes = reinterpret_cast<uint8_t *>(&testValue);

    bool littleEndian = false;
    if (testBytes[0] == 0x04) {
        littleEndian = true;
    } else if (testBytes[0] == 0x01) {
        littleEndian = false;
    }

    auto dstPtr = reinterpret_cast<uint8_t *>(dst);

    auto m8Bit = vdupq_n_u32(255);
    const uint32_t scalarMask = (1u << 10u) - 1u;

    for (int y = 0; y < height; ++y) {
        const uint8_t *srcPointer = src;
        uint8_t *dstPointer = dstPtr;
        int x;
        for (x = 0; x + 4 < width; x += 4) {
            uint32x4_t rgba1010102 = vld1q_u32(reinterpret_cast<const uint32_t *>(srcPointer));

            auto originalR = vshrq_n_u32(
                    vmulq_u32(vandq_u32(vshrq_n_u32(rgba1010102, 20), mask), m8Bit), 10);
            uint32x4_t r = vmaxq_u32(vminq_u32(originalR, maxColors), minColors);
            auto originalG = vshrq_n_u32(
                    vmulq_u32(vandq_u32(vshrq_n_u32(rgba1010102, 10), mask), m8Bit), 10);
            uint32x4_t g = vmaxq_u32(vminq_u32(originalG, maxColors), minColors);
            auto originalB = vshrq_n_u32(vmulq_u32(vandq_u32(rgba1010102, mask), m8Bit), 10);
            uint32x4_t b = vmaxq_u32(vminq_u32(originalB, maxColors), minColors);

            uint32x4_t a1 = vshrq_n_u32(rgba1010102, 30);

            uint32x4_t aq = vorrq_u32(vshlq_n_u32(a1, 8), vorrq_u32(vshlq_n_u32(a1, 6),
                                                                    vorrq_u32(vshlq_n_u32(a1, 4),
                                                                              vorrq_u32(
                                                                                      vshlq_n_u32(
                                                                                              a1,
                                                                                              2),
                                                                                      a1))));
            uint32x4_t a = vshrq_n_u32(vmulq_u32(aq, m8Bit), 10);

            uint8x8_t rUInt8 = vqmovn_u16(vqmovn_high_u32(vqmovn_u32(r), r));
            uint8x8_t gUInt8 = vqmovn_u16(vqmovn_high_u32(vqmovn_u32(g), g));
            uint8x8_t bUInt8 = vqmovn_u16(vqmovn_high_u32(vqmovn_u32(b), b));
            uint8x8_t aUInt8 = vqmovn_u16(vqmovn_high_u32(vqmovn_u32(a), a));

            uint8x8x4_t resultRGBA;
            // Interleave the channels to get RGBA format
            resultRGBA.val[0] = vzip_u8(bUInt8, bUInt8).val[0];
            resultRGBA.val[1] = vzip_u8(gUInt8, gUInt8).val[0];
            resultRGBA.val[2] = vzip_u8(rUInt8, rUInt8).val[1];
            resultRGBA.val[3] = vzip_u8(aUInt8, aUInt8).val[1];

            vst4_u8(reinterpret_cast<uint8_t *>(dstPointer), resultRGBA);

            srcPointer += 16;
            dstPointer += 16;
        }

        for (; x < width; ++x) {
            uint32_t rgba1010102 = reinterpret_cast<const uint32_t *>(srcPointer)[0];

            uint32_t b = (rgba1010102) & scalarMask;
            uint32_t g = (rgba1010102 >> 10) & scalarMask;
            uint32_t r = (rgba1010102 >> 20) & scalarMask;

            uint32_t a1 = (rgba1010102 >> 30);
            uint32_t a = (a1 << 8) | (a1 << 6) | (a1 << 4) | (a1 << 2) | a1;

            // Convert each channel to floating-point values
            auto rUInt16 = static_cast<uint16_t>(r);
            auto gUInt16 = static_cast<uint16_t>(g);
            auto bUInt16 = static_cast<uint16_t>(b);
            auto aUInt16 = static_cast<uint16_t>(a);

            auto dstCast = reinterpret_cast<uint16_t *>(dstPointer);

            if (littleEndian) {
                dstCast[0] = bUInt16;
                dstCast[1] = gUInt16;
                dstCast[2] = rUInt16;
                dstCast[3] = aUInt16;
            } else {
                dstCast[0] = rUInt16;
                dstCast[1] = gUInt16;
                dstCast[2] = bUInt16;
                dstCast[3] = aUInt16;
            }

            srcPointer += 4;
            dstPointer += 4;
        }

        src += srcStride;
        dstPtr += dstStride;
    }
}

void convertRGBA1010102ToU16_NEON(const uint8_t *src, int srcStride, uint16_t *dst, int dstStride,
                                  int width, int height) {
    uint32x4_t mask = vdupq_n_u32(0x3FF); // Create a vector with 10 bits set
    auto maxColors = vdupq_n_u32(1023);
    auto minColors = vdupq_n_u32(0);

    uint32_t testValue = 0x01020304;
    auto testBytes = reinterpret_cast<uint8_t *>(&testValue);

    bool littleEndian = false;
    if (testBytes[0] == 0x04) {
        littleEndian = true;
    } else if (testBytes[0] == 0x01) {
        littleEndian = false;
    }

    const uint32_t scalarMask = (1u << 10u) - 1u;

    auto dstPtr = reinterpret_cast<uint8_t *>(dst);

    constexpr float valueScale = 255.0f / 1023.0;

    for (int y = 0; y < height; ++y) {
        const uint8_t *srcPointer = src;
        uint8_t *dstPointer = dstPtr;
        int x;
        for (x = 0; x + 2 < width; x += 2) {
            uint32x4_t rgba1010102 = vld1q_u32(reinterpret_cast<const uint32_t *>(srcPointer));

            uint32x4_t r = vmaxq_u32(
                    vminq_u32(vandq_u32(vshrq_n_u32(rgba1010102, 20), mask), maxColors),
                    minColors);
            uint32x4_t g = vmaxq_u32(
                    vminq_u32(vandq_u32(vshrq_n_u32(rgba1010102, 10), mask), maxColors),
                    minColors);
            uint32x4_t b = vmaxq_u32(vminq_u32(vandq_u32(rgba1010102, mask), maxColors),
                                     minColors);

            uint32x4_t a1 = vshrq_n_u32(rgba1010102, 30);

            uint32x4_t a = vorrq_u32(vshlq_n_u32(a1, 8), vorrq_u32(vshlq_n_u32(a1, 6),
                                                                   vorrq_u32(vshlq_n_u32(a1, 4),
                                                                             vorrq_u32(
                                                                                     vshlq_n_u32(a1,
                                                                                                 2),
                                                                                     a1))));

            uint16x4_t rUInt16 = vqmovn_u32(b);
            uint16x4_t gUInt16 = vqmovn_u32(g);
            uint16x4_t bUInt16 = vqmovn_u32(r);
            uint16x4_t aUInt16 = vqmovn_u32(vmaxq_u32(vminq_u32(a, maxColors), minColors));

            uint16x4x4_t interleavedRGBA;
            interleavedRGBA.val[0] = rUInt16;
            interleavedRGBA.val[1] = gUInt16;
            interleavedRGBA.val[2] = bUInt16;
            interleavedRGBA.val[3] = aUInt16;

            vst4_u16(reinterpret_cast<uint16_t *>(dstPointer), interleavedRGBA);

            srcPointer += 8;
            dstPointer += 16;
        }

        for (; x < width; ++x) {
            uint32_t rgba1010102 = reinterpret_cast<const uint32_t *>(srcPointer)[0];
            uint32_t b = (rgba1010102) & scalarMask;
            uint32_t g = (rgba1010102 >> 10) & scalarMask;
            uint32_t r = (rgba1010102 >> 20) & scalarMask;

            uint32_t a1 = (rgba1010102 >> 30);
            uint32_t a = (a1 << 8) | (a1 << 6) | (a1 << 4) | (a1 << 2) | a1;

            auto rFloat = clamp(static_cast<uint8_t>(r * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto gFloat = clamp(static_cast<uint8_t>(g * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto bFloat = clamp(static_cast<uint8_t>(b * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto aFloat = clamp(static_cast<uint8_t>(a * valueScale), (uint8_t) 255,
                                (uint8_t) 0);

            auto dstCast = reinterpret_cast<uint8_t *>(dstPointer);
            if (littleEndian) {
                dstCast[0] = bFloat;
                dstCast[1] = gFloat;
                dstCast[2] = rFloat;
                dstCast[3] = aFloat;
            } else {
                dstCast[0] = rFloat;
                dstCast[1] = gFloat;
                dstCast[2] = bFloat;
                dstCast[3] = aFloat;
            }

            srcPointer += 4;
            dstPointer += 4;
        }


        src += srcStride;
        dstPtr += dstStride;
    }
}

#endif

void convertRGBA1010102ToU16_C(const uint8_t *src, int srcStride, uint16_t *dst, int dstStride,
                               int width, int height) {
    auto mDstPointer = reinterpret_cast<uint8_t *>(dst);
    auto mSrcPointer = reinterpret_cast<const uint8_t *>(src);

    uint32_t testValue = 0x01020304;
    auto testBytes = reinterpret_cast<uint8_t *>(&testValue);

    bool littleEndian = false;
    if (testBytes[0] == 0x04) {
        littleEndian = true;
    } else if (testBytes[0] == 0x01) {
        littleEndian = false;
    }

    for (int y = 0; y < height; ++y) {

        auto dstPointer = reinterpret_cast<uint8_t *>(mDstPointer);
        auto srcPointer = reinterpret_cast<const uint8_t *>(mSrcPointer);

        for (int x = 0; x < width; ++x) {
            uint32_t rgba1010102 = reinterpret_cast<const uint32_t *>(srcPointer)[0];

            const uint32_t mask = (1u << 10u) - 1u;
            uint32_t b = (rgba1010102) & mask;
            uint32_t g = (rgba1010102 >> 10) & mask;
            uint32_t r = (rgba1010102 >> 20) & mask;

            uint32_t a1 = (rgba1010102 >> 30);
            uint32_t a = (a1 << 8) | (a1 << 6) | (a1 << 4) | (a1 << 2) | a1;

            auto rUInt16 = static_cast<uint16_t>(r);
            auto gUInt16 = static_cast<uint16_t>(g);
            auto bUInt16 = static_cast<uint16_t>(b);
            auto aUInt16 = static_cast<uint16_t>(a);

            auto dstCast = reinterpret_cast<uint16_t *>(dstPointer);

            if (littleEndian) {
                dstCast[0] = bUInt16;
                dstCast[1] = gUInt16;
                dstCast[2] = rUInt16;
                dstCast[3] = aUInt16;
            } else {
                dstCast[0] = rUInt16;
                dstCast[1] = gUInt16;
                dstCast[2] = bUInt16;
                dstCast[3] = aUInt16;
            }

            srcPointer += 4;
            dstPointer += 8;
        }

        mSrcPointer += srcStride;
        mDstPointer += dstStride;
    }
}

void convertRGBA1010102ToU8_C(const uint8_t *src, int srcStride, uint8_t *dst, int dstStride,
                              int width, int height) {
    auto mDstPointer = reinterpret_cast<uint8_t *>(dst);
    auto mSrcPointer = reinterpret_cast<const uint8_t *>(src);

    uint32_t testValue = 0x01020304;
    auto testBytes = reinterpret_cast<uint8_t *>(&testValue);

    bool littleEndian = false;
    if (testBytes[0] == 0x04) {
        littleEndian = true;
    } else if (testBytes[0] == 0x01) {
        littleEndian = false;
    }

    const uint32_t mask = (1u << 10u) - 1u;

    constexpr float valueScale = 255.0f / 1023.0;

    for (int y = 0; y < height; ++y) {

        auto dstPointer = reinterpret_cast<uint8_t *>(mDstPointer);
        auto srcPointer = reinterpret_cast<const uint8_t *>(mSrcPointer);

        for (int x = 0; x < width; ++x) {
            uint32_t rgba1010102 = reinterpret_cast<const uint32_t *>(srcPointer)[0];

            uint32_t b = (rgba1010102) & mask;
            uint32_t g = (rgba1010102 >> 10) & mask;
            uint32_t r = (rgba1010102 >> 20) & mask;

            uint32_t a1 = (rgba1010102 >> 30);
            uint32_t a = (a1 << 8) | (a1 << 6) | (a1 << 4) | (a1 << 2) | a1;

            auto rFloat = clamp(static_cast<uint8_t>(r * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto gFloat = clamp(static_cast<uint8_t>(g * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto bFloat = clamp(static_cast<uint8_t>(b * valueScale), (uint8_t) 255,
                                (uint8_t) 0);
            auto aFloat = clamp(static_cast<uint8_t>(a * valueScale), (uint8_t) 255,
                                (uint8_t) 0);

            auto dstCast = reinterpret_cast<uint8_t *>(dstPointer);
            if (littleEndian) {
                dstCast[0] = bFloat;
                dstCast[1] = gFloat;
                dstCast[2] = rFloat;
                dstCast[3] = aFloat;
            } else {
                dstCast[0] = rFloat;
                dstCast[1] = gFloat;
                dstCast[2] = bFloat;
                dstCast[3] = aFloat;
            }

            srcPointer += 4;
            dstPointer += 4;
        }

        mSrcPointer += srcStride;
        mDstPointer += dstStride;
    }
}

void RGBA1010102ToU8(const uint8_t *src, int srcStride, uint8_t *dst, int dstStride,
                     int width, int height) {
#if HAVE_NEON
    convertRGBA1010102ToU8_NEON(src, srcStride, dst, dstStride, width, height);
#else
    convertRGBA1010102ToU8_C(src, srcStride, dst, dstStride, width, height);
#endif
}

void RGBA1010102ToU16(const uint8_t *src, int srcStride, uint16_t *dst, int dstStride,
                      int width, int height) {
#if HAVE_NEON
    convertRGBA1010102ToU16_NEON(src, srcStride, dst, dstStride, width, height);
#else
    convertRGBA1010102ToU16_C(src, srcStride, dst, dstStride, width, height);
#endif
}

namespace coder {
    HWY_EXPORT(F16ToRGBA1010102HWY);
    HWY_EXPORT(F32ToRGBA1010102HWY);
    HWY_EXPORT(Rgba8ToRGBA1010102HWY);

    HWY_DLLEXPORT void
    F16ToRGBA1010102(const uint16_t *source, int srcStride, uint8_t *destination, int dstStride,
                     int width,
                     int height) {
        HWY_DYNAMIC_DISPATCH(F16ToRGBA1010102HWY)(source, srcStride, destination, dstStride, width,
                                                  height);
    }

    HWY_DLLEXPORT void
    F32ToRGBA1010102(const float *source, int srcStride, uint8_t *destination, int dstStride,
                     int width,
                     int height) {
        HWY_DYNAMIC_DISPATCH(F32ToRGBA1010102HWY)(source, srcStride, destination, dstStride, width,
                                                  height);
    }

    HWY_DLLEXPORT void
    Rgba8ToRGBA1010102(const uint8_t *source,
                       const int srcStride,
                       uint8_t *destination,
                       const int dstStride,
                       int width,
                       int height, const bool attenuateAlpha) {
        HWY_DYNAMIC_DISPATCH(Rgba8ToRGBA1010102HWY)(source, srcStride, destination, dstStride,
                                                    width,
                                                    height, attenuateAlpha);
    }

}

#endif