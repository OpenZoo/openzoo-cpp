#include "user_interface.h"
#include "txtwind.h"
#include "gamevars.h"

using namespace ZZT;

UserInterface::UserInterface(Driver *driver) {
    this->driver = driver;
}

void UserInterface::SidebarClearLine(int y) {
    driver->draw_string(60, y, 0x11, "                    ");
}

void UserInterface::SidebarClear(void) {
    for (int i = 3; i <= 24; i++) {
        SidebarClearLine(i);
    }
}

bool UserInterface::WaitYesNo(bool defaultReturn) {
    bool returnValue = defaultReturn;
    while (true) {
        driver->idle(IMUntilFrame);
        driver->update_input();

        if (driver->keyPressed == KeyEscape || UpCase(driver->keyPressed) == 'N' || driver->joy_button_pressed(JoyButtonB, false)) {
            returnValue = false;
            break;
        } else if (UpCase(driver->keyPressed) == 'Y' || driver->joy_button_pressed(JoyButtonA, false)) {
            returnValue = true;
            break;
        }
    }
    return returnValue;
}

bool UserInterface::SidebarPromptYesNo(const char *message, bool defaultReturn) {
    SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
    driver->draw_string(63, 5, 0x1F, message);
    driver->draw_char(63 + strlen(message), 5, 0x9E, '_');

    bool returnValue = WaitYesNo(defaultReturn);

    SidebarClearLine(5);
    return returnValue;
}

void UserInterface::PromptString(int16_t x, int16_t y, uint8_t arrowColor, uint8_t color, int16_t width, InputPromptMode mode, char *buffer, int buflen) {
    sstring<255> oldBuffer;
    StrCopy(oldBuffer, buffer);
    bool firstKeyPress = true;

    driver->set_text_input(true, mode);

    do {
        for (int i = 0; i < (width - 1); i++) {
            driver->draw_char(x + i, y, color, ' ');
            if (arrowColor != 0) {
                driver->draw_char(x + i, y - 1, arrowColor, ' ');
            }
        }
        if (arrowColor != 0) {
            driver->draw_char(x + width, y - 1, arrowColor, ' ');
            driver->draw_char(x + strlen(buffer), y - 1, arrowColor | 0x0F, 31);
        }
        driver->draw_string(x, y, color, buffer);

        driver->read_wait_key();

        if (strlen(buffer) < width && driver->keyPressed >= 32 && driver->keyPressed < 128) {
            if (firstKeyPress) {
                StrClear(buffer);
            }
            char chAppend = 0;
            switch (mode) {
                case InputPMNumbers:
                    if (driver->keyPressed >= '0' && driver->keyPressed <= '9') {
                        chAppend = driver->keyPressed;
                    }
                    break;
                case InputPMAlphanumeric:
                    if (
                        (driver->keyPressed >= 'A' && driver->keyPressed <= 'Z')
                        || (driver->keyPressed >= 'a' && driver->keyPressed <= 'z')
                        || (driver->keyPressed >= '0' && driver->keyPressed <= '9')
                        || (driver->keyPressed == '-')
                    ) {
                        chAppend = UpCase(driver->keyPressed);
                    }
                    break;
                default:
                    chAppend = driver->keyPressed;
                    break;
            }
            int pos = strlen(buffer);
            if (chAppend != 0 && pos < (buflen - 1)) {
                buffer[pos++] = chAppend;
                buffer[pos] = 0;
            }
        } else if (driver->keyPressed == KeyLeft || driver->keyPressed == KeyBackspace) {
            int pos = strlen(buffer);
            if (pos > 0) {
                buffer[pos - 1] = 0;
            }
        }

        firstKeyPress = false;
    } while (driver->keyPressed != KeyEnter && driver->keyPressed != KeyEscape);

    driver->set_text_input(false, mode);

    if (driver->keyPressed == KeyEscape) {
        strcpy(buffer, oldBuffer);
    }
}

void UserInterface::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, InputPromptMode mode) {
    bool hasPrompt = (prompt != nullptr);
    bool hasExtension = (extension != nullptr);

    if (hasPrompt) SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
    if (hasPrompt) driver->draw_string(75 - strlen(prompt), 3, 0x1F, prompt);
    driver->draw_string(63, 5, 0x0F, "        ");
    if (hasExtension) driver->draw_string(71, 5, 0x0F, extension);
    
    PromptString(63, 5, 0x1E, 0x0F, hasExtension ? 8 : 11, mode, filename, filenameLen);

    if (hasPrompt) SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
}

void UserInterface::PopupPromptString(const char *question, char *buffer, size_t buffer_len) {
    int x = 3;
    int y = 18;
    int width = 50;
    uint8_t color = 0x4F;

    VideoCopy copy = VideoCopy(width, 6);
    driver->copy_chars(copy, x, y, width, 6, 0, 0);

    TextWindowDrawPattern(driver, x, y, width, color, WinPatTop);
    TextWindowDrawPattern(driver, x, y + 1, width, color, WinPatInner);
    TextWindowDrawPattern(driver, x, y + 2, width, color, WinPatSeparator);
    TextWindowDrawPattern(driver, x, y + 3, width, color, WinPatInner);
    TextWindowDrawPattern(driver, x, y + 4, width, color, WinPatInner);
    TextWindowDrawPattern(driver, x, y + 5, width, color, WinPatBottom);
    driver->draw_string(x + 1 + ((width - strlen(question)) / 2), y + 1, color, question);

    StrClear(buffer);
    PromptString(x + 7, y + 4, (color & 0xF0) | 0x0F, (color & 0xF0) | 0x0E, width - 16, InputPMAnyText, buffer, buffer_len);

    driver->paste_chars(copy, 0, 0, width, 6, x, y);
}

// TODO: Remove duplicate with game.cpp
static const char * menu_str_sound(Game *game) {
    return game->driver->sound_is_enabled() ? "Be quiet" : "Be noisy";
}

void UserInterface::SidebarGameDraw(Game &game, uint32_t flags) {
    if (flags == SIDEBAR_REDRAW) {
        SidebarClear();
        SidebarClearLine(0);
        SidebarClearLine(1);
        SidebarClearLine(2);
        driver->draw_string(61, 0, 0x1F, "    - - - - -      ");
        driver->draw_string(62, 1, 0x70, "      ZZT*     ");
        driver->draw_string(61, 2, 0x1F, "    - - - - -      ");
        if (game.gameStateElement == EPlayer) {
            driver->draw_string(64, 7, 0x1E,  " Health:");
            driver->draw_string(64, 8, 0x1E,  "   Ammo:");
            driver->draw_string(64, 9, 0x1E,  "Torches:");
            driver->draw_string(64, 10, 0x1E, "   Gems:");
            driver->draw_string(64, 11, 0x1E, "  Score:");
            driver->draw_string(64, 12, 0x1E, "   Keys:");
            driver->draw_char(62, 7, 0x1F, game.elementDef(EPlayer).character);
            driver->draw_char(62, 8, 0x1B, game.elementDef(EAmmo).character);
            driver->draw_char(62, 9, 0x16, game.elementDef(ETorch).character);
            driver->draw_char(62, 10, 0x1B, game.elementDef(EGem).character);
            driver->draw_char(62, 12, 0x1F, game.elementDef(EKey).character);
            driver->draw_string(62, 14, 0x70, " T ");
            driver->draw_string(65, 14, 0x1F, " Torch");
            driver->draw_string(62, 15, 0x30, " B ");
            driver->draw_string(62, 16, 0x70, " H ");
            driver->draw_string(65, 16, 0x1F, " Help");
            driver->draw_string(67, 18, 0x30, " \x18\x19\x1A\x1B ");
            driver->draw_string(72, 18, 0x1F, " Move");
            driver->draw_string(61, 19, 0x70, " Shift \x18\x19\x1A\x1B ");
            driver->draw_string(72, 19, 0x1F, " Shoot");
            driver->draw_string(62, 21, 0x70, " S ");
            driver->draw_string(65, 21, 0x1F, " Save game");
            driver->draw_string(62, 22, 0x30, " P ");
            driver->draw_string(65, 22, 0x1F, " Pause");
            driver->draw_string(62, 23, 0x70, " Q ");
            driver->draw_string(65, 23, 0x1F, " Quit");
        } else if (game.gameStateElement == EMonitor) {
            game.SidebarPromptSlider(false, 66, 21, "Game speed:;FS", game.tickSpeed);
            driver->draw_string(62, 21, 0x70, " S ");
            driver->draw_string(62, 7, 0x30, " W ");
            driver->draw_string(65, 7, 0x1E, " World:");
            driver->draw_string(62, 11, 0x70, " P ");
            driver->draw_string(65, 11, 0x1F, " Play");
            driver->draw_string(62, 12, 0x30, " R ");
            driver->draw_string(65, 12, 0x1E, " Restore game");
            driver->draw_string(62, 13, 0x70, " Q ");
            driver->draw_string(65, 13, 0x1E, " Quit");
            driver->draw_string(62, 16, 0x30, " A ");
            driver->draw_string(65, 16, 0x1F, " About ZZT!");
            driver->draw_string(62, 17, 0x70, " H ");
            driver->draw_string(65, 17, 0x1E, " High Scores");

            if (game.editorEnabled) {
                driver->draw_string(62, 18, 0x30, " E ");
                driver->draw_string(65, 18, 0x1E, " Board Editor");
            }
        }
    }

    if (flags & SIDEBAR_FLAG_SET_WORLD_NAME) {
        if (game.gameStateElement == EMonitor) {
            SidebarClearLine(8);

            // truncate
            sstring<8> name;
            StrCopy(name, StrLength(game.world.info.name) != 0 ? game.world.info.name : "Untitled");

            driver->draw_string(69, 8, 0x1F, name);
        }
    }

    if (flags & SIDEBAR_FLAG_UPDATE) {
        sstring<20> s;

        if (game.gameStateElement == EPlayer) {
            if (game.board.info.time_limit_seconds > 0) {
                driver->draw_string(64, 6, 0x1E, "   Time:");
                StrFormat(s, "%d ", game.board.info.time_limit_seconds - game.world.info.board_time_sec);
                driver->draw_string(72, 6, 0x1E, s);
            } else {
                SidebarClearLine(6);
            }

            if (game.world.info.health < 0) {
                game.world.info.health = 0;
            }

            StrFormat(s, "%d ", game.world.info.health);
            driver->draw_string(72, 7, 0x1E, s);
            StrFormat(s, "%d  ", game.world.info.ammo);
            driver->draw_string(72, 8, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.torches);
            driver->draw_string(72, 9, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.gems);
            driver->draw_string(72, 10, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.score);
            driver->draw_string(72, 11, 0x1E, s);

            if (game.world.info.torch_ticks == 0) {
                driver->draw_string(75, 9, 0x16, "    ");
            } else {
                int iLimit = ((game.world.info.torch_ticks * 5) / game.engineDefinition.torchDuration);
                for (int i = 2; i <= 5; i++) {
                    driver->draw_char(73 + i, 9, 0x16, (i <= iLimit) ? 177 : 176);
                }
            }

            for (int i = 0; i < 7; i++) {
                if (game.world.info.keys[i]) {
                    driver->draw_char(72 + i, 12, 0x19 + i, game.elementDef(EKey).character);
                } else {
                    driver->draw_char(72 + i, 12, 0x1F, ' ');
                }
            }

            driver->draw_string(66, 15, 0x1F, menu_str_sound(&game));

            if (game.debugEnabled) {
                // TODO: Add something interesting.
            }
        }
    }
}

void UserInterface::GameHideMessage(Game &game) {
	for (int iy = 0; iy < game.engineDefinition.messageLines; iy++) {
		for (int ix = 0; ix < game.viewport.width; ix++) {
			game.BoardDrawTile(game.viewport.cx_offset + ix + 1, game.viewport.cy_offset + game.viewport.height - iy);
		}
	}
}

void UserInterface::GameShowMessage(Game &game, uint8_t color) {
	sstring<80> message;
	
	if (color == 0) {
		color = messageColor;
	} else {
		messageColor = color;
	}

	int y = game.viewport.y + game.viewport.height - 1;
	for (int i = game.engineDefinition.messageLines - 1; i >= 0; i--) {
		if (game.board.info.message[i][0] == 0) continue;

		int msg_len = strlen(game.board.info.message[i]);
		int width = game.viewport.width;
		int msg_x = (msg_len - width) / 2;
		if (msg_x < 0) {
			msg_x = 0;
			width = strlen(game.board.info.message[i]);
		}

		if (width == game.viewport.width) {
			strncpy(message, game.board.info.message[i] + msg_x, width);
			message[width] = 0;
		} else if (width == game.viewport.width - 1) {
			message[0] = ' ';
			strncpy(message + 1, game.board.info.message[i] + msg_x, width);
			message[width + 1] = 0;
		} else {
			message[0] = ' ';
			strncpy(message + 1, game.board.info.message[i] + msg_x, width);
			message[width + 1] = ' ';
			message[width + 2] = 0;
		}

		game.driver->draw_string(game.viewport.x + (game.viewport.width - strlen(message)) / 2, y--, color, message);
	}
}

void UserInterface::SidebarShowMessage(uint8_t color, const char *message, bool temporary) {
    SidebarClearLine(5);

    if (message != nullptr) {
        // truncate
        sstring<17> name;
        StrCopy(name, message);

        driver->draw_string(62, 5, 0x10 | (color & 0x0F), message);
    }
}

TextWindow *UserInterface::CreateTextWindow(FilesystemDriver *fsDriver) {
	return new TextWindow(driver, fsDriver, 5, 3, 50, 18);
}

void UserInterface::ConfigureViewport(int16_t &x, int16_t &y, int16_t &width, int16_t &height) {
	x = 0;
	y = 0;
	width = 60;
	height = 25;
}

void UserInterface::DisplayFile(FilesystemDriver *filesystem, const char *filename, const char *title) {
    TextWindow *window = CreateTextWindow(filesystem);
    StrCopy(window->title, title);
    window->OpenFile(filename, false);
    if (window->line_count > 0) {
	    window->selectable = false;
        window->DrawOpen();
        window->Select(false, true);
        window->DrawClose();
    }
	delete window;
}

int UserInterface::HandleMenu(Game &game, const MenuEntry *entries, bool simulate) {
    if (driver->joy_button_pressed(JoyButtonStart, simulate) || driver->keyPressed == KeyF10) {
        if (simulate) {
            return 1;
        }

        sstring<10> numStr;
        const MenuEntry *entry = entries;
        const char *name;
        TextWindow *window = CreateTextWindow(nullptr);
        StrCopy(window->title, "Menu");
        int i = 0;
        while (entry->id >= 0) {
            name = entry->get_name(&game);
            if (name != NULL) {
                StrFromInt(numStr, i);
                window->Append(DynString("!") + numStr + ";" + name);
            }
            entry++; i++;
        }
        window->DrawOpen();
        window->Select(true, false);
        window->DrawClose();
        if (!window->rejected && !StrEmpty(window->hyperlink)) {
            int result = atoi(window->hyperlink);
            return entries[result].id;
        } else {
            return -1;
        }
    } else {
        const MenuEntry *entry = entries;
        while (entry->id >= 0) {
            if (entry->matches_key(UpCase(driver->keyPressed))) {
                return entry->id;
            }
            if (entry->joy_button != 0 && driver->joy_button_pressed(entry->joy_button, simulate)) {
                return entry->id;
            }
            entry++;
        }
    }
    return -1;
}