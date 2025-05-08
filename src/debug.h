#pragma once

#include "packing.h"

namespace spright {

void draw_debug_info(Image& image, const Sprite& sprite, const SizeF& scale = { 1, 1 });
void draw_debug_info(Image& image, const Slice& slice, const SizeF& scale = { 1, 1 });

} // namespace
