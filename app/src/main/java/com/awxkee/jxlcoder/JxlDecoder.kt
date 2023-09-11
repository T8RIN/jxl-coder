package com.awxkee.jxlcoder

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.drawable.BitmapDrawable
import android.util.Size
import coil.ImageLoader
import coil.decode.DecodeResult
import coil.decode.Decoder
import coil.fetch.SourceResult
import coil.request.Options
import coil.size.Scale
import coil.size.pxOrElse
import kotlinx.coroutines.runInterruptible
import okio.BufferedSource
import okio.ByteString.Companion.toByteString

class JxlDecoder(
    private val source: SourceResult,
    private val options: Options,
    private val imageLoader: ImageLoader
) : Decoder {

    override suspend fun decode(): DecodeResult? = runInterruptible {
        try {
            val byteArray = source.source.source().readByteArray()
            val originalSize = JxlCoder().getSize(byteArray) ?: return@runInterruptible null
            val dstWidth = options.size.width.pxOrElse { 0 }
            val dstHeight = options.size.height.pxOrElse { 0 }
            if (options.scale == Scale.FIT) {
                val scaledSize = aspectScale(originalSize, Size(dstWidth, dstHeight))
                val originalImage =
                    JxlCoder().decodeSampled(byteArray, scaledSize.width, scaledSize.height)
                return@runInterruptible DecodeResult(
                    BitmapDrawable(
                        options.context.resources,
                        originalImage
                    ), true
                )
            }
            val isHorizontal = originalSize.width > originalSize.height
            val square = originalSize.width * originalSize.height
            val maxSquare = 2480 * 1440
            val targetSizeNotAspect: Size =
                if (square > maxSquare) (if (isHorizontal) Size(
                    2480,
                    1440
                ) else Size(
                    1440,
                    2480
                )) else originalSize
            val aspectSize = aspectScale(originalSize, targetSizeNotAspect)
            val originalImage = JxlCoder().decodeSampled(byteArray, aspectSize.width, aspectSize.height)
            val resizedBitmap = resizeAspectFill(originalImage, Size(dstWidth, dstHeight))
            originalImage.recycle()
            return@runInterruptible DecodeResult(
                BitmapDrawable(
                    options.context.resources,
                    resizedBitmap
                ), true
            )
        } catch (e: Exception) {
            throw e
        }
    }

    private fun resizeAspectFill(sourceBitmap: Bitmap, dstSize: Size): Bitmap {
        val background = Bitmap.createBitmap(dstSize.width, dstSize.height, Bitmap.Config.ARGB_8888)
        val originalWidth: Float = background.width.toFloat()
        val originalHeight: Float = background.height.toFloat()
        val canvas = Canvas(background)

        val scale: Float =
            if (dstSize.height > dstSize.width) dstSize.height / originalHeight else dstSize.width / originalWidth

        val xTranslation = (dstSize.width - originalWidth * scale) / 2.0f
        val yTranslation: Float = (dstSize.height - originalHeight * scale) / 2.0f

        val transformation = Matrix()
        transformation.postTranslate(xTranslation, yTranslation)
        transformation.preScale(scale, scale)

        val paint = Paint(Paint.ANTI_ALIAS_FLAG)
        paint.isFilterBitmap = true

        canvas.drawBitmap(sourceBitmap, transformation, paint)
        return background
    }

    private fun aspectScale(sourceSize: Size, dstSize: Size): Size {
        val isHorizontal = sourceSize.width > sourceSize.height
        val targetSize = if (isHorizontal) dstSize else Size(dstSize.height, dstSize.width)
        if (targetSize.width > sourceSize.width && targetSize.height > sourceSize.height) {
            return sourceSize
        }
        val imageAspectRatio = sourceSize.width.toFloat() / sourceSize.height.toFloat()
        val canvasAspectRation = targetSize.width.toFloat() / targetSize.height.toFloat()
        val resizeFactor: Float = if (imageAspectRatio > canvasAspectRation) {
            targetSize.width.toFloat() / sourceSize.width.toFloat()
        } else {
            targetSize.height.toFloat() / sourceSize.height.toFloat()
        }
        return Size(
            (sourceSize.width.toFloat() * resizeFactor).toInt(),
            (sourceSize.height.toFloat() * resizeFactor).toInt()
        )
    }

    class Factory : Decoder.Factory {
        override fun create(
            result: SourceResult,
            options: Options,
            imageLoader: ImageLoader
        ) = if (isJXL(result.source.source())) {
            JxlDecoder(result, options, imageLoader)
        } else null

        private val MAGIC_1 = byteArrayOf(0xFF.toByte(), 0x0A).toByteString()
        private val MAGIC_2 = byteArrayOf(
            0x0.toByte(),
            0x0.toByte(),
            0x0.toByte(),
            0x0C.toByte(),
            0x4A,
            0x58,
            0x4C,
            0x20,
            0x0D,
            0x0A,
            0x87.toByte(),
            0x0A
        ).toByteString()

        private fun isJXL(source: BufferedSource): Boolean {
            return source.rangeEquals(0, MAGIC_1) || source.rangeEquals(
                0,
                MAGIC_2
            )
        }
    }

}