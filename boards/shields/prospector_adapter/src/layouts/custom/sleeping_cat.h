#pragma once

#include <stdint.h>

/*
 * 16×16 pixel-art sleeping cat sprite.
 * Rendered at CAT_SCALE× for a crisp retro look.
 *
 * Palette indices:
 *   0 = transparent (black background)
 *   1 = outline      (dark blue-gray)
 *   2 = body fill    (medium blue-gray)
 *   3 = highlight    (light blue-gray)
 *   4 = accent/pink  (inner ears, nose)
 */

#define CAT_ART_W  16
#define CAT_ART_H  16
#define CAT_SCALE  3
#define CAT_RENDER_W (CAT_ART_W * CAT_SCALE)  /* 48 */
#define CAT_RENDER_H (CAT_ART_H * CAT_SCALE)  /* 48 */
#define CAT_NUM_FRAMES 2
#define CAT_PALETTE_SIZE 5

static const uint32_t cat_palette[CAT_PALETTE_SIZE] = {
    0x000000, /* 0: background */
    0x2A3A4A, /* 1: outline    */
    0x506878, /* 2: body       */
    0x708898, /* 3: highlight  */
    0x806070, /* 4: pink accent */
};

/*
 * Two frames for a subtle breathing animation.
 * Frame 0 – resting
 * Frame 1 – body highlight shimmer (inhale)
 *
 *  Visual key (frame 0):
 *
 *    ..##......##....     ear tips
 *    .#@@#....#@@#...     ears
 *    #@P@@####@@P@#..     inner ears (P=pink)
 *    #@@oo@@@@oo@@#..     forehead (o=highlight)
 *    #@oo^o@@o^oo@#..     eyes  ^ = closed-eye peaks
 *    #@o^o^@@^o^o@#..     eyes    = closed-eye bases
 *    #@@oo@@@@oo@@#..     cheeks
 *    .#@@@@P@@@@@#...     nose
 *    .#@@@o@o@@@@#...     mouth
 *    ..#@@@@@@@@#....     chin
 *    ..#@o@@@@o@#....     paws
 *    .#@@@@@@@@@@#...     body
 *    #@@@@@@@@@@@@#..     body wider
 *    #@@@@@@@@@@@@@#.     body + tail
 *    .#@@@@@@@@@@@@#.     lower body
 *    ..###########...     bottom
 */
static const uint8_t cat_frames[CAT_NUM_FRAMES][CAT_ART_H][CAT_ART_W] = {
    /* ---- Frame 0: resting ---- */
    {
        {0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0},
        {0,1,2,2,1,0,0,0,0,1,2,2,1,0,0,0},
        {1,2,4,2,2,1,1,1,1,2,2,4,2,1,0,0},
        {1,2,2,3,3,2,2,2,2,3,3,2,2,1,0,0},
        {1,2,3,3,1,3,2,2,3,1,3,3,2,1,0,0},
        {1,2,3,1,3,1,2,2,1,3,1,3,2,1,0,0},
        {1,2,2,3,3,2,2,2,2,3,3,2,2,1,0,0},
        {0,1,2,2,2,2,4,2,2,2,2,2,1,0,0,0},
        {0,1,2,2,2,3,2,3,2,2,2,2,1,0,0,0},
        {0,0,1,2,2,2,2,2,2,2,2,1,0,0,0,0},
        {0,0,1,2,3,2,2,2,2,3,2,1,0,0,0,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    },
    /* ---- Frame 1: breathing (body highlights shift) ---- */
    {
        {0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0},
        {0,1,2,2,1,0,0,0,0,1,2,2,1,0,0,0},
        {1,2,4,2,2,1,1,1,1,2,2,4,2,1,0,0},
        {1,2,2,3,3,2,2,2,2,3,3,2,2,1,0,0},
        {1,2,3,3,1,3,2,2,3,1,3,3,2,1,0,0},
        {1,2,3,1,3,1,2,2,1,3,1,3,2,1,0,0},
        {1,2,2,3,3,2,2,2,2,3,3,2,2,1,0,0},
        {0,1,2,2,2,2,4,2,2,2,2,2,1,0,0,0},
        {0,1,2,2,2,3,2,3,2,2,2,2,1,0,0,0},
        {0,0,1,2,2,2,2,2,2,2,2,1,0,0,0,0},
        {0,0,1,2,3,2,2,2,2,3,2,1,0,0,0,0},
        {0,1,2,2,3,2,2,2,2,3,2,2,1,0,0,0},
        {1,2,2,3,2,2,2,2,2,2,3,2,2,1,0,0},
        {1,2,2,2,3,2,2,2,2,3,2,2,2,2,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    },
};
