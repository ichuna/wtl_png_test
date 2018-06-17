#pragma once

#include <vector>

class PngDecoder
{
public:
  PngDecoder();
  ~PngDecoder();

  static constexpr int DEFAULT_ZLIB_COMPRESSION = 6;

  enum ColorFormat {
    // 4 bytes per pixel, in RGBA order in memory regardless of endianness.
    FORMAT_RGBA,

    // 4 bytes per pixel, in BGRA order in memory regardless of endianness.
    // This is the default Windows DIB order.
    FORMAT_BGRA,

    // SkBitmap format. For Encode() kN32_SkColorType (4 bytes per pixel) and
    // kAlpha_8_SkColorType (1 byte per pixel) formats are supported.
    // kAlpha_8_SkColorType gets encoded into a grayscale PNG treating alpha as
    // the color intensity. For Decode() kN32_SkColorType is always used.
    FORMAT_SkBitmap
  };


  static bool Decode(const unsigned char* input, size_t input_size,
    ColorFormat format, std::vector<unsigned char>* output,
    int* w, int* h);

};
