# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Version 4.0.0] - 2025-12-22

### Changed

- Renamed `*-bounds` definitions to `*-size`.
- Changed meaning of `rect` in the output description. It used to be the logical bounds of the sprite. Now it is the rectangle the sprite takes on the sheet (which can be bigger than `trimmedRect`, when the size is increased by `min-size`, `divisible-size`, `common-size`).
- The logical bounds of a sprite can now be defined by a `margin`, which is added to `rect`.
- Renamed `vertices` to `outline` in output description and made it relative to `rect` (it used to be relative to the `trimmedRect`).

### Added

- Added `margin` definition.
- Added the sheets' `outputs` to description.

### Fixed

- Fixed `divisible-width`.
- Fixed `trimmedSourceRect` in description when transforming.
- Fixed `align-pivot` when anchor is not top/left.
- Fixed alignment when sprite is rotated.
- Fixed `extrude` and uncropped pivot-point.
- Fixed `scale` when downsampling to nearest.
- Improved `rotate` accuracy.
- Fixed `trim convex`.
- Fixed crash on `duplicate` before first sprite.
- Prevent `grid`/`atlas` in input sequences.
- Ensuring invalid definitions do not lead to invalid states.

## [Version 3.8.0] - 2025-11-21

### Added

- Added `grid-vertical` and `grid-cells-vertical`.
- Allow to set different `trim-margin` for each side (#25).
- Added `resize` transformation (#26).

### Fixed

- Allow to set `max-sprites` to 0.
- Fixed `atlas` detecting sprites fully enclosed in other sprites.

## [Version 3.7.0] - 2025-11-03

### Added

- Allow to split lines with semicolons.
- Transformations can also be applied to outputs.
- Outputting sheets in description.
- Loading image data only on demand.
- Added CSS template.

### Changed

- Generating ordered JSON description.
- Renamed input property `sources` to `sourceSprites` in description.
- Renamed scale filters `triangle`/`pointsample` to `bilinear`/`nearest`.

### Fixed

- Fixed index of duplicated sprite (#24).
- Fixed tags of duplicated sprite (#24).
- Not keeping transforms when duplicating sprite.
- Fixed globbing to only add not yet added inputs.
- Improved GIF output.

## [Version 3.6.1] - 2025-05-03

### Added

- Added `resize` definition.
- Added `rotate` definition.
- Added `transform` definition.
- Added `duplicate` definition.
- Added shortcut argument -c for '--mode complete'.

## [Version 3.5.4] - 2025-01-04

### Addded

- Looking for templates also in share/spright directory (#18).

### Fixed

- Fixed case of package name causing problems with spright-vscode installation on Linux (houmain/spright-vscode#3).

## [Version 3.5.3] - 2024-07-28

### Added

- Added reason to packing sprite failed warning.

### Fixed

- Fixed installation on MacOS.

## [Version 3.5.2] - 2024-07-28

### Added

- Added .DEB and .RPM package releases.

### Changed

- glob only adds not yet encountered inputs.

## [Version 3.5.1] - 2024-07-25

### Added

- Allow filename sequence for description to output each slice separately (#14).
- Substitute variable in template filename.

### Fixed

- Fixed variable source.filenameBase to contain no extension.

## [Version 3.5.0] - 2024-06-17

### Added

- Allow `grid` cell size of one dimension to be 0 (to maximize).
- Added function `removeDirectories`.
- Added function `joinPaths`.

### Changed

- Not merging sequences in `glob` when `grid` or `atlas` is active.

### Fixed

- Fixed # in string literals.
- Fixed `grid` when globbing.
- Preventing `grid` and `atlas` in `input` file sequences.
- `grid-cells` is considering `grid-spacing`.

## [Version 3.4.0] - 2024-06-15

### Added

- Added function `makeId`.
- Added function `base64`.
- Added variables `source.filenameBase`, `source.filenameStem`, and `source.filenameId`.

### Changed

- Separated `texture.path` from `texture.filename`.
- Also lookup templates in "templates" subdirectory.
- Also packaging templates.

### Fixed

- Improved pixijs.inja template.

## [Version 3.3.1] - 2024-06-06

### Added

- Added max-sprites definition.
- Added Cocos2d-x template.

### Changed

- Abort globbing when pattern is too coarse.

### Fixed

- Ensure the lowest index duplicate sprite is the one that is kept.
- globbing does not always search subdirectories.

## [Version 3.3.0] - 2023-05-28

### Added

- Added pack method origin.
- Added alpha mode opaque.
- Added color parameter to alpha mode clear.

### Changed

- Ignoring unsupported extension when globbing with '.\*'.
- Skip output of descriptions without filename.

### Fixed

- Properly handling missing closing quote.
- Expand sprite bounds by over-alignment.

## [Version 3.2.0] - 2023-05-23

### Added

- Added PixiJS template.
- Replacing variables in tag values.

### Changed

- Renamed template files to .inja.

### Fixed

- Fixed output path for descriptions.
- Fixed redundancy check when outputting multiple descriptions.

## [Version 3.1.0] - 2023-05-20

### Added

- Added default sheet.
- Added description and template definitions.

### Changed

- Reporting unpackable sprites as warnings.
- y-parameter of grid is no longer optional.
- Atlas completion does not add skipped.

## [Version 3.0.0] - 2023-05-07

### Changed

- Added _glob_, removed _source_ definition.
- Replaced several commandline parameters with single run mode parameter.
- Outputting sprite indices by input/source.
- Checking sequence bounds.

### Added

- Autocompletion can update.
- Allow to autocomplete only specific inputs.
- Generalized _skip_ to atlas and sequences.
- Allow to specify grid's offset from the bottom-right corner.

## [Version 2.3.0] - 2023-03-24

### Added

- Added describe commandline option.
- Added autocompletion for globbing and sequences.
- Added inputs to output description.
- Added errors as warnings option.

### Changed

- Replaced debug commandline option with definition.
- Outputting either autocompleted definition or output description.
- No longer supporting multiple input files.

### Fixed

- Filename sequence improved.

## [Version 2.2.0] - 2023-03-03

### Added

- Added gif output.
- Added min-bounds.
- Added common-bounds.
- Added align.
- Added align-pivot.
- Allow to separate arguments by comma.

### Changed

- Renamed align-width to divisible-width.
- Renamed common-divisor to divisible-bounds.

## [Version 2.1.0] - 2023-02-24

### Fixed

- Fixed output scope.
- Fixed expression evaluation.
- Allow unexpanded variables in filename sequences.

## [Version 2.0.0] - 2023-02-22

### Changed

- Split output in sheet and output.
- Removed implicit output.
- Renamed layers to maps.
- Added pack mode layers.
- Replaced scalings with scale.

### Added

- Parallelized trimming.
- Added texture to JSON output.

## [Version 1.9.0] - 2023-02-12

### Added

- Added sources to JSON output.
- Added set for setting variables.
- Added data for setting per sprite data.
- Added variable evaluation.

### Changed

- Improved tag output.
- Improved output order.
- Compacting number arrays in JSON output.
- Compacted vertices in JSON output.

### Fixed

- Fixed pack method keep for multiple sources.
- Fixed deduplication when sprites are reordered.

## [Version 1.8.0] - 2023-02-07

### Added

- Not updating outputs when sources did not change.
- Added _scalings_.
- Added extrude modes _mirror_ and _repeat_.
- Added pack methods _rows_ and _columns_.
- Added output of _inputFilename_.

### Changed

- Removed -- commandline argument.

### Fixed

- Fixed debug output of vertices when rotated.

## [Version 1.7.0] - 2023-01-26

### Added

- Allow to set grid and grid-cells at once.
- Improved debug overlay.
- Support writing BMP.

## [Version 1.6.0] - 2023-01-21

### Added

- Created github.com/houmain/spright-test-suite.
- Detecting definitions without effects.
- Updating output files only when they change.
- Displaying input filename in error message.
- Support writing TGA.
- Added crop-pivot.

### Changed

- Changed span's scope from input to sprite.
- Removed support for reading JPEG, PSD and HDR.
- Outputting layers in parallel.

### Fixed

- Ignore layers during file globbing.
- Fixed appending output path to output files.

## [Version 1.5.0] - 2022-11-14

### Added

- Added grid-cells.

### Changed

- Cleanup of JSON output.

### Fixed

- Fixed bug in pack mode compact.
- Fixed pack mode compact without trim convex.
- Fixed allow-rotate with pack mode compact.
- Fixed Visual Studio compile flags for catch2.

## [Version 1.4.0] - 2022-10-15

### Added

- Added layers (issue #4).
- Support mixed number and word pivot declarations. (e.g. pivot center 8)
- Allow to offset pivot points (e.g. pivot top + 3 right - 5)

### Changed

- Removed old definition names sheet/texture for input/output.
- Improved invalid argument count error messages.

### Fixed

- Fixed -- commandline arguments.
- Fixed sample cpp.template.

[version 4.0.0]: https://github.com/houmain/spright/compare/3.8.0...4.0.0
[version 3.8.0]: https://github.com/houmain/spright/compare/3.7.0...3.8.0
[version 3.7.0]: https://github.com/houmain/spright/compare/3.6.1...3.7.0
[version 3.6.1]: https://github.com/houmain/spright/compare/3.5.4...3.6.1
[version 3.5.4]: https://github.com/houmain/spright/compare/3.5.3...3.5.4
[version 3.5.3]: https://github.com/houmain/spright/compare/3.5.2...3.5.3
[version 3.5.2]: https://github.com/houmain/spright/compare/3.5.1...3.5.2
[version 3.5.1]: https://github.com/houmain/spright/compare/3.5.0...3.5.1
[version 3.5.0]: https://github.com/houmain/spright/compare/3.4.0...3.5.0
[version 3.4.0]: https://github.com/houmain/spright/compare/3.3.1...3.4.0
[version 3.3.1]: https://github.com/houmain/spright/compare/3.3.0...3.3.1
[version 3.3.0]: https://github.com/houmain/spright/compare/3.2.0...3.3.0
[version 3.2.0]: https://github.com/houmain/spright/compare/3.1.0...3.2.0
[version 3.1.0]: https://github.com/houmain/spright/compare/3.0.0...3.1.0
[version 3.0.0]: https://github.com/houmain/spright/compare/2.3.0...3.0.0
[version 2.3.0]: https://github.com/houmain/spright/compare/2.2.0...2.3.0
[version 2.2.0]: https://github.com/houmain/spright/compare/2.1.0...2.2.0
[version 2.1.0]: https://github.com/houmain/spright/compare/2.0.0...2.1.0
[version 2.0.0]: https://github.com/houmain/spright/compare/1.9.0...2.0.0
[version 1.9.0]: https://github.com/houmain/spright/compare/1.8.0...1.9.0
[version 1.8.0]: https://github.com/houmain/spright/compare/1.7.0...1.8.0
[version 1.7.0]: https://github.com/houmain/spright/compare/1.6.0...1.7.0
[version 1.6.0]: https://github.com/houmain/spright/compare/1.5.0...1.6.0
[version 1.5.0]: https://github.com/houmain/spright/compare/1.4.0...1.5.0
[version 1.4.0]: https://github.com/houmain/spright/compare/1.3.1...1.4.0
