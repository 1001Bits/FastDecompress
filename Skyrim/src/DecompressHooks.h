#pragma once

namespace FastDecompress
{
    // Replaces Skyrim's stock zlib and LZ4 with:
    //   - libdeflate (~2.4x faster) for whole-buffer form decompression
    //   - zlib-ng compat (~2x faster, SIMD) for streaming inflate
    //   - LZ4 v1.10.0 (latest) for BSA asset decompression
    //
    // Supports Skyrim SE (1.5.97), AE (1.6.x), and VR (1.4.15).
    inline constexpr const char* kVersion = "1.4.0";
    void Install();
}
