#include "cutout.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace negiysem {

namespace {

// Distance from the background color below which a pixel is fully
// transparent, and above which it is fully opaque; in between the alpha
// ramps linearly. Values are Euclidean RGB distances (0..441).
constexpr double kTransparentBelow = 40.0;
constexpr double kOpaqueAbove = 95.0;

struct Rgb {
    double r = 0, g = 0, b = 0;
};

double distance(const unsigned char* px, const Rgb& bg) {
    const double dr = px[0] - bg.r;
    const double dg = px[1] - bg.g;
    const double db = px[2] - bg.b;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

// The chroma color is whatever dominates the border: per-channel median of
// a two-pixel ring, which shrugs off JPEG noise and stray garment pixels.
Rgb estimateBackground(const unsigned char* rgba, int w, int h) {
    std::vector<unsigned char> r, g, b;
    const auto sample = [&](int x, int y) {
        const unsigned char* px = rgba + 4 * (y * w + x);
        r.push_back(px[0]);
        g.push_back(px[1]);
        b.push_back(px[2]);
    };
    const int ring = std::min(2, std::min(w, h));
    for (int y = 0; y < h; ++y) {
        const bool edge_row = y < ring || y >= h - ring;
        for (int x = 0; x < w; ++x) {
            if (edge_row || x < ring || x >= w - ring) sample(x, y);
        }
    }
    const auto median = [](std::vector<unsigned char>& v) {
        std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
        return static_cast<double>(v[v.size() / 2]);
    };
    return {median(r), median(g), median(b)};
}

// Crops the RGBA image to the pixels with alpha above `alpha_floor` plus a
// small margin and encodes the result as a PNG. Throws when everything is
// transparent.
std::string cropAndEncode(const unsigned char* rgba, int w, int h, int alpha_floor) {
    int min_x = w, min_y = h, max_x = -1, max_y = -1;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (rgba[4 * (y * w + x) + 3] > alpha_floor) {
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }
        }
    }
    if (max_x < 0) {
        throw std::runtime_error("image is fully transparent");
    }
    const int margin = std::max(w, h) / 25;
    min_x = std::max(0, min_x - margin);
    min_y = std::max(0, min_y - margin);
    max_x = std::min(w - 1, max_x + margin);
    max_y = std::min(h - 1, max_y + margin);
    const int cw = max_x - min_x + 1;
    const int ch = max_y - min_y + 1;
    std::vector<unsigned char> cropped(static_cast<size_t>(cw) * ch * 4);
    for (int y = 0; y < ch; ++y) {
        std::copy_n(rgba + 4 * ((min_y + y) * w + min_x), static_cast<size_t>(cw) * 4,
                    cropped.data() + 4 * static_cast<size_t>(y) * cw);
    }

    std::string png;
    stbi_write_png_to_func(
        [](void* context, void* data, int size) {
            static_cast<std::string*>(context)->append(static_cast<const char*>(data),
                                                       static_cast<size_t>(size));
        },
        &png, cw, ch, 4, cropped.data(), cw * 4);
    if (png.empty()) {
        throw std::runtime_error("could not encode cutout PNG");
    }
    return png;
}

unsigned char* decodeRgba(const std::string& image_bytes, int* w, int* h) {
    int channels = 0;
    unsigned char* decoded = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(image_bytes.data()),
        static_cast<int>(image_bytes.size()), w, h, &channels, 4);
    if (decoded == nullptr) {
        throw std::runtime_error(std::string("could not decode image: ") +
                                 stbi_failure_reason());
    }
    return decoded;
}

}  // namespace

std::string removeChromaBackground(const std::string& image_bytes) {
    int w = 0, h = 0;
    unsigned char* decoded = decodeRgba(image_bytes, &w, &h);
    const std::unique_ptr<unsigned char, void (*)(void*)> guard(decoded, stbi_image_free);

    const Rgb bg = estimateBackground(decoded, w, h);

    size_t transparent = 0;
    for (int i = 0; i < w * h; ++i) {
        unsigned char* px = decoded + 4 * i;
        const double d = distance(px, bg);
        double alpha;
        if (d <= kTransparentBelow) {
            alpha = 0.0;
        } else if (d >= kOpaqueAbove) {
            alpha = 1.0;
        } else {
            alpha = (d - kTransparentBelow) / (kOpaqueAbove - kTransparentBelow);
        }
        if (alpha == 0.0) {
            transparent += 1;
            px[0] = px[1] = px[2] = 0;
            px[3] = 0;
            continue;
        }
        // Semi-transparent edge pixels are a mix of subject and chroma; undo
        // the mix so no colored fringe survives on the new background.
        if (alpha < 1.0) {
            for (int c = 0; c < 3; ++c) {
                const double bg_c = c == 0 ? bg.r : (c == 1 ? bg.g : bg.b);
                const double unmixed = (px[c] - (1.0 - alpha) * bg_c) / alpha;
                px[c] = static_cast<unsigned char>(std::clamp(unmixed, 0.0, 255.0));
            }
        }
        px[3] = static_cast<unsigned char>(std::lround(alpha * 255.0));
    }

    // A generated chroma shot is mostly background. When nearly nothing keyed
    // out, the model ignored the chroma instruction — better to fail loudly
    // than to store an opaque rectangle as a "cutout".
    if (transparent < static_cast<size_t>(w) * h / 10) {
        throw std::runtime_error("no chroma background detected");
    }

    return cropAndEncode(decoded, w, h, /*alpha_floor=*/0);
}

std::string trimCutout(const std::string& png_bytes) {
    int w = 0, h = 0;
    unsigned char* decoded = decodeRgba(png_bytes, &w, &h);
    const std::unique_ptr<unsigned char, void (*)(void*)> guard(decoded, stbi_image_free);
    // Ignore faint segmentation haze when finding the subject.
    return cropAndEncode(decoded, w, h, /*alpha_floor=*/8);
}

}  // namespace negiysem
