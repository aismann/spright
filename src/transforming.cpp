
#include "transforming.h"

namespace spright {

namespace {
  void transform_image(Image& image, const TransformStep& step, 
      const Image& source) {
    std::visit(overloaded{
      [&](const TransformScale& scale) {
        image = resize_image(image, scale.scale, scale.scale_filter);
      },
      [&](const TransformResize& resize) {
        auto scale = SizeF{
          (resize.size.x ? resize.size.x / image.width() : 1.0),
          (resize.size.y ? resize.size.y / image.height() : 1.0),
        };
        if (!resize.size.x)
          scale.x = scale.y;
        if (!resize.size.y)
          scale.y = scale.x;
        image = resize_image(image, scale, resize.scale_filter);
      },
      [&](const TransformRotate& rotate) {
        const auto background = guess_colorkey(source);
        image = rotate_image(image, rotate.angle, background, rotate.rotate_method);
      }
    }, step);
  }

  void transform_image(Image& image, 
      const std::vector<TransformPtr>& transforms, 
      const Image& source_image) {
    for (const auto& transform : transforms)
      for (const auto& step : *transform)
        transform_image(image, step, source_image);
  }

  void transform_scale(SizeF& scale, const TransformStep& step) {
    std::visit(overloaded{
      [&](const TransformScale& resize) {
        scale.x *= resize.scale.x;
        scale.y *= resize.scale.y;
      },
      [&](const TransformResize&) {
        scale = { };
      },
      [&](const TransformRotate&) {
        scale = { };
      }
    }, step);
  }
} // namespace

void transform_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    if (!sprite.transforms.empty()) {
      sprite.untransformed_source = sprite.source;
      sprite.untransformed_source_rect = sprite.source_rect;

      auto image = convert_to_linear(sprite.source->image(), sprite.source_rect);
      transform_image(image, sprite.transforms, sprite.source->image());

      sprite.source = std::make_shared<ImageFile>(
        convert_to_srgb(image), sprite.source->path(), 
        sprite.source->filename());
      sprite.source_rect = image.rect();
    }
}

void restore_untransformed_sources(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    if (sprite.untransformed_source) {
      sprite.source = std::move(sprite.untransformed_source);
      sprite.source_rect = sprite.untransformed_source_rect;
      // source is not trimmed when trimming after transformation
      sprite.trimmed_source_rect = sprite.source_rect;
    }
}

Image transform_output(Image&& source, 
    const std::vector<TransformPtr>& transforms) {
  if (transforms.empty())
    return std::move(source);

  auto image = convert_to_linear(source);
  transform_image(image, transforms, source);
  return convert_to_srgb(image);
}

SizeF get_transform_scale(const std::vector<TransformPtr>& transforms) {
  auto scale = SizeF{ 1, 1 };
  for (const auto& transform : transforms)
    for (const auto& step : *transform)
      transform_scale(scale, step);
  return scale;
}

} // namespace
