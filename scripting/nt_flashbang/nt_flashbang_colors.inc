#if defined _flashbangs_colors_included_
  #endinput
#endif
#define _flashbangs_colors_included_

#define MAX_RGB_STRLEN 12
#define MAX_COLOR_STRLEN 24

enum {
  RGB_RED = 0,
  RGB_GREEN,
  RGB_BLUE,
  RGB_ENUM_COUNT
};

enum {
  COLOR_WHITE = 0,
  COLOR_BLACK,
  COLOR_PINK,
  COLOR_PINK_LIGHT,
  COLOR_YELLOW,
  COLOR_YELLOW_LIGHT,
  COLOR_AQUA,
  COLOR_AQUA_LIGHT,
  COLOR_ENUM_COUNT
};

// NOTE: These must match the COLOR enum
int g_iColor_Preset[COLOR_ENUM_COUNT][RGB_ENUM_COUNT] = {
  { 255,  255,  255 },
  { 0,    0,    0   },
  { 255,  127,  255 },
  { 255,  170,  255 },
  { 255,  255,  127 },
  { 255,  255,  170 },
  { 127,  255,  255 },
  { 170,  255,  255 },
};

// NOTE: These must match the COLOR enum
new const String:g_sColorName_Preset[COLOR_ENUM_COUNT][MAX_COLOR_STRLEN] = {
  "White",
  "Black",
  "Pink",
  "Light Pink",
  "Yellow",
  "Light Yellow",
  "Aqua",
  "Light Aqua",
};
