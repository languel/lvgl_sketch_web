#if 1 /* Set it to "1" to enable the content */

/* Max horizontal and vertical resolution of your display */
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 480

/* Color depth: 16-bit is common for most displays */
#define LV_COLOR_DEPTH 16

/* Enable the Tiny JPEG Decoder */
#define LV_USE_TJPGD 1

/*PNG decoder library*/
#define LV_USE_PNG 1

/* JPG + split JPG decoder library.
 * Split JPG is a custom format optimized for embedded systems. */
#define LV_USE_SJPG 1

/*GIF decoder library*/
#define LV_USE_GIF 1


/* Enable the canvas and image widgets */
#define LV_USE_CANVAS 1
#define LV_USE_IMG 1

/* Enable other features as needed */
#define LV_USE_LABEL 1
#define LV_USE_BTN 1

#endif /* End of content */