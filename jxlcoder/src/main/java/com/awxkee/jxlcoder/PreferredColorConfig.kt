package com.awxkee.jxlcoder

import android.os.Build
import androidx.annotation.RequiresApi

enum class PreferredColorConfig(internal val value: Int) {
    /**
     * Default then library will determine the best color space by itself
     * @property RGBA_8888 will clip out HDR content if some
     * @property RGBA_F16 available only from 26+ OS version
     * @property RGB_565 will clip out HDR and 8bit content
     * @property RGBA_1010102 supported only on os 33+
     * @property HARDWARE supported only on OS 29+ and have default android HARDWARE bitmap limitations
     */
    DEFAULT(1),
    RGBA_8888(2),

    @RequiresApi(Build.VERSION_CODES.O)
    RGBA_F16(3),
    RGB_565(4),

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    RGBA_1010102(5),

    @RequiresApi(Build.VERSION_CODES.Q)
    HARDWARE(6),
}