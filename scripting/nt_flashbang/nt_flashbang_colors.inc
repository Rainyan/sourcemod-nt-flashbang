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
  SHADE_WHITE = 0,
  SHADE_GRAY,
  SHADE_BLACK,
  SHADE_RED,
  SHADE_BROWN,
  SHADE_YELLOW,
  SHADE_GREEN,
  SHADE_BLUE,
  SHADE_ENUM_COUNT
}

enum {
  COLOR_1 = 0,
  COLOR_2,
  COLOR_3,
  COLOR_4,
  COLOR_5,
  COLOR_ENUM_COUNT
};

// NOTE: These must match the COLOR enum
int g_iColor_Preset[SHADE_ENUM_COUNT][COLOR_ENUM_COUNT][RGB_ENUM_COUNT] = {
  // Whites
  {
    { 255,  255,  255 },  // White
    { 234,  224,  200 },  // Pearl
    { 245,  245,  220 },  // Beige
    { 245,  255,  250 },  // Pastel mint
    { 255,  248,  231 },  // Cosmic latte
  },
  // Grays
  {
    { 140,  146,  172 },  // Cool gray
    { 229,  228,  226 },  // Platinum
    { 172,  172,  172 },  // Silver
    {  83,  104,  120 },  // Payne's grey
    { 112,  128,  144 },  // Slate gray
  },
  // Blacks
  {
    {  0,      0,    0  },  // Black
    { 52,     52,   52  },  // Jet black
    { 54,     69,   79  },  // Charcoal
    { 53,     56,   57  },  // Onyx
    { 85,     85,   85  },  // Davy's gray
  },
  // Reds
  {
    { 150,    0,    24  },  // Carmine
    { 255,    0,   127  },  // Rose
    { 255,  192,   203  },  // Pink
    { 218,  112,   214  },  // Orchid
    { 224,  176,   255  },  // Mauve
  },
  // Browns
  {
    { 121,    68,    59  },  // Bole
    { 123,    63,     0  },  // Chocolate
    { 195,   176,   145  },  // Khaki
    {  99,    81,    71  },  // Umber
    { 133,   109,    77  },  // French bistre
  },
  // Yellows
  {
    { 258,    238,    0  },  // Aureolin
    { 255,    215,    0  },  // Gold
    { 128,    128,    0  },  // Olive
    { 251,    206,  177  },  // Apricot
    { 248,    222,  126  },  // Jasmine
  },
  // Greens
  {
    { 34,    139,     34  },  // Forest green
    { 62,    180,    137  },  // Mint
    { 57,    255,     20  },  // Neon green
    {  0,    128,    128  },  // Teal
    { 80,    220,    100  },  // Emerald
  },
  // Blues
  {
    {   0,   127,    255  },  // Azure
    {   0,    49,     83  },  // Prussian blue
    {  64,   130,    109  },  // Viridian
    { 135,   206,    235  },  // Sky blue
    { 204,   204,    255  },  // Periwinkle
  },
};

new const String:g_sPreset_Shade[SHADE_ENUM_COUNT][] = {
  "White",
  "Gray",
  "Black",
  "Red",
  "Brown",
  "Yellow",
  "Green",
  "Blue",
};

new const String:g_sPreset_Color[SHADE_ENUM_COUNT][COLOR_ENUM_COUNT][] = {
  // Whites
  {
    "White",
    "Pearl",
    "Beige",
    "Pastel mint",
    "Cosmic latte",
  },
  // Grays
  {
    "Cool gray",
    "Platinum",
    "Silver",
    "Payne's grey",
    "Slate gray",
  },
  // Blacks
  {
    "Black",
    "Jet black",
    "Charcoal",
    "Onyx",
    "Davy's gray",
  },
  // Reds
  {
    "Carmine",
    "Rose",
    "Pink",
    "Orchid",
    "Mauve",
  },
  // Browns
  {
    "Bole",
    "Chocolate",
    "Khaki",
    "Umber",
    "French bistre",
  },
  // Yellows
  {
    "Aureolin",
    "Gold",
    "Olive",
    "Apricot",
    "Jasmine",
  },
  // Greens
  {
    "Forest green",
    "Mint",
    "Neon green",
    "Teal",
    "Emerald",
  },
  // Blues
  {
    "Azure",
    "Prussian blue",
    "Viridian",
    "Sky blue",
    "Periwinkle",
  },
};
