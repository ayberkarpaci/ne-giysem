#pragma once

#include <string>

namespace negiysem {

// Turns a product shot on a uniform chroma-key background (any solid color —
// it is estimated from the border pixels) into a transparent RGBA PNG,
// cropped to the subject plus a small margin. Accepts any format stb_image
// can decode (PNG, JPEG, WebP is NOT included). Throws std::runtime_error
// when the bytes cannot be decoded, when no chroma background is detected
// (almost nothing became transparent), or when the subject would vanish
// entirely.
std::string removeChromaBackground(const std::string& image_bytes);

// Crops an already-transparent PNG (e.g. a rembg result, which keeps the
// full photo canvas) down to its subject plus a small margin. Throws when
// the bytes cannot be decoded or everything is transparent.
std::string trimCutout(const std::string& png_bytes);

}  // namespace negiysem
