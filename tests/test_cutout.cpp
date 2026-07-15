#include <catch2/catch_test_macros.hpp>

// Declarations only — the implementation lives in cutout.cpp.
#include <stb_image.h>
#include <stb_image_write.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "cutout.h"

namespace {

// A tiny in-memory PNG: `size` x `size` background of color `bg` with a
// centered square of color `fg` covering the middle half.
std::string syntheticShot(int size, const uint8_t bg[3], const uint8_t* fg,
                          int noise = 0) {
    std::vector<uint8_t> rgb(static_cast<size_t>(size) * size * 3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const bool inside = fg != nullptr && x >= size / 4 && x < 3 * size / 4 &&
                                y >= size / 4 && y < 3 * size / 4;
            const uint8_t* color = inside ? fg : bg;
            for (int c = 0; c < 3; ++c) {
                int v = color[c];
                if (!inside && noise > 0) v += (x * 7 + y * 13 + c * 5) % (2 * noise + 1) - noise;
                rgb[3 * (static_cast<size_t>(y) * size + x) + c] =
                    static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
            }
        }
    }
    std::string png;
    stbi_write_png_to_func(
        [](void* context, void* data, int len) {
            static_cast<std::string*>(context)->append(static_cast<const char*>(data),
                                                       static_cast<size_t>(len));
        },
        &png, size, size, 3, rgb.data(), size * 3);
    return png;
}

struct DecodedRgba {
    int w = 0, h = 0;
    std::vector<uint8_t> px;

    const uint8_t* at(int x, int y) const { return px.data() + 4 * (static_cast<size_t>(y) * w + x); }
};

DecodedRgba decode(const std::string& bytes) {
    int w = 0, h = 0, channels = 0;
    uint8_t* data = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(bytes.data()),
                                          static_cast<int>(bytes.size()), &w, &h, &channels, 4);
    REQUIRE(data != nullptr);
    DecodedRgba out;
    out.w = w;
    out.h = h;
    out.px.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return out;
}

const uint8_t kMagenta[3] = {255, 0, 255};
const uint8_t kRed[3] = {200, 30, 40};

}  // namespace

TEST_CASE("removeChromaBackground keys out the background and keeps the subject") {
    const std::string png = syntheticShot(64, kMagenta, kRed);
    const DecodedRgba out = decode(negiysem::removeChromaBackground(png));

    // Cropped to the 32px square plus a small margin — well under the 64px canvas.
    CHECK(out.w < 48);
    CHECK(out.h < 48);

    const uint8_t* center = out.at(out.w / 2, out.h / 2);
    CHECK(center[3] == 255);           // subject stays opaque
    CHECK(static_cast<int>(center[0]) == 200);  // and keeps its color
    CHECK(out.at(0, 0)[3] == 0);       // background corner is transparent
    CHECK(out.at(out.w - 1, out.h - 1)[3] == 0);
}

TEST_CASE("removeChromaBackground survives a noisy background") {
    const std::string png = syntheticShot(64, kMagenta, kRed, /*noise=*/10);
    const DecodedRgba out = decode(negiysem::removeChromaBackground(png));
    CHECK(out.at(0, 0)[3] == 0);
    CHECK(out.at(out.w / 2, out.h / 2)[3] == 255);
}

TEST_CASE("removeChromaBackground rejects an image without a chroma background") {
    // The subject color fills the border too, so nothing keys out.
    const std::string png = syntheticShot(64, kRed, kRed);
    CHECK_THROWS_AS(negiysem::removeChromaBackground(png), std::runtime_error);
}

TEST_CASE("removeChromaBackground rejects an empty chroma-only image") {
    const std::string png = syntheticShot(64, kMagenta, nullptr);
    CHECK_THROWS_AS(negiysem::removeChromaBackground(png), std::runtime_error);
}

TEST_CASE("removeChromaBackground rejects undecodable bytes") {
    CHECK_THROWS_AS(negiysem::removeChromaBackground("not an image"),
                    std::runtime_error);
}

TEST_CASE("trimCutout crops a transparent PNG to its subject") {
    // A transparent canvas with an opaque square in the middle, like a
    // rembg result: full photo size, subject surrounded by alpha 0.
    const int size = 64;
    std::vector<uint8_t> rgba(static_cast<size_t>(size) * size * 4, 0);
    for (int y = size / 4; y < 3 * size / 4; ++y) {
        for (int x = size / 4; x < 3 * size / 4; ++x) {
            uint8_t* px = rgba.data() + 4 * (static_cast<size_t>(y) * size + x);
            px[0] = 200; px[1] = 30; px[2] = 40; px[3] = 255;
        }
    }
    std::string png;
    stbi_write_png_to_func(
        [](void* context, void* data, int len) {
            static_cast<std::string*>(context)->append(static_cast<const char*>(data),
                                                       static_cast<size_t>(len));
        },
        &png, size, size, 4, rgba.data(), size * 4);

    const DecodedRgba out = decode(negiysem::trimCutout(png));
    CHECK(out.w < 48);
    CHECK(out.h < 48);
    CHECK(out.at(out.w / 2, out.h / 2)[3] == 255);
    CHECK(out.at(0, 0)[3] == 0);
}

TEST_CASE("trimCutout rejects a fully transparent image") {
    const int size = 16;
    std::vector<uint8_t> rgba(static_cast<size_t>(size) * size * 4, 0);
    std::string png;
    stbi_write_png_to_func(
        [](void* context, void* data, int len) {
            static_cast<std::string*>(context)->append(static_cast<const char*>(data),
                                                       static_cast<size_t>(len));
        },
        &png, size, size, 4, rgba.data(), size * 4);
    CHECK_THROWS_AS(negiysem::trimCutout(png), std::runtime_error);
}
