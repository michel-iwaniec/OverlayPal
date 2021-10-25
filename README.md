# OverlayPal NES image converter

## Where can it be downloaded?

OverlayPal is Free Software, and all the source code is located at https://github.com/michel-iwaniec/OverlayPal

For the less adventurous, automatically built executables can be downloaded from the AppVeyor build site:
https://ci.appveyor.com/project/michel-iwaniec/overlaypal

## So what is this thing?

A program to convert pixel images to NES constraints, by automatically assigning pixel image colors to background / sprite palettes.

![Bernie](testimages/Bernie-converted.png) ![Bernie Converted BG](testimages/Bernie-converted_bg.png) ![Bernie Converted SPR](testimages/Bernie-converted_spr.png)

### Uhmm... why? Ain't a pixel drawing program enough to make NES pixel art?

The NES was a quirky system in terms of graphics. Designed in an era where pixel memory was expensive, it provides only a single graphics mode.

This mode - geared towards games with a scrolling level and individual game objects - provides only two graphics primitives:

* One scrolling background divided into a grid of 16x16 pixel cells. Each of these can use either of 4 different 4-color palettes - with one color shared among all palettes.
* 64 tiny hardware sprites of either 8x8 or 8x16 pixels. Every sprite can use either of 4 different 3-color palettes, and a maximum of 8 horizontally adjacent sprites are allowed.

Both the background and sprites use only a 2-bit color depth, with additional color depth achieved by using aforementioned palettes. These restrictions make colorful pixel art seen on other platforms a challenge to pull off on the NES.

The most colorful full-screen art in the history of the NES tended to used a carefully designed combinations of sprites overlaid on top of background in order to bypass the 3-colors + background limit and simulate more colors.

A dedicated artist who carefully crafts a background with sprite overlay design from the bottom-up - with these limitations in mind - will always achieve the best results. 

But OverlayPal intends to provide an alternative automated top-down method, which takes an arbitrary image and attempts to find a way to partition and duplicate the colors into background and sprite palettes that fits the limitations. Hopefully making the process easier to visualise for artists new to the color quirks of the NES. 

OverlayPal utilises of the CMPL language and CBC library for mathematical modelling / solving to search the large solution space of color combinations effectively.

Besides being an automated conversion flow, OverlayPal also provides multiple debugging views for showing the converted image. These can highlight where the problematic areas occur, allowing artists to fix these problematic areas with minor tweaks that are unlikely to be noticed.

### Loading a file into OverlayPal

Images loaded can be of two types:

1. RGB images. These will be remapped into the NES's 52 colors using a pre-selected color mapping.
2. Indexed-color images, where each color directly represents one of the possible 52 colors the NES can display.

Because the NES Picture Processing Unit generates all its video in the NTSC color space, there is no definite RGB palette. The color could vary significantly  across different TV sets. And the after-market world of emulation shows an equal variation of RGB palette flavors.

If you wish to convert graphics targeting the NES from the start, it is strongly recommended to use alternative (2) and work with the actual NES hardware colors, by disabling the "Color mapping" checkbox.

The "Input" UI also allows selecting which NES color should be treated as background. Which color is chosen can have a drastic effect on the ability to convert an image. Typically you'll want the background color to be the most common color among your grid cells. Black (1D) is a common choice.

#### Adding new palettes to OverlayPal

If you wish to try out converting existing RGB images using a different palette flavor - or simply want to alternate between palettes - OverlayPal supports loading an arbitrary number of .pal files to switch between.

Simply put a 192-byte .pal files (used by most NES emulators) in the "nespalettes" folder and OverlayPal will load this palette and allow using it for both color display and RGB color mapping.

#### Unique colors option

With arbitrary images not created from a specific NES palette flavor, you might find a nusiance in how colors meant to be distinct sometimes get remapped to identical colors, effectively turning the color remapping into a lossy conversion.

To remedy this issue, enabling the "Unique colors" option ensures each unique color will remaing unique in color mapping, at the cost of a poorer preservation of a color's hue.

The "Unique colors" option is NOT recommended if your input image contains just slightly different colors by accident that you actually intended to be the same color.

### Converting an image

Pressing "Convert" will convert the image using the CMPL optimisation solver. The right image will show a busy indicator, and eventually come back with a success or failure to convert. The "Generated Palettes" window will also show the background / sprite palettes of the output image.

![OverlayPal screenshot](/screenshots/Bernie-screenshot.png)

Successfully converted images can then be saved to a PNG file - optionally with different palette filters applied to separate background / overlay(s).

#### Timeout value
The "Timeout" value allows setting the maximum time in seconds to wait for the CMPL solver to complete. Note that as the solving currently happens in two sequential passes, this timeout will actually be waited on twice.

### Setting limits for optimisation

OverlayPal contains settings for the maximum number of background palettes, maximum number of sprite palettes, and maximum number of sprites per scanline. These reflect the hardware limitations of the NES.

Normally you would want to leave these at their default maximum values. However, if you are designing graphics with additional restrictions you can tweak these settings.

### Shift X / Y + Find Optimal

The grid of the background means that shifting pixel art horizontally / vertically by 1-15 pixels can significantly affect the color count of each cell, and make a conversion possible / not possible.

OverlayPal provides settings to do this shift on a loaded image, as well as a button to auto-detect (in truth just guess) the optimal settings.

### Size mode

OverlayPal allows you to globally choose the sizes of sprites as well as background grid cells.

1. Sprites can be 8x16 or 8x8. 8x16 generally requires fewer sprites, while 8x8 sprites may have more success with converting images due to smaller palette granularity.
2. Backgrounds can use 16x16 or 8x8 palette grid cells. 16x16 is the usual hardware limit on the NES, while 8x8 requires additional hardware such as the MMC5 mapper.

### Show/hide palette colors

These checkboxes allow you to show and hide pixels in the converted image on a per-palette basis.
Additionally they also affect the saved image, allowing you to save the background / sprite overlay(s) as separate images.

### Grid Cell Debug Mode

This allows showing the colors in the converted image.

The right-most radio buttons allow switching between two display modes for BG or overlay:

* Debug BG: Enables debugging numbers for the background, along with the background grid.
* Debug SPR: Enables debugging for sprites, along with boxes around them.

The left-most radio buttons allow switching between what numbers are displayed

* Off: Show no numbers
* Number of colors: Shows the color count for each background grid cell / hardware sprite (excluding the common background color)
* Palette colors: Show the individual NES colors the grid cell / hardware uses
* Palette indices: Shows the same colors remapped to an index in the Generated 32-color palette (8 sub-palettes)
* Attributes: Shows the attribute number of the grid cell / hardware sprite (i.e., which of the 4 sub-palettes it is using)

### Save converted PNG...

This allows you to (with palette display settings applied). The saved image will be an indexed-color image, with 32 color entries directly representing the NES palette.

### Export...

This allows exporting the converted image (with palette display settings applied) to the binary formats used by the NES hardware and other NES graphics editing tools.

Namely, the following files are saved:
* [filename].nam file - The 1kB NES nametable data storing tile indices and 16x16 attributes selecting palettes
* [filename].exram - Extended bits for tile indices and 8x8 attributes, matching the MMC5 exRAM layout
* [filename]_bg.chr - The character data for the background layer
* [filename]_spr.chr - The character data for the sprite layer
* [filename].oam - The tile indices, positions and palette selection for sprites, encoded in NES Object Attribute Memory format
* [filename]_palette.dat - The 32-entry palette representing NES PPU colors

The dialog box will query you for the name of the .nam file, and derive the other filenames accordingly.

### Using OverlayPal as a background verifier task

In order to provide an uninterrupted flow for artists working in their favorite pixel program, OverlayPal can detect file changes on disk and trigger a conversion automatically. This allows working on an image in a paint program, and only glancing at the OverlayPal window to verify that a conversion is still possible.

To enable this mode, simple check these two checkboxes:

* "Track file" on the leftmost UI box. This will detect changes to the image on disk.
* "Automatic". This will trigger a conversion whenever the input image has changed.
