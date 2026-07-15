#pragma once

#include <string>

namespace negiysem {

// Client for a local rembg server (https://github.com/danielgatis/rembg) —
// free, open-source background removal that runs entirely on this machine.
// The server loads the model once and then strips a photo in well under a
// second, so one long-lived process beats spawning the CLI per photo.

// Makes sure a rembg server is listening on 127.0.0.1:`port`, spawning
// `command` ("<path-to-rembg> s -p <port> ...") when nothing answers. The
// child is tied to this process (Windows job object), so it dies with us.
// Returns the base URL, e.g. "http://127.0.0.1:7101". Throws
// std::runtime_error when the server cannot be started or never gets ready.
std::string ensureRembgServer(const std::string& command, int port);

// The rembg CLI executable to spawn: the REMBG_COMMAND config value if set,
// otherwise rembg.exe found on PATH or in the usual Python install spots.
// Empty when nothing is found (rembg is not installed).
std::string findRembgCommand(const std::string& configured);

// Removes the background of one photo: returns a full-canvas transparent
// PNG. `model` picks the segmentation model (e.g. "birefnet-general").
// The first call per model is slow (the server loads, and may first
// download, the model); later calls are fast. Throws on failure.
std::string rembgRemove(const std::string& base_url,
                        const std::string& image_bytes,
                        const std::string& model);

}  // namespace negiysem
