#pragma once

#include "image.h"
#include "settings.h"
#include "FilenameSequence.h"
#include <memory>

namespace spright {

using ImageFilePtr = std::shared_ptr<const class ImageFile>;
using SheetPtr = std::shared_ptr<const struct Sheet>;
using OutputPtr = std::shared_ptr<const struct Output>;
using MapVectorPtr = std::shared_ptr<const std::vector<ImageFilePtr>>;
using StringMap = std::map<std::string, std::string, std::less<>>;

enum class AnchorX { left, center, right };
enum class AnchorY { top, middle, bottom };

template<typename T>
struct AnchorT : PointT<T> {
  AnchorX anchor_x;
  AnchorY anchor_y;
};
using Anchor = AnchorT<int>;
using AnchorF = AnchorT<real>;

enum class Trim { none, rect, convex };

enum class Alpha { keep, opaque, clear, bleed, premultiply, colorkey };

enum class Pack { binpack, rows, columns, compact, origin, single, layers, keep };

enum class Duplicates { keep, share, drop };

struct Extrude {
  int count;
  WrapMode mode;
};

class ImageFile {
public:
  ImageFile(Image image, std::filesystem::path path, std::filesystem::path filename) 
    : m_image(std::move(image)),
      m_path(std::move(path)),
      m_filename(std::move(filename)),
      m_width(m_image.width()), 
      m_height(m_image.height()) {
  }

  ImageFile(std::filesystem::path path, std::filesystem::path filename, RGBA colorkey = { }) 
    : m_path(std::move(path)), 
      m_filename(std::move(filename)),
      m_colorkey(colorkey) {
    load_image_header(m_path / m_filename, &m_width, &m_height);
  }

  const std::filesystem::path& path() const { return m_path; }
  const std::filesystem::path& filename() const { return m_filename; }
  int width() const { return m_width; }
  int height() const { return m_height; }
  Rect rect() const { return { 0, 0, width(), height() }; }

  Image& image() {    
    lazy_load_image();
    return m_image;
  }

  const Image& image() const {
    lazy_load_image();
    return m_image;
  }

private:
  void lazy_load_image() const {
    const auto lock = std::lock_guard(m_mutex);
    if (m_image)
      return;

    m_image = load_image(m_path / m_filename);
    auto colorkey = m_colorkey;
    if (colorkey != RGBA{ }) {
      if (!colorkey.a)
        colorkey = guess_colorkey(m_image);
      replace_color(m_image, colorkey, RGBA{ });
    }
  }

  mutable std::mutex m_mutex;
  mutable Image m_image;
  std::filesystem::path m_path;
  std::filesystem::path m_filename;
  RGBA m_colorkey{ };
  int m_width{ };
  int m_height{ };
};

struct TransformScale {
  SizeF scale;
  ScaleFilter scale_filter;
};

struct TransformRotate {
  real angle;
  RotateMethod rotate_method;
};

struct TransformResize {
  SizeF size;
  ScaleFilter scale_filter;
};

using TransformStep = std::variant<TransformScale, TransformRotate, TransformResize>;
using Transform = std::vector<TransformStep>;
using TransformPtr = std::shared_ptr<const Transform>;

struct Input {
  int index{ };
  std::string source_filenames;
  std::vector<ImageFilePtr> sources;
};

struct Output {
  int warning_line_number{ };
  FilenameSequence filename;
  std::string default_map_suffix;
  std::vector<std::string> map_suffixes;
  Alpha alpha{ };
  RGBA alpha_color{ };
  std::vector<TransformPtr> transforms;
  bool debug{ };

  // the scale after transformation, 0 when rotated
  SizeF scale{ };
};

struct Sheet {
  int index{ };
  std::string id;
  std::filesystem::path input_file;
  std::vector<OutputPtr> outputs;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool square{ };
  int divisible_width{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  Duplicates duplicates{ };
  Pack pack{ };
};

struct Sprite {
  int warning_line_number{ };
  int index{ };
  int input_index{ };
  int input_sprite_index{ };
  std::string id;
  SheetPtr sheet;
  ImageFilePtr source;
  MapVectorPtr maps;
  Rect source_rect{ };
  Rect trimmed_source_rect{ };
  AnchorF pivot{ };
  Trim trim{ Trim::none };
  Margin trim_margin{ };
  int trim_threshold{ };
  bool trim_gray_levels{ };
  bool crop{ };
  bool crop_pivot{ };
  // margin around rect, resulting in bounds
  Margin margin{ };
  Extrude extrude{ };
  Size min_size{ };
  Size divisible_size{ };
  std::string common_size;
  // placement of the trimmed-rect in rect
  Anchor align{ };
  std::string align_pivot;
  StringMap tags;
  VariantMap data;

  std::vector<TransformPtr> transforms;
  ImageFilePtr untransformed_source;
  Rect untransformed_source_rect{ };

  int slice_index{ -1 };
  // total space it allocates on the output (including padding added by extrude)
  Size size{ };
  Rect rect{ };
  Rect trimmed_rect{ };
  bool rotated{ };
  std::vector<PointF> vertices;
  int duplicate_of_index{ -1 };
};

struct Description {
  std::filesystem::path filename;
  std::filesystem::path template_filename;
};

struct InputDefinition {
  std::vector<Input> inputs;
  std::vector<Sprite> sprites;
  std::vector<Description> descriptions;
  VariantMap variables;
};

InputDefinition parse_definition(const Settings& settings);
int get_max_slice_count(const Sheet& sheet);

} // namespace
