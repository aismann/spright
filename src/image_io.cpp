
#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "gifenc/gifenc.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  using RGBASpan = span<RGBA>;

  // https://en.wikipedia.org/wiki/Median_cut
  std::vector<RGBA> median_cut_reduction(RGBASpan image, int max_colors) {
    struct Bucket {
      RGBASpan colors;
      RGBA::Channel max_channel_range;
    };

    const auto sort_bucket = [](Bucket& bucket) {
      // compute channel with maximum range
      auto& colors = bucket.colors;
      auto min = colors.front();
      auto max = colors.front();
      for (const auto& c : colors) {
        for (auto i = 0; i < 4; ++i) {
          min.channel(i) = std::min(min.channel(i), c.channel(i));
          max.channel(i) = std::max(max.channel(i), c.channel(i));
        }
      }
      auto max_channel = 0;
      auto max_channel_range = RGBA::Channel{ };
      for (auto i = 0; i < 4; ++i) {
        const auto channel_range = RGBA::to_channel(max.channel(i) - min.channel(i));
        if (channel_range > max_channel_range) {
          max_channel_range = channel_range;
          max_channel = i;
        }
      }
      bucket.max_channel_range = max_channel_range;

      // sort colors by this channel
      std::sort(colors.begin(), colors.end(),
        [&](const RGBA& a, const RGBA& b) {
          return a.channel(max_channel) < b.channel(max_channel);
        });
    };

    auto buckets = std::vector<Bucket>();
    buckets.reserve(to_unsigned(max_colors));

    // start with one bucket containing whole image
    buckets.push_back({ image });
    sort_bucket(buckets.back());

    while (to_int(buckets.size()) < max_colors) {
      // split bucket with maximum range
      if (buckets.back().max_channel_range == 0)
        break;
      const auto colors = buckets.back().colors;
      buckets.pop_back();

      auto half_buckets = std::array<Bucket, 2>{ };
      scheduler.for_each_parallel(2, [&](size_t index) {
        half_buckets[index].colors =
          (index == 0 ?
            colors.subspan(0, colors.size() / 2) :
            colors.subspan(colors.size() / 2));
        sort_bucket(half_buckets[index]);
      });

      // insert sorted in bucket list
      for (const auto& half_bucket : half_buckets)
        buckets.insert(std::lower_bound(buckets.begin(), buckets.end(), half_bucket,
          [](const Bucket& a, const Bucket& b) {
            return (a.max_channel_range < b.max_channel_range);
          }), half_bucket);
    }

    // get average colors of buckets
    auto palette = Palette();
    for (const auto& bucket : buckets) {
      auto sum = std::array<uint32_t, 4>();
      for (const auto& color : bucket.colors)
        for (auto i = 0; i < 4; ++i)
          sum[to_unsigned(i)] += color.channel(i);
      auto color = RGBA{ };
      for (auto i = 0; i < 4; ++i)
        color.channel(i) = RGBA::to_channel(sum[to_unsigned(i)] / bucket.colors.size());
      palette.push_back(color);
    }

    // remove duplicate colors from palette
    std::sort(palette.begin(), palette.end());
    palette.erase(std::unique(palette.begin(), palette.end()), palette.end());
    return palette;
  }

  int index_of_closest_palette_color(const Palette& palette, const RGBA& color) {
    auto min_index = 0;
    auto min_distance = std::numeric_limits<int>::max();
    for (auto i = 0u; i < palette.size(); ++i) {
      const auto r = palette[i].r - color.r;
      const auto g = palette[i].g - color.g;
      const auto b = palette[i].b - color.b;
      const auto distance = (r * r + g * g + b * b);
      if (distance < min_distance) {
        min_index = to_int(i);
        min_distance = distance;
      }
    }
    return min_index;
  }

  const RGBA& closest_palette_color(const Palette& palette, const RGBA& color) {
    return palette[to_unsigned(index_of_closest_palette_color(palette, color))];
  }

  // https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
  void floyd_steinberg_dithering(ImageView<RGBA> image_rgba, const Palette& palette) {
    const auto diff = [](const RGBA::Channel& a, const RGBA::Channel& b) {
      return static_cast<int>(a) - static_cast<int>(b);
    };
    const auto saturate = [](int value) {
      return RGBA::to_channel(std::clamp(value, 0, 255));
    };
    const auto w = image_rgba.width();
    const auto h = image_rgba.height();
    for (auto y = 0; y < h; ++y)
      for (auto x = 0; x < w; ++x) {
        auto& color = image_rgba.value_at({ x, y });
        const auto old_color = color;
        color = closest_palette_color(palette, color);
        const auto error_r = diff(old_color.r, color.r);
        const auto error_g = diff(old_color.g, color.g);
        const auto error_b = diff(old_color.b, color.b);
        const auto apply_error = [&](int x, int y, int fs) {
          auto& color = image_rgba.value_at({
            std::clamp(x, 0, w - 1), std::clamp(y, 0, h - 1)
          });
          color.r = saturate(color.r + error_r * fs / 16);
          color.g = saturate(color.g + error_g * fs / 16);
          color.b = saturate(color.b + error_b * fs / 16);
        };
        apply_error(x + 1, y,     7);
        apply_error(x - 1, y + 1, 3);
        apply_error(x    , y + 1, 5);
        apply_error(x + 1, y + 1, 1);
      }
  }

  [[maybe_unused]] Palette generate_palette(const Image& image, int max_colors) {
    auto clone = clone_image(image);
    const auto clone_rgba = clone.view<RGBA>();
    return median_cut_reduction(
      { clone_rgba.values(), to_unsigned(clone_rgba.size()) }, max_colors);
  }

  Palette generate_palette(const Animation& animation, int max_colors) {
    if (animation.frames.empty())
      return {};
    const auto width = animation.frames[0].image.width();
    const auto height = animation.frames[0].image.height();
    const auto count = to_int(animation.frames.size());
    auto merged = Image(ImageType::RGBA, width, height * count);
    auto pos = merged.data().data();
    for (const auto& frame : animation.frames) {
      std::memcpy(pos, frame.image.data().data(), frame.image.size_bytes());
      pos += frame.image.size_bytes();
    }
    return generate_palette(merged, max_colors);
  }

  Image quantize_image(ImageView<const RGBA> image_rgba, const Palette& palette) {
    auto out = Image(ImageType::Mono, image_rgba.width(), image_rgba.height());
    const auto out_mono = out.view<RGBA::Channel>();
    for (auto y = 0; y < image_rgba.height(); ++y)
      for (auto x = 0; x < image_rgba.width(); ++x)
        out_mono.value_at({ x, y }) = RGBA::to_channel(
          index_of_closest_palette_color(palette,
            image_rgba.value_at({ x, y })));
    return out;
  }

  // https://giflib.sourceforge.net/whatsinagif/
  bool write_gif(const std::string& filename, const Animation& animation) {
    if (animation.frames.empty())
      return false;

    const auto max_colors = (animation.max_colors ?
      std::min(animation.max_colors, 256) : 256);
    const auto palette = generate_palette(animation, max_colors);
    auto bits = 0;
    for (auto c = palette.size() - 1; c; c >>= 1)
      ++bits;
    auto palette_rgb = std::make_unique<RGBA::Channel[]>((1 << bits) * 3);
    auto pos = palette_rgb.get();
    for (const auto& color : palette) {
      *pos++ = color.r;
      *pos++ = color.g;
      *pos++ = color.b;
    }

    auto transparent_index = -1;
    if (animation.color_key)
      transparent_index = index_of_closest_palette_color(
        palette, *animation.color_key);

    auto width = 0;
    auto height = 0;
    for (const auto& frame : animation.frames) {
      width = std::max(width, frame.image.width());
      height = std::max(height, frame.image.height());
    }
    if (width > 0xFFFF || height > 0xFFFF)
      return false;

    auto gif = ge_new_gif(filename.c_str(),
      static_cast<uint16_t>(width),
      static_cast<uint16_t>(height),
      palette_rgb.get(), bits, transparent_index, animation.loop_count);
    if (!gif)
      return false;

    auto mutex = std::mutex{ };
    auto frames_data = std::vector<Image>(animation.frames.size());
    scheduler.for_each_parallel(animation.frames,
      [&](const Animation::Frame& frame) {
        auto frame_data = Image{ };
        if (to_int(palette.size()) == max_colors) {
          auto dithered = clone_image(frame.image);
          floyd_steinberg_dithering(dithered.view<RGBA>(), palette);
          frame_data = quantize_image(dithered.view<const RGBA>(), palette);
        }
        else {
          frame_data = quantize_image(frame.image.view<RGBA>(), palette);
        }
        const auto lock = std::lock_guard(mutex);
        const auto index = to_unsigned(std::distance(animation.frames.data(), &frame));
        frames_data[index] = std::move(frame_data);
      });

    for (auto i = 0u; i < animation.frames.size(); ++i) {
      const auto& frame = animation.frames[i];
      const auto& frame_data = frames_data[i];
      const auto delay = std::chrono::duration_cast<
        std::chrono::duration<uint16_t, std::ratio<1, 100>>>(
        std::chrono::duration<real>(frame.duration)).count();

      std::memcpy(gif->frame, frame_data.data().data(), frame_data.size_bytes());
      ge_add_frame(gif, delay);
    }
    ge_close_gif(gif);
    return true;
  }
} // namespace

Image load_image(const std::filesystem::path& filename) {
  auto width = 0;
  auto height = 0;
  auto data = std::add_pointer_t<std::byte>{ };

#if defined(EMBED_TEST_FILES)
  if (filename == "test/Items.png") {
    unsigned char file[] {
#include "test/Items.png.inc"
    };
    auto channels = 0;
    data = reinterpret_cast<std::byte*>(stbi_load_from_memory(
        file, sizeof(file), &width, &height, &channels, sizeof(RGBA)));
  }
  else
#endif

#if defined(_WIN32)
  if (auto file = _wfopen(filename.wstring().c_str(), L"rb")) {
#else
  if (auto file = std::fopen(path_to_utf8(filename).c_str(), "rb")) {
#endif
    auto channels = 0;
    data = reinterpret_cast<std::byte*>(stbi_load_from_file(
        file, &width, &height, &channels, sizeof(RGBA)));
    std::fclose(file);
  }
  if (!data)
    throw std::runtime_error("loading file '" +
      path_to_utf8(filename) + "' failed");

  return Image(ImageType::RGBA, width, height, data);
}

void load_image_header(const std::filesystem::path& filename, int* width, int* height) {
#if defined(EMBED_TEST_FILES)
  if (filename == "test/Items.png") {
    unsigned char file[] {
#include "test/Items.png.inc"
    };
    stbi_info_from_memory(file, sizeof(file), width, height, nullptr);
  }
  else
#endif

#if defined(_WIN32)
  if (auto file = _wfopen(filename.wstring().c_str(), L"rb")) {
#else
  if (auto file = std::fopen(path_to_utf8(filename).c_str(), "rb")) {
#endif
    stbi_info_from_file(file, width, height, nullptr);
    std::fclose(file);
  }
  if (!*width)
    throw std::runtime_error("loading file '" +
      path_to_utf8(filename) + "' failed");
}

void save_image(const Image& image, const std::filesystem::path& path) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  const auto filename = path_to_utf8(path);
  const auto extension = to_lower(path_to_utf8(path.extension()));
  const auto result = [&]() -> bool {
    if (extension == ".gif") {
      auto animation = Animation{ };
      animation.frames.push_back({ 0, clone_image(image), 0.0 });
      return write_gif(filename, animation);
    }

    const auto comp = to_int(sizeof(RGBA));
    const auto image_rgba = image.view<RGBA>();
    if (extension == ".png" || extension.empty())
      return stbi_write_png(filename.c_str(),
        image.width(), image.height(), comp, image_rgba.values(), 0);

    if (extension == ".bmp")
      return stbi_write_bmp(filename.c_str(),
        image.width(), image.height(), comp, image_rgba.values());

    stbi_write_tga_with_rle = 1;
    if (extension == ".tga")
      return stbi_write_tga(filename.c_str(),
        image.width(), image.height(), comp, image_rgba.values());

    error("unsupported image file format '", filename, "'");
  }();
  if (!result)
    error("writing file '", filename, "' failed");
}

void save_animation(const Animation& animation, const std::filesystem::path& path) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  const auto filename = path_to_utf8(path);
  const auto extension = to_lower(path_to_utf8(path.extension()));
  const auto result = [&]() -> bool {
    if (extension == ".gif")
      return write_gif(filename, animation);

    error("unsupported animation file format '", filename, "'");
  }();
  if (!result)
    error("writing file '", filename, "' failed");
}

} // namespace
