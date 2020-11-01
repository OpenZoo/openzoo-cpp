// Auto-generated file. Please do not edit directly.

const ElementDef defaultElementDefs[ElementCount] = {
	{ // 0 - Empty
		' ', // character
		0x70, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		true, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"Empty", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 1
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementBoardEdgeTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 2
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementMessageTimerTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 3 - Monitor
		' ', // character
		0x07, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		1, // cycle
		ElementMonitorTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"Monitor", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 4 - Player
		0x02, // character
		0x1F, // color
		true, // destructible
		true, // pushable
		true, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		1, // cycle
		ElementPlayerTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryItem, // editor category
		'Z', // editor shortcut
		"Player", // name
		"Items:", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 5 - Ammo
		0x84, // character
		0x03, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementAmmoTouch, // touch func
		EditorCategoryItem, // editor category
		'A', // editor shortcut
		"Ammo", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 6 - Torch
		0x9D, // character
		0x06, // color
		false, // destructible
		false, // pushable
		true, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementTorchTouch, // touch func
		EditorCategoryItem, // editor category
		'T', // editor shortcut
		"Torch", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 7 - Gem
		0x04, // character
		ColorChoiceOnBlack, // color
		true, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementGemTouch, // touch func
		EditorCategoryItem, // editor category
		'G', // editor shortcut
		"Gem", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 8 - Key
		0x0C, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementKeyTouch, // touch func
		EditorCategoryItem, // editor category
		'K', // editor shortcut
		"Key", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 9 - Door
		0x0A, // character
		ColorWhiteOnChoice, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDoorTouch, // touch func
		EditorCategoryItem, // editor category
		'D', // editor shortcut
		"Door", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 10 - Scroll
		0xE8, // character
		0x0F, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		1, // cycle
		ElementScrollTick, // tick func
		ElementScrollTouch, // touch func
		EditorCategoryItem, // editor category
		'S', // editor shortcut
		"Scroll", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"Edit text of scroll", // param text name
		0, // score value
	},
	{ // 11 - Passage
		0xF0, // character
		ColorWhiteOnChoice, // color
		false, // destructible
		false, // pushable
		true, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		0, // cycle
		ElementDefaultTick, // tick func
		ElementPassageTouch, // touch func
		EditorCategoryItem, // editor category
		'P', // editor shortcut
		"Passage", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"Room thru passage?", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 12 - Duplicator
		0xFA, // character
		0x0F, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementDuplicatorDraw, // draw func
		2, // cycle
		ElementDuplicatorTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryItem, // editor category
		'U', // editor shortcut
		"Duplicator", // name
		"", // category name
		"", // param 1 name
		"Duplication rate?;SF", // param 2 name
		"", // param bullet type name
		"", // param board name
		"Source direction?", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 13 - Bomb
		0x0B, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementBombDraw, // draw func
		6, // cycle
		ElementBombTick, // tick func
		ElementBombTouch, // touch func
		EditorCategoryItem, // editor category
		'B', // editor shortcut
		"Bomb", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 14 - Energizer
		0x7F, // character
		0x05, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementEnergizerTouch, // touch func
		EditorCategoryItem, // editor category
		'E', // editor shortcut
		"Energizer", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 15 - Star
		'S', // character
		0x0F, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementStarDraw, // draw func
		1, // cycle
		ElementStarTick, // tick func
		ElementDamagingTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"Star", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 16 - Clockwise
		'/', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementConveyorCWDraw, // draw func
		3, // cycle
		ElementConveyorCWTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryItem, // editor category
		'1', // editor shortcut
		"Clockwise", // name
		"Conveyors:", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 17 - Counter
		0x5C, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementConveyorCCWDraw, // draw func
		2, // cycle
		ElementConveyorCCWTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryItem, // editor category
		'2', // editor shortcut
		"Counter", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 18 - Bullet
		0xF8, // character
		0x0F, // color
		true, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		1, // cycle
		ElementBulletTick, // tick func
		ElementDamagingTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"Bullet", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 19 - Water
		0xB0, // character
		0xF9, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		true, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementWaterTouch, // touch func
		EditorCategoryTerrain, // editor category
		'W', // editor shortcut
		"Water", // name
		"Terrains:", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 20 - Forest
		0xB0, // character
		0x20, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementForestTouch, // touch func
		EditorCategoryTerrain, // editor category
		'F', // editor shortcut
		"Forest", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 21 - Solid
		0xDB, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryTerrain, // editor category
		'S', // editor shortcut
		"Solid", // name
		"Walls:", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 22 - Normal
		0xB2, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryTerrain, // editor category
		'N', // editor shortcut
		"Normal", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 23 - Breakable
		0xB1, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryTerrain, // editor category
		'B', // editor shortcut
		"Breakable", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 24 - Boulder
		0xFE, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementPushableTouch, // touch func
		EditorCategoryTerrain, // editor category
		'O', // editor shortcut
		"Boulder", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 25 - Slider (NS)
		0x12, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementPushableTouch, // touch func
		EditorCategoryTerrain, // editor category
		'1', // editor shortcut
		"Slider (NS)", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 26 - Slider (EW)
		0x1D, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementPushableTouch, // touch func
		EditorCategoryTerrain, // editor category
		'2', // editor shortcut
		"Slider (EW)", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 27 - Fake
		0xB2, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		true, // placeable on top
		true, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementFakeTouch, // touch func
		EditorCategoryTerrain, // editor category
		'A', // editor shortcut
		"Fake", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 28 - Invisible
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementInvisibleTouch, // touch func
		EditorCategoryTerrain, // editor category
		'I', // editor shortcut
		"Invisible", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 29 - Blink wall
		0xCE, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementBlinkWallDraw, // draw func
		1, // cycle
		ElementBlinkWallTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryTerrain, // editor category
		'L', // editor shortcut
		"Blink wall", // name
		"", // category name
		"Starting time", // param 1 name
		"Period", // param 2 name
		"", // param bullet type name
		"", // param board name
		"Wall direction", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 30 - Transporter
		0xC5, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementTransporterDraw, // draw func
		2, // cycle
		ElementTransporterTick, // tick func
		ElementTransporterTouch, // touch func
		EditorCategoryTerrain, // editor category
		'T', // editor shortcut
		"Transporter", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"Direction?", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 31 - Line
		0xCE, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementLineDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"Line", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 32 - Ricochet
		'*', // character
		0x0A, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryTerrain, // editor category
		'R', // editor shortcut
		"Ricochet", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 33
		0xCD, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 34 - Bear
		0x99, // character
		0x06, // color
		true, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		3, // cycle
		ElementBearTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'B', // editor shortcut
		"Bear", // name
		"Creatures:", // category name
		"Sensitivity?", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		1, // score value
	},
	{ // 35 - Ruffian
		0x05, // character
		0x0D, // color
		true, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		1, // cycle
		ElementRuffianTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'R', // editor shortcut
		"Ruffian", // name
		"", // category name
		"Intelligence?", // param 1 name
		"Resting time?", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		2, // score value
	},
	{ // 36 - Object
		0x02, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementObjectDraw, // draw func
		3, // cycle
		ElementObjectTick, // tick func
		ElementObjectTouch, // touch func
		EditorCategoryCreature, // editor category
		'O', // editor shortcut
		"Object", // name
		"", // category name
		"Character?", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"Edit Program", // param text name
		0, // score value
	},
	{ // 37 - Slime
		'*', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		3, // cycle
		ElementSlimeTick, // tick func
		ElementSlimeTouch, // touch func
		EditorCategoryCreature, // editor category
		'V', // editor shortcut
		"Slime", // name
		"", // category name
		"", // param 1 name
		"Movement speed?;FS", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 38 - Shark
		'^', // character
		0x07, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		3, // cycle
		ElementSharkTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryCreature, // editor category
		'Y', // editor shortcut
		"Shark", // name
		"", // category name
		"Intelligence?", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 39 - Spinning gun
		0x18, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementSpinningGunDraw, // draw func
		2, // cycle
		ElementSpinningGunTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryCreature, // editor category
		'G', // editor shortcut
		"Spinning gun", // name
		"", // category name
		"Intelligence?", // param 1 name
		"Firing rate?", // param 2 name
		"Firing type?", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 40 - Pusher
		0x10, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		true, // has draw func
		ElementPusherDraw, // draw func
		4, // cycle
		ElementPusherTick, // tick func
		ElementDefaultTouch, // touch func
		EditorCategoryCreature, // editor category
		'P', // editor shortcut
		"Pusher", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"Push direction?", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 41 - Lion
		0xEA, // character
		0x0C, // color
		true, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		2, // cycle
		ElementLionTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'L', // editor shortcut
		"Lion", // name
		"Beasts:", // category name
		"Intelligence?", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		1, // score value
	},
	{ // 42 - Tiger
		0xE3, // character
		0x0B, // color
		true, // destructible
		true, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		2, // cycle
		ElementTigerTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'T', // editor shortcut
		"Tiger", // name
		"", // category name
		"Intelligence?", // param 1 name
		"Firing rate?", // param 2 name
		"Firing type?", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		2, // score value
	},
	{ // 43
		0xBA, // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 44 - Head
		0xE9, // character
		ColorChoiceOnBlack, // color
		true, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		2, // cycle
		ElementCentipedeHeadTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'H', // editor shortcut
		"Head", // name
		"Centipedes", // category name
		"Intelligence?", // param 1 name
		"Deviance?", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		1, // score value
	},
	{ // 45 - Segment
		'O', // character
		ColorChoiceOnBlack, // color
		true, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		2, // cycle
		ElementCentipedeSegmentTick, // tick func
		ElementDamagingTouch, // touch func
		EditorCategoryCreature, // editor category
		'S', // editor shortcut
		"Segment", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		3, // score value
	},
	{ // 46
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 47
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 48
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 49
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 50
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 51
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 52
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	},
	{ // 53
		' ', // character
		ColorChoiceOnBlack, // color
		false, // destructible
		false, // pushable
		false, // visible in dark
		false, // placeable on top
		false, // walkable
		false, // has draw func
		ElementDefaultDraw, // draw func
		-1, // cycle
		ElementDefaultTick, // tick func
		ElementDefaultTouch, // touch func
		0, // editor category
		'\0', // editor shortcut
		"", // name
		"", // category name
		"", // param 1 name
		"", // param 2 name
		"", // param bullet type name
		"", // param board name
		"", // param dir name
		"", // param text name
		0, // score value
	}
};
