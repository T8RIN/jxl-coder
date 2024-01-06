package com.awxkee.jxlcoder

import android.graphics.Bitmap
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.core.graphics.drawable.toBitmap
import coil.Coil
import coil.request.ImageRequest
import com.t8rin.bitmapscaler.BitmapScaler
import com.t8rin.bitmapscaler.ScaleMode
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        var image by mutableStateOf<Bitmap?>(null)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CoroutineScope(Dispatchers.Main).launch {
                image = Coil.imageLoader(this@MainActivity).execute(
                    ImageRequest.Builder(this@MainActivity)
                        .size(1000, 1000)
                        .allowHardware(false)
                        .data("https://github.com/T8RIN/ImageToolbox/raw/master/fastlane/metadata/android/en-US/images/banner/banner1.png")
                        .build()
                ).drawable!!.toBitmap()
            }


            setContent {
                LazyColumn {
                    image?.let { bitmap ->
                        item {
                            Text("Upscaling")
                            Image(bitmap = bitmap.asImageBitmap(), contentDescription = null)
                        }
                        ScaleMode.values().forEach {
                            item {
                                Text(text = it.name)
                                Image(
                                    bitmap = remember(bitmap) {
                                        BitmapScaler.scale(
                                            bitmap = bitmap,
                                            dstWidth = 3000,
                                            dstHeight = 3000,
                                            scaleMode = it
                                        ).asImageBitmap()
                                    },
                                    contentDescription = null,
                                    contentScale = ContentScale.FillWidth,
                                    modifier = Modifier.fillMaxWidth()
                                )
                            }
                        }
                    }
                    image?.let { bitmap ->
                        item {
                            Text("LowerScaling")
                            Image(bitmap = bitmap.asImageBitmap(), contentDescription = null)
                        }
                        ScaleMode.values().forEach {
                            item {
                                Text(text = it.name)
                                Image(
                                    bitmap = remember(bitmap) {
                                        BitmapScaler.scale(
                                            bitmap = bitmap,
                                            dstWidth = 200,
                                            dstHeight = 200,
                                            scaleMode = it
                                        ).asImageBitmap()
                                    },
                                    contentDescription = null,
                                    contentScale = ContentScale.FillWidth,
                                    modifier = Modifier.fillMaxWidth()
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}