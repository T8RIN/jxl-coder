package com.awxkee.jxlcoder

import android.graphics.Bitmap
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
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
import com.t8rin.bitmapscaler.ScaleMode.Bilinear
import com.t8rin.bitmapscaler.ScaleMode.Cubic
import com.t8rin.bitmapscaler.ScaleMode.Hann
import com.t8rin.bitmapscaler.ScaleMode.Lanczos
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
                        .allowHardware(false)
                        .data("https://avatars.githubusercontent.com/u/52178347?s=80&u=bc9f21f39a78388cbdf833914d210dfe030d7116&v=4")
                        .build()
                ).drawable!!.toBitmap()
            }


            setContent {
                Column(Modifier.verticalScroll(rememberScrollState())) {
                    image?.let {
                        Image(bitmap = it.asImageBitmap(), contentDescription = null)
                        Image(
                            bitmap = remember(it) {
                                BitmapScaler.scale(
                                    bitmap = it,
                                    dstWidth = 1000,
                                    dstHeight = 1000,
                                    scaleMode = Hann
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