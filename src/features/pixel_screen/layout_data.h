#pragma once
#include <stdint.h>
namespace pixel_screen { namespace print_layout {
struct Glyph { char ch; uint8_t width, rows[5]; };
static constexpr Glyph font[]={
    {'0', 3, {7, 5, 5, 5, 7}},
    {'1', 3, {2, 6, 2, 2, 7}},
    {'2', 3, {7, 1, 7, 4, 7}},
    {'3', 3, {7, 1, 7, 1, 7}},
    {'4', 3, {5, 5, 7, 1, 1}},
    {'5', 3, {7, 4, 7, 1, 7}},
    {'6', 3, {7, 4, 7, 5, 7}},
    {'7', 3, {7, 1, 2, 2, 2}},
    {'8', 3, {7, 5, 7, 5, 7}},
    {'9', 3, {7, 5, 7, 1, 7}},
    {'A', 3, {2, 5, 7, 5, 5}},
    {'B', 3, {6, 5, 6, 5, 6}},
    {'C', 3, {7, 4, 4, 4, 7}},
    {'D', 3, {6, 5, 5, 5, 6}},
    {'E', 3, {7, 4, 6, 4, 7}},
    {'F', 3, {7, 4, 6, 4, 4}},
    {'H', 3, {5, 5, 7, 5, 5}},
    {'I', 3, {7, 2, 2, 2, 7}},
    {'K', 3, {5, 5, 6, 5, 5}},
    {'L', 3, {4, 4, 4, 4, 7}},
    {'M', 3, {5, 7, 7, 5, 5}},
    {'N', 3, {5, 7, 7, 7, 5}},
    {'O', 3, {7, 5, 5, 5, 7}},
    {'P', 3, {6, 5, 6, 4, 4}},
    {'R', 3, {6, 5, 6, 5, 5}},
    {'S', 3, {7, 4, 7, 1, 7}},
    {'T', 3, {7, 2, 2, 2, 2}},
    {'U', 3, {5, 5, 5, 5, 7}},
    {'W', 3, {5, 5, 7, 7, 5}},
    {'X', 3, {5, 5, 2, 5, 5}},
    {'%', 3, {5, 1, 2, 4, 5}},
    {'/', 3, {1, 1, 2, 4, 4}},
    {':', 1, {0, 1, 0, 1, 0}},
    {'-', 3, {0, 0, 7, 0, 0}},
    {' ', 3, {0, 0, 0, 0, 0}},
    {'G', 3, {7, 4, 5, 5, 7}},
    {'J', 3, {1, 1, 1, 5, 7}},
    {'Q', 3, {7, 5, 5, 7, 1}},
    {'V', 3, {5, 5, 5, 5, 2}},
    {'Y', 3, {5, 5, 2, 2, 2}},
    {'Z', 3, {7, 1, 2, 4, 7}},
};
static constexpr uint8_t progressRainbow[]={8, 14, 6, 12, 11, 10, 9};
static constexpr unsigned rainbowBandPixels=2;
static constexpr unsigned frameMs=100, headerHold=40, scrollHold=8, scrollStep=2, minimumCycleFrames=120;
enum : uint8_t { LayerTop=32, LayerBottom=33, NozzleTop=34, NozzleBottom=35, BedTop=36, BedBottom=37, NumberBottom=38, SeparatorTop=39, SeparatorBottom=40, PrepLabelTop=41, PrepLabelBottom=42, PrepNumberTop=43, PrepNumberBottom=44, PrepSeparatorTop=45, PrepSeparatorBottom=46, PrintGreen=47, PrepareYellow=48 };
} }
