# OpenZoo GBA port - technical documentation

## Timer usage

  * Timer 0 - PIT emulation timer, runs at ~18.2 Hz
  * Timer 1 - sound timer, used dynamically depending on note length
  * Timer 2, 3 - used for measuring CPU time if DEBUG_CONSOLE enabled

## Video

Mode 0 is used, with all four background layers enabled.

  * BG0 - background tiles, odd X positions
  * BG1 - background tiles, even X positions
  * BG2 - foreground tiles, odd X positions
  * BG3 - foreground tiles, even X positions

BG0 and BG2 are scrolled by 4 pixels to the left. By storing the glyph tile data aligned to the right,
this allows a 4-wide text mode. In addition, 8-wide glyphs can be used at the same time by placing them
only on BG1 and BG3's map data.

For glyph heights smaller than 8, HDMA is used to scroll the backgrounds up, creating the illusion of
a smaller tile height by skipping the unused lines. Notably, this requires a 2.5KB (160 x 16 bytes) buffer.
An IRQ approach was tried initially, but is too slow on real hardware.

As scrolling is handled by the OpenZoo game engine and always coarse, the maps are set to the smallest
allowed size - 32x32. The unused area at the bottom can be repurposed for a debug console.

All four background layers point to the same tile data. For background tiles, tile #219 is used - in
CP437, this is the all-solid tile.

The backgrounds operate in 16-color mode, with tiles always set to use color 1 as foreground. The sixteen
palettes correspond to each color of PC text mode.

Emulating PC text mode blinking is accomplished by storing two copies of two copies of the tile data.
The first copy contains two repetitions of the character set; the second copy omits the latter, replacing
it with blank space. This means that all that is required to flip is changing the tile data base in the
background control registers. Unfortunately, this means that fitting both the map data and multiple
copies of the blinking-supporting character set is not possible.

### VRAM layout

 * 00000 - 07FFF: 4x6 graphics mode (w/ blinking) characters
     * chars 0-255: charset, not blinking
     * chars 256-511: charset, blinking, visible
     * chars 512-767: [blink] charset, not blinking
     * chars 768-1023: [blink] empty space, not visible
 * 08000 - 0BFFF: 4x8/8x8 combo graphics mode (w/o blinking) characters (TODO)
     * chars 0-255: 4x8 charset
     * chars 256-511: 8x8 charset
 * 0C000 - 0C7FF: BG0 map data
 * 0C800 - 0CFFF: BG1 map data
 * 0D000 - 0D7FF: BG2 map data
 * 0D800 - 0DFFF: BG3 map data
 * 0E000 - 0FFFF: currently unused
 * 10000 - 17FFF: OBJ VRAM - we don't use OBJ, so this may be repurposed for other data; currently unused

## Audio

Emulating the PC speaker on GBA is easy - simply run a sound timer for the duration of a given frequency,
and use DMG channel 2 to emit 1/2 duty square waves.

Notably, since we're only generating square waves, we set the GBA's sampling rate to 262.144kHz. This should
hopefully improve audio fidelity a little on real hardware.

There are some frequency inaccuracies introduced by virtue of the GBA's limitations, but this isn't really
a big deal
