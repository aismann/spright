
#include "packing.h"
#include <unordered_set>

namespace spright {

namespace {
  int get_max_size(int size, int max_size, bool power_of_two) {
    if (power_of_two && size)
      size = ceil_to_pot(size);

    if (power_of_two && max_size)
      max_size = floor_to_pot(max_size);

    if (size > 0 && max_size > 0)
      return std::min(size, max_size);
    if (size > 0)
      return size;
    if (max_size > 0)
      return max_size;
    return std::numeric_limits<int>::max();
  }

  template<typename T>
  PointT<T> get_anchor_coordinates(const AnchorT<T>& anchor, const SizeT<T>& size) {
    auto coords = PointT<T>{ anchor };
    switch (anchor.anchor_x) {
      case AnchorX::left:   coords.x += 0; break;
      case AnchorX::center: coords.x += size.x / 2; break;
      case AnchorX::right:  coords.x += size.x; break;
    }
    switch (anchor.anchor_y) {
      case AnchorY::top:    coords.y += 0; break;
      case AnchorY::middle: coords.y += size.y / 2; break;
      case AnchorY::bottom: coords.y += size.y; break;
    }
    return coords;
  }

  void initialize_sprite_size(Sprite& s) {
    const auto size = s.trimmed_source_rect.size();
    s.size.x = std::max(s.min_size.x,
      ceil(size.x + 2 * s.extrude.count, s.divisible_size.x));
    s.size.y = std::max(s.min_size.y,
      ceil(size.y + 2 * s.extrude.count, s.divisible_size.y));
  }

  void update_sprite_alignment(Sprite& s) {
    const auto margin = s.size - s.trimmed_source_rect.size();
    const auto coords = get_anchor_coordinates(s.align, margin);
    s.align.x += coords.x;
    s.align.y += coords.y;

    // clamp to zero
    s.align.x = std::max(s.align.x, 0);
    s.align.y = std::max(s.align.y, 0);

    // expand size by over-alignment
    s.size.x = std::max(s.size.x, s.trimmed_source_rect.w + s.align.x);
    s.size.y = std::max(s.size.y, s.trimmed_source_rect.h + s.align.y);
  }

  void update_aligned_pivot(std::vector<Sprite>& sprites) {
    const auto get_pivot_coords = [](const Sprite& s) {
      const auto pivot_rect = SizeF(s.crop_pivot ? 
        s.trimmed_source_rect.size() : s.source_rect.size());
      return get_anchor_coordinates(s.pivot, pivot_rect);
    };

    auto sprites_by_key = std::map<std::string, std::vector<Sprite*>>();
    for (auto& sprite : sprites)
      if (!sprite.align_pivot.empty())
        sprites_by_key[sprite.align_pivot].push_back(&sprite);

    for (const auto& [key, sprites] : sprites_by_key) {
      auto max_pivot = PointF{
        std::numeric_limits<real>::min(),
        std::numeric_limits<real>::min()
      };
      for (const auto* sprite : sprites) {
        const auto pivot_coords = get_pivot_coords(*sprite);
        max_pivot.x = std::max(max_pivot.x, pivot_coords.x);
        max_pivot.y = std::max(max_pivot.y, pivot_coords.y);
      }
      for (auto* sprite : sprites) {
        const auto pivot_coords = get_pivot_coords(*sprite);
        const auto offset = Point{ 
          round_to_int(max_pivot.x - pivot_coords.x),
          round_to_int(max_pivot.y - pivot_coords.y),
        };
        sprite->align = Anchor{ offset, AnchorX::left,  AnchorY::top };
        sprite->size.x += offset.x;
        sprite->size.y += offset.y;
      }
    }
  }

  void update_common_size(std::vector<Sprite>& sprites) {
    auto sprites_by_key = std::map<std::string, std::vector<Sprite*>>();
    for (auto& sprite : sprites)
      if (!sprite.common_size.empty())
        sprites_by_key[sprite.common_size].push_back(&sprite);

    for (const auto& [key, sprites] : sprites_by_key) {
      auto max_size = Size{ };
      for (const auto* sprite : sprites) {
        max_size.x = std::max(max_size.x, sprite->size.x);
        max_size.y = std::max(max_size.y, sprite->size.y);
      }
      for (auto* sprite : sprites)
        sprite->size = max_size;
    }
  }

  void update_sprite_rect(Sprite& s) {
    s.rect.x = s.trimmed_source_rect.x;
    s.rect.y = s.trimmed_source_rect.y;
    s.rect.w = s.size.x;
    s.rect.h = s.size.y;
  }

  void update_sprite_trimmed_rect(Sprite& s) {
    s.trimmed_rect.x = s.rect.x;
    s.trimmed_rect.y = s.rect.y;
    if (s.sheet && s.sheet->pack != Pack::keep) {
      s.trimmed_rect.x += s.align.x;
      s.trimmed_rect.y += s.align.y;
    }
    s.trimmed_rect.w = s.trimmed_source_rect.w;
    s.trimmed_rect.h = s.trimmed_source_rect.h;
  }

  void update_sprite_margin(Sprite& s) {
    if (s.crop) {
      // crop to trimmed rect
      s.margin.x0 += (s.rect.x0() - s.trimmed_rect.x0());
      s.margin.y0 += (s.rect.y0() - s.trimmed_rect.y0());
      s.margin.x1 += (s.trimmed_rect.x1() - s.rect.x1());
      s.margin.y1 += (s.trimmed_rect.y1() - s.rect.y1());
    }
    else {
      // expand when source had more margin (trimmed-source-rect to source-rect)
      auto source_bounds = RectF(s.source_rect);
      auto bounds = expand(RectF(s.rect), s.margin);
      source_bounds.x -= s.trimmed_source_rect.x;
      source_bounds.y -= s.trimmed_source_rect.y;
      bounds.x -= s.trimmed_rect.x;
      bounds.y -= s.trimmed_rect.y;
      const auto grow_w = std::max(source_bounds.w - bounds.w, 0.0);
      const auto grow_h = std::max(source_bounds.h - bounds.h, 0.0);
      const auto offset_x = std::max(std::min(bounds.x - source_bounds.x, grow_w), 0.0);
      const auto offset_y = std::max(std::min(bounds.y - source_bounds.y, grow_h), 0.0);
      s.margin.x0 += offset_x;
      s.margin.y0 += offset_y;
      s.margin.x1 += grow_w - offset_x;
      s.margin.y1 += grow_h - offset_y;
    }

    // ensure non-negative bounds
    if (s.margin.x0 + s.margin.x1 <= -s.rect.w)
      s.margin.x0 = s.margin.x1 = -to_real(s.rect.w) / 2;
    if (s.margin.y0 + s.margin.y1 <= -s.rect.h)
      s.margin.y0 = s.margin.y1 = -to_real(s.rect.h) / 2;

    // when rotated correct trimmed-rect afterwards (found out empirically)
    if (s.rotated) {
      const auto margin = s.size - s.trimmed_source_rect.size();
      s.trimmed_rect.x += -s.align.x + (margin.y - s.align.y);
      s.trimmed_rect.y += -s.align.y + s.align.x;
    }
  }

  void update_sprite_pivot_point(Sprite &s) {
    const auto pivot_rect = SizeF(s.crop_pivot ? s.trimmed_rect.size() : s.rect.size());
    const auto pivot_coords = get_anchor_coordinates(s.pivot, pivot_rect);
    s.pivot.x = pivot_coords.x;
    s.pivot.y = pivot_coords.y;
    if (s.crop_pivot) {
      s.pivot.x += s.align.x;
      s.pivot.y += s.align.y;
    }
  }

  void pack_slice(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
    assert(!sprites.empty());

    switch (sheet->pack) {
      case Pack::binpack: return pack_binpack(sheet, sprites, slices, sprites.size() > 1000);
      case Pack::compact: return pack_compact(sheet, sprites, slices);
      case Pack::single: return pack_single(sheet, sprites, slices);
      case Pack::keep: return pack_keep(sheet, sprites, slices);
      case Pack::rows: return pack_lines(sheet, sprites, slices, true);
      case Pack::columns: return pack_lines(sheet, sprites, slices, false);
      case Pack::origin: return pack_origin(sheet, sprites, slices, false);
      case Pack::layers: return pack_origin(sheet, sprites, slices, true);
    }
  }

  void pack_slice_deduplicate(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
    assert(!sprites.empty());

    // sort duplicates to back
    auto unique_sprites = sprites;
    for (auto i = sprites.size() - 1; ; --i) {
      for (auto j = size_t{ }; j < i; ++j) {
        if (is_identical(sprites[i].source->image(), sprites[i].trimmed_source_rect,
                         sprites[j].source->image(), sprites[j].trimmed_source_rect)) {
          sprites[i].duplicate_of_index = sprites[j].index;
          std::swap(sprites[i], unique_sprites.back());
          unique_sprites = unique_sprites.first(unique_sprites.size() - 1);
          break;
        }
      }
      if (i == 0)
        break;
    }

    // restore order of unique sprites before packing
    std::sort(unique_sprites.begin(), unique_sprites.end(),
      [](const Sprite& a, const Sprite& b) { return (a.index < b.index); });

    pack_slice(sheet, unique_sprites, slices);

    const auto duplicate_sprites = sprites.last(sprites.size() - unique_sprites.size());
    if (sheet->duplicates == Duplicates::drop) {
      for (auto& sprite : duplicate_sprites)
        sprite.sheet = { };
    }
    else {
      // copy rectangles from unique to duplicate sprites
      auto sprites_by_index = std::map<int, const Sprite*>();
      for (const auto& sprite : unique_sprites)
        sprites_by_index[sprite.index] = &sprite;
      for (auto& duplicate : duplicate_sprites) {
        const auto& sprite = *sprites_by_index.find(duplicate.duplicate_of_index)->second;
        duplicate.slice_index = sprite.slice_index;
        duplicate.trimmed_rect = sprite.trimmed_rect;
        duplicate.rotated = sprite.rotated;
      }
    }
  }

  std::vector<Slice> pack_sprites_by_sheet(SpriteSpan sprites) {
    if (sprites.empty())
      return { };

    // sort sprites by sheet
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.sheet->index, a.index) <
               std::tie(b.sheet->index, b.index);
      });

    auto slices = std::vector<Slice>();
    for (auto begin = sprites.begin(), it = begin; ; ++it)
      if (it == sprites.end() ||
          it->sheet != begin->sheet) {
        const auto& sheet = begin->sheet;
        if (sheet->duplicates != Duplicates::keep)
          pack_slice_deduplicate(sheet, { begin, it }, slices);
        else
          pack_slice(sheet, { begin, it }, slices);

        if (it == sprites.end())
          break;
        begin = it;
      }
    return slices;
  }

  std::string get_packing_failed_reason(const Sprite& sprite, int slice_count) {
    const auto& sheet = *sprite.sheet;

    const auto [max_width, max_height] = get_slice_max_size(sheet);
    if (sprite.rect.w + sheet.border_padding > max_width)
      return "max-width exceeded";
    if (sprite.rect.h + sheet.border_padding > max_height)
      return "max-height exceeded";

    if (slice_count == get_max_slice_count(sheet)) {
      if (slice_count == 1)
        return "does not fit on single slice";
      return "limited slice count exceeded";
    }
    return "unknown reason";
  }
} // namespace

std::pair<int, int> get_slice_max_size(const Sheet& sheet) {
  return {
    get_max_size(sheet.width, sheet.max_width, sheet.power_of_two),
    get_max_size(sheet.height, sheet.max_height, sheet.power_of_two)
  };
}

std::vector<Slice> pack_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    initialize_sprite_size(sprite);

  // apply alignments which affect size first
  for (auto& sprite : sprites)
    if (!sprite.align_pivot.empty())
      update_sprite_alignment(sprite);
  update_aligned_pivot(sprites);

  // otherwise apply alignments after updating size
  update_common_size(sprites);
  for (auto& sprite : sprites)
    if (sprite.align_pivot.empty())
      update_sprite_alignment(sprite);

  for (auto& sprite : sprites)
    update_sprite_rect(sprite);

  auto slices = pack_sprites_by_sheet(sprites);

  for (auto& sprite : sprites) {
    update_sprite_trimmed_rect(sprite);
    update_sprite_margin(sprite);
    update_sprite_pivot_point(sprite);
  }

  // finish slices
  for (auto i = size_t{ }; i < slices.size(); ++i) {
    auto& slice = slices[i];
    recompute_slice_size(slice);
    slice.index = to_int(i);
  }

  for (const auto& sprite : sprites)
    if (sprite.sheet && sprite.slice_index < 0)
      warning("packing sprite failed: " + 
        get_packing_failed_reason(sprite, to_int(slices.size())),
        sprite.warning_line_number);

  return slices;
}

void create_slices_from_indices(const SheetPtr& sheet_ptr, 
    SpriteSpan sprites, std::vector<Slice>& slices) {

  // sort sprites by slice index
  std::sort(std::begin(sprites), std::end(sprites),
    [](const Sprite& a, const Sprite& b) {
      return std::tie(a.slice_index, a.index) <
             std::tie(b.slice_index, b.index);
    });

  // create slices
  auto begin = sprites.begin();
  const auto end = sprites.end();
  for (auto it = begin;; ++it)
    if (it == end || 
        it->slice_index != begin->slice_index) {

      if (begin->slice_index >= 0)
        slices.push_back({
          sheet_ptr,
          begin->slice_index,
          SpriteSpan(begin, it),
        });

      begin = it;
      if (it == end)
        break;
    }
}

void recompute_slice_size(Slice& slice) {
  const auto& sheet = *slice.sheet;

  auto max_x = 0;
  auto max_y = 0;
  for (const auto& sprite : slice.sprites) {
    max_x = std::max(max_x, sprite.rect.x +
      (sprite.rotated ? sprite.size.y : sprite.size.x));
    max_y = std::max(max_y, sprite.rect.y +
      (sprite.rotated ? sprite.size.x : sprite.size.y));
  }
  slice.width = std::max(sheet.width, max_x + sheet.border_padding);
  slice.height = std::max(sheet.height, max_y + sheet.border_padding);

  if (sheet.divisible_width)
    slice.width = ceil(slice.width, sheet.divisible_width);

  if (sheet.power_of_two) {
    slice.width = ceil_to_pot(slice.width);
    slice.height = ceil_to_pot(slice.height);
  }
  if (sheet.square)
    slice.width = slice.height = std::max(slice.width, slice.height);
}

void update_last_source_written_times(std::vector<Slice>& slices) {
  scheduler.for_each_parallel(slices,
    [](Slice& slice) {
      auto last_write_time = get_last_write_time(slice.sheet->input_file);

      auto sources = std::unordered_set<const ImageFile*>();
      for (const auto& sprite : slice.sprites)
        sources.insert(sprite.source.get());
      for (const auto& source : sources)
        last_write_time = std::max(last_write_time,
          get_last_write_time(source->path() / source->filename()));

      slice.last_source_written_time = last_write_time;
    });
}

} // namespace
