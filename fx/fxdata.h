#pragma once

/**** FX data header generated by fxdata-build.py tool version 1.15 ****/

using uint24_t = __uint24;

// Initialize FX hardware using  FX::begin(FX_DATA_PAGE); in the setup() function.

constexpr uint16_t FX_DATA_PAGE  = 0xffef;
constexpr uint24_t FX_DATA_BYTES = 14;

constexpr uint16_t FX_SAVE_PAGE  = 0xfff0;
constexpr uint24_t FX_SAVE_BYTES = 4096;

constexpr uint24_t whatever = 0x000000;
constexpr uint24_t savbuf = 0x00000E;
