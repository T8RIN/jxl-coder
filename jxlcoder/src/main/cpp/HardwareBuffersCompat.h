/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 16/09/2023
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

#ifndef JXL_HARDWAREBUFFERSCOMPAT_H
#define JXL_HARDWAREBUFFERSCOMPAT_H

#include <android/bitmap.h>
#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>

bool loadAHardwareBuffersAPI();

typedef int (*_Nullable AHardwareBufferAllocateFunc)(
    const AHardwareBuffer_Desc *_Nonnull desc, AHardwareBuffer *_Nullable *_Nonnull outBuffer
);

typedef int (*_Nullable AHardwareBufferIsSupportedFunc)(
    const AHardwareBuffer_Desc *_Nonnull desc
);

typedef int (*_Nullable AHardwareBufferReleaseFunc)(
    AHardwareBuffer *_Nonnull buffer
);

typedef int (*_Nullable AHardwareBufferUnlockFunc)(
    AHardwareBuffer *_Nonnull buffer, int32_t *_Nullable fence
);

typedef int (*_Nullable AHardwareBufferLockFunc)(
    AHardwareBuffer *_Nonnull buffer, uint64_t usage, int32_t fence,
    const ARect *_Nullable rect, void *_Nullable *_Nonnull outVirtualAddress
);

typedef void (*_Nullable AHardwareBufferDescribeFunc)(
    const AHardwareBuffer *_Nonnull buffer, AHardwareBuffer_Desc *_Nonnull outDesc
);

typedef jobject _Nullable (*_Nullable AHardwareBufferToHardwareBufferFunc)(
    JNIEnv *_Nonnull env, AHardwareBuffer *_Nonnull hardwareBuffer
);

extern AHardwareBufferAllocateFunc AHardwareBuffer_allocate_compat;
extern AHardwareBufferIsSupportedFunc AHardwareBuffer_isSupported_compat;
extern AHardwareBufferUnlockFunc AHardwareBuffer_unlock_compat;
extern AHardwareBufferReleaseFunc AHardwareBuffer_release_compat;
extern AHardwareBufferLockFunc AHardwareBuffer_lock_compat;
extern AHardwareBufferToHardwareBufferFunc AHardwareBuffer_toHardwareBuffer_compat;
extern AHardwareBufferDescribeFunc AHardwareBuffer_describe_compat;

#endif //JXL_HARDWAREBUFFERSCOMPAT_H
