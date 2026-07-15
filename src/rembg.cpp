#include "rembg.h"

#include <cpr/cpr.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace negiysem {

namespace {

bool serverAnswers(const std::string& base_url) {
    const cpr::Response r = cpr::Get(cpr::Url{base_url + "/api"}, cpr::Timeout{2000});
    return r.status_code != 0;
}

#ifdef _WIN32
// Ties spawned children to our lifetime: closing the job handle (which the
// OS does when we exit, however we exit) kills everything assigned to it.
HANDLE lifetimeJob() {
    static HANDLE job = [] {
        HANDLE h = CreateJobObjectA(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(h, JobObjectExtendedLimitInformation, &info, sizeof(info));
        return h;
    }();
    return job;
}

bool spawnDetached(const std::string& command_line) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string cmd = command_line;  // CreateProcess wants a mutable buffer
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    AssignProcessToJobObject(lifetimeJob(), pi.hProcess);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}
#else
bool spawnDetached(const std::string& command_line) {
    return std::system((command_line + " >/dev/null 2>&1 &").c_str()) == 0;
}
#endif

}  // namespace

std::string findRembgCommand(const std::string& configured) {
    if (!configured.empty()) return configured;
#ifdef _WIN32
    char found[MAX_PATH];
    if (SearchPathA(nullptr, "rembg", ".exe", MAX_PATH, found, nullptr) > 0) {
        return found;
    }
    // pip installs rembg.exe into the Python Scripts folder, which usually
    // is not on PATH; look through the per-user Python installs.
    const char* local_appdata = std::getenv("LOCALAPPDATA");
    if (local_appdata != nullptr) {
        namespace fs = std::filesystem;
        const fs::path pythons = fs::path(local_appdata) / "Programs" / "Python";
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(pythons, ec)) {
            const fs::path exe = entry.path() / "Scripts" / "rembg.exe";
            if (fs::exists(exe, ec)) return exe.string();
        }
    }
    return "";
#else
    return "rembg";
#endif
}

std::string ensureRembgServer(const std::string& command, int port) {
    const std::string base_url = "http://127.0.0.1:" + std::to_string(port);
    if (serverAnswers(base_url)) return base_url;

    if (command.empty()) {
        throw std::runtime_error(
            "rembg is not installed (run: py -m pip install \"rembg[cli]\" onnxruntime)");
    }
    const std::string command_line = "\"" + command + "\" s -p " + std::to_string(port) +
                                     " -h 127.0.0.1 --no-ui";
    if (!spawnDetached(command_line)) {
        throw std::runtime_error("could not start the rembg server: " + command_line);
    }
    // uvicorn needs a moment to come up (the model loads later, per request).
    for (int attempt = 0; attempt < 60; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (serverAnswers(base_url)) return base_url;
    }
    throw std::runtime_error("the rembg server did not become ready on " + base_url);
}

std::string rembgRemove(const std::string& base_url,
                        const std::string& image_bytes,
                        const std::string& model) {
    // Generous timeout: the first request per model loads it into memory,
    // and may first download it (birefnet-general is ~1 GB, one time only).
    const cpr::Response r = cpr::Post(
        cpr::Url{base_url + "/api/remove"},
        cpr::Parameters{{"model", model}},
        cpr::Multipart{{"file", cpr::Buffer{image_bytes.begin(), image_bytes.end(),
                                            "photo.png"}}},
        cpr::Timeout{600000});
    if (r.status_code == 0) {
        throw std::runtime_error("rembg request failed: " + r.error.message);
    }
    if (r.status_code != 200) {
        throw std::runtime_error("rembg returned HTTP " + std::to_string(r.status_code) +
                                 ": " + r.text.substr(0, 200));
    }
    return r.text;
}

}  // namespace negiysem
