#include <cstdio>
#include "gamevars.h"
#include "txtwind.h"
#include "driver_sdl.h"

using namespace ZZT;
using namespace ZZT::Utils;

void gameConfigure(Game &game) {
	game.editorEnabled = true;
}

int main(int argc, char** argv) {
	SDL2Driver driver = SDL2Driver();
	Game game = Game();
	
	game.input = &driver;
	game.sound = &driver;
	game.video = &driver;

	driver.install();

	StrCopy(game.startupWorldFileName, "TOWN");
	// ResourceDataFileName
	// ResetConfig
	game.gameTitleExitRequested = false;
	gameConfigure(game);
	// ParseArguments;

	if (!game.gameTitleExitRequested) {
		TextWindowInit(5, 3, 50, 18);

		game.video->set_cursor(false);
		game.video->clrscr();

		game.tickSpeed = 4;
		game.debugEnabled = false;
		StrCopy(game.savedGameFileName, "SAVED");
		StrCopy(game.savedBoardFileName, "TEMP");
		game.GenerateTransitionTable();
		game.WorldCreate();

		game.GameTitleLoop();
	}

	game.video->clrscr();
	game.video->set_cursor(true);

	driver.uninstall();

	return 0;
}
