package com.t8rin.bitmapscaler

import android.graphics.Bitmap
import androidx.annotation.Keep

@Keep
object BitmapScaler {

    fun scale(
        bitmap: Bitmap,
        dstWidth: Int,
        dstHeight: Int,
        scaleMode: ScaleMode
    ): Bitmap = scaleImpl(
        bitmap = bitmap,
        dstWidth = dstWidth,
        dstHeight = dstHeight,
        scaleMode = scaleMode.ordinal + 1
    )

    private external fun scaleImpl(
        bitmap: Bitmap,
        dstWidth: Int,
        dstHeight: Int,
        scaleMode: Int
    ): Bitmap

    init {
        System.loadLibrary("jxlcoder")
    }

}