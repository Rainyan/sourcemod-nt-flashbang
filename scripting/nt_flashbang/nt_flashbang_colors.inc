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
  SHADE_MONOCHROME = 0,
  SHADE_ENUM_COUNT
}

enum {
  COLOR_1 = 0,
  COLOR_2,
  COLOR_3,
  COLOR_4,
  COLOR_5,
  COLOR_6,
  COLOR_ENUM_COUNT
};

// NOTE: These must match the COLOR enum
int g_iColor_Preset[SHADE_ENUM_COUNT][COLOR_ENUM_COUNT][RGB_ENUM_COUNT] = {
  // Monochrome
  {
    { 223, 223, 223 },
    { 255, 207, 207 },
    { 207, 255, 207 },
    { 207, 207, 255 },
    { 236, 236, 197 },
    { 236, 197, 236 },
  }
};

new const String:g_sPreset_Shade[SHADE_ENUM_COUNT][] = {
  "Group 1",
};

new const String:g_sPreset_Color[SHADE_ENUM_COUNT][COLOR_ENUM_COUNT][] = {
  // Monochromes
  {
    "White",
    "Red hue",
    "Green hue",
    "Blue hue",
    "Yellow hue",
    "Pink hue",
  },
};

void Test_RGB_Values()
{
  for (int i = 0; i < SHADE_ENUM_COUNT; i++)
  {
    int initialBrightness = g_iColor_Preset[i][0][0] + g_iColor_Preset[i][0][1] + g_iColor_Preset[i][0][2];
    for (int j = 1; j < COLOR_ENUM_COUNT; j++)
    {
      if (initialBrightness != g_iColor_Preset[i][j][0] + g_iColor_Preset[i][j][1] + g_iColor_Preset[i][j][2])
      {
        SetFailState("RGB brightnesses are not equal!");
      }
    }
  }
}
