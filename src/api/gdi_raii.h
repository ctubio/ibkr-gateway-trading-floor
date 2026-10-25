#pragma once

#include <memory>
#include <stdexcept>
#include <string>

template <typename Handle>
struct GdiObjectDeleter {
    using pointer = Handle;

    void operator()(Handle handle) const noexcept {
        if (handle) DeleteObject(reinterpret_cast<HGDIOBJ>(handle));
    }
};

struct DeviceContextDeleter {
    using pointer = HDC;

    void operator()(HDC hdc) const noexcept {
        if (hdc) DeleteDC(hdc);
    }
};

template <typename Handle, typename Deleter>
class unique_gdi_resource {
    std::unique_ptr<void, Deleter> resource_;

public:
    unique_gdi_resource() = default;
    explicit unique_gdi_resource(Handle handle) : resource_(handle) {}

    unique_gdi_resource(const unique_gdi_resource&) = delete;
    unique_gdi_resource& operator=(const unique_gdi_resource&) = delete;
    unique_gdi_resource(unique_gdi_resource&&) noexcept = default;
    unique_gdi_resource& operator=(unique_gdi_resource&&) noexcept = default;

    Handle get() const noexcept { return resource_.get(); }
    operator Handle() const noexcept { return get(); }
    explicit operator bool() const noexcept { return get() != nullptr; }

    void reset(Handle handle = nullptr) noexcept { resource_.reset(handle); }
};

using unique_hbrush = unique_gdi_resource<HBRUSH, GdiObjectDeleter<HBRUSH>>;
using unique_hfont = unique_gdi_resource<HFONT, GdiObjectDeleter<HFONT>>;
using unique_hpen = unique_gdi_resource<HPEN, GdiObjectDeleter<HPEN>>;
using unique_hbitmap = unique_gdi_resource<HBITMAP, GdiObjectDeleter<HBITMAP>>;
using unique_hdc = unique_gdi_resource<HDC, DeviceContextDeleter>;

class GdiplusLifetime {
    ULONG_PTR token_{};

public:
    GdiplusLifetime() {
        Gdiplus::GdiplusStartupInput input;
        const auto status = Gdiplus::GdiplusStartup(&token_, &input, nullptr);
        if (status != Gdiplus::Ok)
            throw std::runtime_error("GDI+ startup failed: " + std::to_string(status));
    }

    GdiplusLifetime(const GdiplusLifetime&) = delete;
    GdiplusLifetime& operator=(const GdiplusLifetime&) = delete;

    ~GdiplusLifetime() {
        if (token_) Gdiplus::GdiplusShutdown(token_);
    }
};
