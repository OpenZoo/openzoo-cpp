#!/bin/bash
# Sizes based on https://wiki.freepascal.org/Windows_Icon
# source: 16x16, 32x32
convert icon16.png \( icon.png \) \( icon48.png \) -filter box -bordercolor none \
	`# 16x16` \
	\( -clone 0 -border 2 \) `# 20x20` \
	\( -clone 0 -border 4 \) `# 24x24` \
	`# 32x32` \
	\( -clone 1 -border 4 \) `# 40x40` \
	`# 48x48` \
	\( -clone 2 -border 6 \) `# 60x60` \
	\( -clone 1 -resize 64x64 \) `# 64x64` \
	\( -clone 1 -resize 64x64 -border 4 \) `# 72x72` \
	\( -clone 1 -resize 96x96 \) `# 96x96` \
	\( -clone 1 -resize 128x128 \) `# 128x128` \
	\( -clone 1 -resize 256x256 \) `# 256x256` \
	-colors 16 icon.ico
