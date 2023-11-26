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

#include "Rgb565.h"
#include "HalfFloats.h"
#include <thread>

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Rgb565.cpp"

#include "hwy/foreach_target.h"
#include "hwy/highway.h"

using namespace std;

HWY_BEFORE_NAMESPACE();

namespace coder::HWY_NAMESPACE {

    using hwy::HWY_NAMESPACE::Set;
    using hwy::HWY_NAMESPACE::FixedTag;
    using hwy::HWY_NAMESPACE::Vec;
    using hwy::HWY_NAMESPACE::Mul;
    using hwy::HWY_NAMESPACE::Max;
    using hwy::HWY_NAMESPACE::Min;
    using hwy::HWY_NAMESPACE::Zero;
    using hwy::HWY_NAMESPACE::BitCast;
    using hwy::HWY_NAMESPACE::ConvertTo;
    using hwy::HWY_NAMESPACE::PromoteTo;
    using hwy::HWY_NAMESPACE::DemoteTo;
    using hwy::HWY_NAMESPACE::Combine;
    using hwy::HWY_NAMESPACE::Rebind;
    using hwy::HWY_NAMESPACE::LowerHalf;
    using hwy::HWY_NAMESPACE::ShiftLeft;
    using hwy::HWY_NAMESPACE::ShiftRight;
    using hwy::HWY_NAMESPACE::UpperHalf;
    using hwy::HWY_NAMESPACE::LoadInterleaved4;
    using hwy::HWY_NAMESPACE::StoreU;
    using hwy::HWY_NAMESPACE::Or;
    using hwy::float16_t;
    using hwy::float32_t;

    void
    Rgba8To565HWYRow(const uint8_t *source, uint16_t *destination, int width) {
        const FixedTag<uint16_t, 8> du16;
        const FixedTag<uint8_t, 8> du8x8;
        using VU16 = Vec<decltype(du16)>;
        using VU8x8 = Vec<decltype(du8x8)>;

        Rebind<uint16_t, FixedTag<uint8_t, 8>> rdu16;

        int x = 0;
        int pixels = 8;

        auto src = reinterpret_cast<const uint8_t *>(source);
        auto dst = reinterpret_cast<uint16_t *>(destination);
        for (x = 0; x + pixels < width; x += pixels) {
            VU8x8 ru8Row;
            VU8x8 gu8Row;
            VU8x8 bu8Row;
            VU8x8 au8Row;
            LoadInterleaved4(du8x8, reinterpret_cast<const uint8_t *>(src),
                             ru8Row, gu8Row, bu8Row, au8Row);

            auto rdu16Vec = ShiftLeft<11>(ShiftRight<3>(PromoteTo(rdu16, ru8Row)));
            auto gdu16Vec = ShiftLeft<5>(ShiftRight<2>(PromoteTo(rdu16, gu8Row)));
            auto bdu16Vec = ShiftRight<3>(PromoteTo(rdu16, bu8Row));

            auto result = Or(Or(rdu16Vec, gdu16Vec), bdu16Vec);
            StoreU(result, du16, dst);
            src += 4 * pixels;
            dst += pixels;
        }

        for (; x < width; ++x) {
            uint16_t red565 = (src[0] >> 3) << 11;
            uint16_t green565 = (src[1] >> 2) << 5;
            uint16_t blue565 = src[2] >> 3;

            auto result = static_cast<uint16_t>(red565 | green565 | blue565);
            dst[0] = result;

            src += 4;
            dst += 1;
        }
    }

    void Rgba8To565HWY(const uint8_t *sourceData, const int srcStride,
                       uint16_t *dst, const int dstStride, const int width,
                       const int height, const int bitDepth) {

        auto mSrc = reinterpret_cast<const uint8_t *>(sourceData);
        auto mDst = reinterpret_cast<uint8_t *>(dst);

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
                    [start, end, mSrc, mDst, srcStride, dstStride, width]() {
                        for (int y = start; y < end; ++y) {
                            Rgba8To565HWYRow(
                                    reinterpret_cast<const uint8_t *>(mSrc + srcStride * y),
                                    reinterpret_cast<uint16_t *>(mDst + dstStride * y),
                                    width);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }

    inline Vec<FixedTag<uint8_t, 8>>
    ConvertF16ToU16Row(Vec<FixedTag<uint16_t, 8>> v, float maxColors) {
        FixedTag<float16_t, 4> df16;
        FixedTag<uint16_t, 4> dfu416;
        FixedTag<uint8_t, 8> du8;
        Rebind<float, decltype(df16)> rf32;
        Rebind<int32_t, decltype(rf32)> ri32;
        Rebind<uint8_t, decltype(rf32)> ru8;

        using VU8 = Vec<decltype(du8)>;

        auto minColors = Zero(rf32);
        auto vMaxColors = Set(rf32, (int) maxColors);

        auto lower = DemoteTo(ru8, ConvertTo(ri32,
                                             Max(Min(Mul(
                                                     PromoteTo(rf32, BitCast(df16, LowerHalf(v))),
                                                     vMaxColors), vMaxColors), minColors)
        ));
        auto upper = DemoteTo(ru8, ConvertTo(ri32,
                                             Max(Min(Mul(PromoteTo(rf32,
                                                                   BitCast(df16,
                                                                           UpperHalf(dfu416, v))),
                                                         vMaxColors), vMaxColors), minColors)
        ));
        return Combine(du8, upper, lower);
    }

    void
    RGBAF16To565RowHWY(const uint16_t *source, uint16_t *destination, int width,
                       float maxColors) {
        const FixedTag<uint16_t, 8> du16;
        const FixedTag<uint8_t, 8> du8;
        using VU16 = Vec<decltype(du16)>;
        using VU8 = Vec<decltype(du8)>;

        Rebind<uint16_t, FixedTag<uint8_t, 8>> rdu16;

        int x = 0;
        int pixels = 8;

        auto src = reinterpret_cast<const uint16_t *>(source);
        auto dst = reinterpret_cast<uint16_t *>(destination);
        for (x = 0; x + pixels < width; x += pixels) {
            VU16 ru16Row;
            VU16 gu16Row;
            VU16 bu16Row;
            VU16 au16Row;
            LoadInterleaved4(du16, reinterpret_cast<const uint16_t *>(src),
                             ru16Row, gu16Row,
                             bu16Row,
                             au16Row);

            auto r16Row = ConvertF16ToU16Row(ru16Row, maxColors);
            auto g16Row = ConvertF16ToU16Row(gu16Row, maxColors);
            auto b16Row = ConvertF16ToU16Row(bu16Row, maxColors);

            auto rdu16Vec = ShiftLeft<11>(ShiftRight<3>(PromoteTo(rdu16, r16Row)));
            auto gdu16Vec = ShiftLeft<5>(ShiftRight<2>(PromoteTo(rdu16, g16Row)));
            auto bdu16Vec = ShiftRight<3>(PromoteTo(rdu16, b16Row));

            auto result = Or(Or(rdu16Vec, gdu16Vec), bdu16Vec);
            StoreU(result, du16, dst);

            src += 4 * pixels;
            dst += pixels;
        }

        for (; x < width; ++x) {
            uint16_t red565 = ((uint16_t) roundf(half_to_float(src[0]) * maxColors) >> 3) << 11;
            uint16_t green565 = ((uint16_t) roundf(half_to_float(src[1]) * maxColors) >> 2) << 5;
            uint16_t blue565 = (uint16_t) roundf(half_to_float(src[2]) * maxColors) >> 3;

            auto result = static_cast<uint16_t>(red565 | green565 | blue565);
            dst[0] = result;

            src += 4;
            dst += 1;
        }

    }

    inline Vec<FixedTag<uint16_t, 4>>
    ConvertF32ToU16Row(Vec<FixedTag<float, 4>> v, float maxColors) {
        FixedTag<float32_t, 4> df32;
        Rebind<int32_t, decltype(df32)> ri32;
        Rebind<uint16_t, decltype(df32)> ru8;

        auto minColors = Zero(df32);
        auto vMaxColors = Set(df32, (float) maxColors);
        auto lower = DemoteTo(ru8,
                              ConvertTo(ri32, Max(Min(Mul(v, vMaxColors), vMaxColors), minColors)));
        return lower;
    }

    void
    RGBAF32To565RowHWY(const float *source, uint16_t *destination, int width,
                       float maxColors) {
        const FixedTag<float, 4> df16;
        const FixedTag<uint16_t, 4> du16;
        const FixedTag<uint8_t, 4> du8;
        using VU16 = Vec<decltype(df16)>;
        using VU8 = Vec<decltype(du8)>;

        int x = 0;
        int pixels = 4;

        auto src = reinterpret_cast<const float *>(source);
        auto dst = reinterpret_cast<uint16_t *>(destination);
        for (x = 0; x + pixels < width; x += pixels) {
            VU16 ru16Row;
            VU16 gu16Row;
            VU16 bu16Row;
            VU16 au16Row;
            LoadInterleaved4(df16, reinterpret_cast<const float *>(src),
                             ru16Row, gu16Row,
                             bu16Row,
                             au16Row);

            auto r16Row = ConvertF32ToU16Row(ru16Row, maxColors);
            auto g16Row = ConvertF32ToU16Row(gu16Row, maxColors);
            auto b16Row = ConvertF32ToU16Row(bu16Row, maxColors);

            auto rdu16Vec = ShiftLeft<11>(ShiftRight<3>(r16Row));
            auto gdu16Vec = ShiftLeft<5>(ShiftRight<2>(g16Row));
            auto bdu16Vec = ShiftRight<3>(b16Row);

            auto result = Or(Or(rdu16Vec, gdu16Vec), bdu16Vec);
            StoreU(result, du16, dst);

            src += 4 * pixels;
            dst += pixels;
        }

        for (; x < width; ++x) {
            uint16_t red565 = ((uint16_t) (src[0] * maxColors) >> 3) << 11;
            uint16_t green565 = ((uint16_t) (src[1] * maxColors) >> 2) << 5;
            uint16_t blue565 = (uint16_t) (src[2] * maxColors) >> 3;

            auto result = static_cast<uint16_t>(red565 | green565 | blue565);
            dst[0] = result;

            src += 4;
            dst += 1;
        }

    }

    void RGBAF16To565HWY(const uint16_t *sourceData, int srcStride,
                         uint16_t *dst, int dstStride, int width,
                         int height) {
        float maxColors = powf(2, (float) 8) - 1;

        auto mSrc = reinterpret_cast<const uint8_t *>(sourceData);
        auto mDst = reinterpret_cast<uint8_t *>(dst);

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
                    [start, end, mSrc, mDst, srcStride, dstStride, width, maxColors]() {
                        for (int y = start; y < end; ++y) {
                            RGBAF16To565RowHWY(
                                    reinterpret_cast<const uint16_t *>(mSrc + srcStride * y),
                                    reinterpret_cast<uint16_t *>(mDst + dstStride * y),
                                    width, maxColors);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }

    void RGBAF32To565HWY(const float *sourceData, int srcStride,
                         uint16_t *dst, int dstStride, int width,
                         int height) {
        float maxColors = powf(2, (float) 8) - 1;

        auto mSrc = reinterpret_cast<const uint8_t *>(sourceData);
        auto mDst = reinterpret_cast<uint8_t *>(dst);

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
                    [start, end, mSrc, mDst, srcStride, dstStride, width, maxColors]() {
                        for (int y = start; y < end; ++y) {
                            RGBAF32To565RowHWY(
                                    reinterpret_cast<const float *>(mSrc + srcStride * y),
                                    reinterpret_cast<uint16_t *>(mDst + dstStride * y),
                                    width, maxColors);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }
}

HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace coder {
    HWY_EXPORT(Rgba8To565HWY);
    HWY_DLLEXPORT void Rgba8To565(const uint8_t *sourceData, int srcStride,
                                  uint16_t *dst, int dstStride, int width,
                                  int height, int bitDepth) {
        HWY_DYNAMIC_DISPATCH(Rgba8To565HWY)(sourceData, srcStride, dst, dstStride, width,
                                            height, bitDepth);
    }

    HWY_EXPORT(RGBAF16To565HWY);
    HWY_DLLEXPORT void RGBAF16To565(const uint16_t *sourceData, int srcStride,
                                    uint16_t *dst, int dstStride, int width,
                                    int height) {
        HWY_DYNAMIC_DISPATCH(RGBAF16To565HWY)(sourceData, srcStride, dst, dstStride, width,
                                              height);
    }

    HWY_EXPORT(RGBAF32To565HWY);
    HWY_DLLEXPORT void RGBAF32To565(const float *sourceData, int srcStride,
                                    uint16_t *dst, int dstStride, int width,
                                    int height) {
        HWY_DYNAMIC_DISPATCH(RGBAF32To565HWY)(sourceData, srcStride, dst, dstStride, width,
                                              height);
    }
}
#endif