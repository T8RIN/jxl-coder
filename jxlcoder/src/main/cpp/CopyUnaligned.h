//
// Created by Radzivon Bartoshyk on 12/09/2023.
//

#ifndef JXL_COPYUNALIGNEDRGBA_H
#define JXL_COPYUNALIGNEDRGBA_H

#include <vector>

namespace coder {
    void
    CopyUnalignedRGBA(const uint8_t *__restrict__ src, int srcStride, uint8_t *__restrict__ dst,
                      int dstStride, int width,
                      int height,
                      int pixelSize);

    void
    CopyUnalignedRGB565(const uint8_t *__restrict__ src, int srcStride, uint8_t *__restrict__ dst,
                        int dstStride, int width,
                        int height);
}

#endif //JXL_COPYUNALIGNEDRGBA_H
