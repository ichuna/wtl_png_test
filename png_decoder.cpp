
#include "stdafx.h"
#include "logging.h"
#include "png_decoder.h"
#include <atlstr.h>

#include "third_party/libpng/png.h"
#include "third_party/zlib/zlib.h"

namespace {
#ifdef __cplusplus 
  extern "C" {
#endif 
    void writelog(const char *format, ...);
#ifdef __cplusplus 
  }
#endif 

  // Gamma constants: We assume we're on Windows which uses a gamma of 2.2.
  const double kMaxGamma = 21474.83;  // Maximum gamma accepted by png library.
  const double kDefaultGamma = 2.2;
  const double kInverseGamma = 1.0 / kDefaultGamma;

  typedef unsigned U8CPU;
  typedef uint32_t SkPMColor;
  typedef uint32_t SkColor;
  /** return the alpha byte from a SkColor value */
#define SkColorGetA(color)      (((color) >> 24) & 0xFF)
  /** return the red byte from a SkColor value */
#define SkColorGetR(color)      (((color) >> 16) & 0xFF)
  /** return the green byte from a SkColor value */
#define SkColorGetG(color)      (((color) >>  8) & 0xFF)
  /** return the blue byte from a SkColor value */
#define SkColorGetB(color)      (((color) >>  0) & 0xFF)

  typedef unsigned U16CPU;
  U8CPU SkMulDiv255Round(U16CPU a, U16CPU b) {
    unsigned prod = a*b + 128;
    return (prod + (prod >> 8)) >> 8;
  }

#define SK_A32_SHIFT    24
#define SK_R32_SHIFT    16
#define SK_G32_SHIFT    8
#define SK_B32_SHIFT    0
  static inline SkPMColor SkPackARGB32(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {

    return (a << SK_A32_SHIFT) | (r << SK_R32_SHIFT) |
      (g << SK_G32_SHIFT) | (b << SK_B32_SHIFT);
  }

  static inline
    SkPMColor SkPremultiplyARGBInline(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    if (a != 255) {
      r = SkMulDiv255Round(r, a);
      g = SkMulDiv255Round(g, a);
      b = SkMulDiv255Round(b, a);
    }
    return SkPackARGB32(a, r, g, b);
  }

  SkPMColor SkPreMultiplyARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    return SkPremultiplyARGBInline(a, r, g, b);
  }

  class PngDecoderState {
  public:
    // Output is a vector<unsigned char>.
    PngDecoderState(PngDecoder::ColorFormat ofmt, std::vector<unsigned char>* o)
      : output_format(ofmt),
      output_channels(0),
      is_opaque(true),
      output(o),
      width(0),
      height(0),
      done(false) {
    }

    // Output is an SkBitmap.
    explicit PngDecoderState()
      : output_format(PngDecoder::FORMAT_SkBitmap),
      output_channels(0),
      is_opaque(true),
      output(NULL),
      width(0),
      height(0),
      done(false) {
    }

    PngDecoder::ColorFormat output_format;
    int output_channels;

    // An incoming SkBitmap to write to. If NULL, we write to output instead.

    // Used during the reading of an SkBitmap. Defaults to true until we see a
    // pixel with anything other than an alpha of 255.
    bool is_opaque;

    // The other way to decode output, where we write into an intermediary buffer
    // instead of directly to an SkBitmap.
    std::vector<unsigned char>* output;

    // Size of the image, set in the info callback.
    int width;
    int height;

    // Set to true when we've found the end of the data.
    bool done;

  private:
  };


  // User transform (passed to libpng) which converts a row decoded by libpng to
  // Skia format. Expects the row to have 4 channels, otherwise there won't be
  // enough room in |data|.
  void ConvertRGBARowToSkia(png_structp png_ptr,
    png_row_infop row_info,
    png_bytep data) {
    const int channels = row_info->channels;

    PngDecoderState* state =
      static_cast<PngDecoderState*>(png_get_user_transform_ptr(png_ptr));
    PNG_LOG("LibPNG user transform pointer is NULL\n");

    unsigned char* const end = data + row_info->rowbytes;
    for (unsigned char* p = data; p < end; p += channels) {
      uint32_t* sk_pixel = reinterpret_cast<uint32_t*>(p);
      const unsigned char alpha = p[channels - 1];
      if (alpha != 255) {
        state->is_opaque = false;
        *sk_pixel = SkPreMultiplyARGB(alpha, p[0], p[1], p[2]);
      }
      else {
        *sk_pixel = SkPackARGB32(alpha, p[0], p[1], p[2]);
      }
    }
  }


// Holds png struct and info ensuring the proper destruction.
class PngReadStructInfo {
public:
  PngReadStructInfo() : png_ptr_(nullptr), info_ptr_(nullptr) {
  }
  ~PngReadStructInfo() {
    png_destroy_read_struct(&png_ptr_, &info_ptr_, NULL);
  }

  bool Build(const unsigned char* input, size_t input_size) {
    if (input_size < 8) {
      PNG_LOG("_________Build 1 %d\n", (int)input);
      return false;  // Input data too small to be a png
    }

    // Have libpng check the signature, it likes the first 8 bytes.
    if (png_sig_cmp(const_cast<unsigned char*>(input), 0, 8) != 0) {
      PNG_LOG("_________Build 2 %d\n", (int)input);
      return false;
    }

    png_ptr_ = png_create_read_struct(
      PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr_) {
      PNG_LOG("_________Build 3 %d\n", (int)input);
      return false;
    }

    info_ptr_ = png_create_info_struct(png_ptr_);
    if (!info_ptr_) {
      PNG_LOG("_________Build 4 %d\n", (int)input);
      return false;
    }
    return true;
  }

  png_struct* png_ptr_;
  png_info* info_ptr_;
private:
};

void LogLibPNGDecodeError(png_structp png_ptr, png_const_charp error_msg) {
  ATL::CString ss(error_msg);
  ::MessageBox(NULL, ss, L"PNG ERROR", MB_OK);
  longjmp(png_jmpbuf(png_ptr), 1);
}

void LogLibPNGDecodeWarning(png_structp png_ptr, png_const_charp warning_msg) {
  PNG_LOG("libpng decode warning:: %s\n", warning_msg);
}

// Called when the png header has been read. This code is based on the WebKit
// PNGImageDecoder
void DecodeInfoCallback(png_struct* png_ptr, png_info* info_ptr) {
  PngDecoderState* state = static_cast<PngDecoderState*>(
    png_get_progressive_ptr(png_ptr));

  int bit_depth, color_type, interlace_type, compression_type;
  int filter_type;
  png_uint_32 w, h;
  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
    &interlace_type, &compression_type, &filter_type);

  // Bounds check. When the image is unreasonably big, we'll error out and
  // end up back at the setjmp call when we set up decoding.  "Unreasonably big"
  // means "big enough that w * h * 32bpp might overflow an int"; we choose this
  // threshold to match WebKit and because a number of places in code assume
  // that an image's size (in bytes) fits in a (signed) int.
  unsigned long long total_size =
    static_cast<unsigned long long>(w) * static_cast<unsigned long long>(h);
  if (total_size > ((1 << 29) - 1))
    longjmp(png_jmpbuf(png_ptr), 1);
  state->width = static_cast<int>(w);
  state->height = static_cast<int>(h);

  // The following png_set_* calls have to be done in the order dictated by
  // the libpng docs. Please take care if you have to move any of them. This
  // is also why certain things are done outside of the switch, even though
  // they look like they belong there.

  // Expand to ensure we use 24-bit for RGB and 32-bit for RGBA.
  if (color_type == PNG_COLOR_TYPE_PALETTE ||
    (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8))
    png_set_expand(png_ptr);

  // The '!= 0' is for silencing a Windows compiler warning.
  bool input_has_alpha = ((color_type & PNG_COLOR_MASK_ALPHA) != 0);

  // Transparency for paletted images.
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_expand(png_ptr);
    input_has_alpha = true;
  }

  // Convert 16-bit to 8-bit.
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  // Pick our row format converter necessary for this data.
  if (!input_has_alpha) {
    switch (state->output_format) {
    case PngDecoder::FORMAT_RGBA:
      state->output_channels = 4;
      png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
      break;
    case PngDecoder::FORMAT_BGRA:
      state->output_channels = 4;
      png_set_bgr(png_ptr);
      png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
      break;
    case PngDecoder::FORMAT_SkBitmap:
      state->output_channels = 4;
      png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
      break;
    }
  }
  else {
    switch (state->output_format) {
    case PngDecoder::FORMAT_RGBA:
      state->output_channels = 4;
      break;
    case PngDecoder::FORMAT_BGRA:
      state->output_channels = 4;
      png_set_bgr(png_ptr);
      break;
    case PngDecoder::FORMAT_SkBitmap:
      state->output_channels = 4;
      break;
    }
  }

  // Expand grayscale to RGB.
  if (color_type == PNG_COLOR_TYPE_GRAY ||
    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  // Deal with gamma and keep it under our control.
  double gamma;
  if (png_get_gAMA(png_ptr, info_ptr, &gamma)) {
    if (gamma <= 0.0 || gamma > kMaxGamma) {
      gamma = kInverseGamma;
      png_set_gAMA(png_ptr, info_ptr, gamma);
    }
    png_set_gamma(png_ptr, kDefaultGamma, gamma);
  }
  else {
    png_set_gamma(png_ptr, kDefaultGamma, kInverseGamma);
  }

  // Setting the user transforms here (as opposed to inside the switch above)
  // because all png_set_* calls need to be done in the specific order
  // mandated by libpng.
  if (state->output_format == PngDecoder::FORMAT_SkBitmap) {
    png_set_read_user_transform_fn(png_ptr, ConvertRGBARowToSkia);
    png_set_user_transform_info(png_ptr, state, 0, 0);
  }

  // Tell libpng to send us rows for interlaced pngs.
  if (interlace_type == PNG_INTERLACE_ADAM7)
    png_set_interlace_handling(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  if (state->output) {
    state->output->resize(
      state->width * state->output_channels * state->height);
  }
}

void DecodeRowCallback(png_struct* png_ptr, png_byte* new_row,
  png_uint_32 row_num, int pass) {
  if (!new_row)
    return;  // Interlaced image; row didn't change this pass.

  PngDecoderState* state = static_cast<PngDecoderState*>(
    png_get_progressive_ptr(png_ptr));

  if (static_cast<int>(row_num) > state->height) {

    PNG_LOG( "DecodeRowCallback 4 \n");
    return;
  }

  unsigned char* base = NULL;
  if (state->output)
    base = &state->output->front();

  unsigned char* dest = &base[state->width * state->output_channels * row_num];
  png_progressive_combine_row(png_ptr, dest, new_row);
}

void DecodeEndCallback(png_struct* png_ptr, png_info* info) {
  PngDecoderState* state = static_cast<PngDecoderState*>(
    png_get_progressive_ptr(png_ptr));

  // Mark the image as complete, this will tell the Decode function that we
  // have successfully found the end of the data.
  state->done = true;
}

}

PngDecoder::PngDecoder()
{
}


PngDecoder::~PngDecoder()
{
}

bool PngDecoder::Decode(const unsigned char* input, size_t input_size,
  ColorFormat format, std::vector<unsigned char>* output,
  int* w, int* h) {
  PngReadStructInfo si;
  if (!si.Build(input, input_size))
    return false;

  if (setjmp(png_jmpbuf(si.png_ptr_))) {
    // The destroyer will ensure that the structures are cleaned up in this
    // case, even though we may get here as a jump from random parts of the
    // PNG library called below.
    return false;
  }

  PngDecoderState state(format, output);

  png_set_error_fn(si.png_ptr_, NULL,
    LogLibPNGDecodeError, LogLibPNGDecodeWarning);
  png_set_progressive_read_fn(si.png_ptr_, &state, &DecodeInfoCallback,
    &DecodeRowCallback, &DecodeEndCallback);
  png_process_data(si.png_ptr_,
    si.info_ptr_,
    const_cast<unsigned char*>(input),
    input_size);

  if (!state.done) {
    // Fed it all the data but the library didn't think we got all the data, so
    // this file must be truncated.
    output->clear();
    return false;
  }

  *w = state.width;
  *h = state.height;
  return true;
}
