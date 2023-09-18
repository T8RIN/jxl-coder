//
// Created by Radzivon Bartoshyk on 16/09/2023.
//

#ifndef AVIF_REFORMATBITMAP_H
#define AVIF_REFORMATBITMAP_H

#include <jni.h>
#include <vector>
#include "Support.h"

void
ReformatColorConfig(JNIEnv *env, std::vector<uint8_t> &imageData, std::string &imageConfig,
                    PreferredColorConfig preferredColorConfig, int depth,
                    int imageWidth, int imageHeight, int *stride, bool *useFloats,
                    jobject *hwBuffer);

#endif //AVIF_REFORMATBITMAP_H
