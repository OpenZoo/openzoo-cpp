#include "user_interface_super_zzt.h"
#include "txtwind.h"
#include "gamevars.h"

using namespace ZZT;

UserInterfaceSuperZZT::UserInterfaceSuperZZT(Driver *driver)
: UserInterface(driver) {
    message_backup = nullptr;
}

VideoCopy *UserInterfaceSuperZZT::BackupSidebar(void) {
    VideoCopy *copy = new VideoCopy(24, 1);
    driver->copy_chars(*copy, 12, 19, 24, 1, 0, 0);
    return copy;
}

void UserInterfaceSuperZZT::RestoreSidebar(VideoCopy *copy) {
    driver->paste_chars(*copy, 0, 0, 24, 1, 12, 19);
    delete copy;
}

bool UserInterfaceSuperZZT::SidebarPromptYesNo(const char *message, bool defaultReturn) {
    VideoCopy *copy = BackupSidebar();

	driver->draw_string(12, 19, 0x0F, "                        ");
    driver->draw_string(13, 19, 0x0F, message);
    driver->draw_char(13 + strlen(message), 19, 0x0E, '_');

    bool returnValue = WaitYesNo(defaultReturn);

    RestoreSidebar(copy);
    return returnValue;
}

void UserInterfaceSuperZZT::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, InputPromptMode mode) {
    bool hasPrompt = (prompt != nullptr);
    bool hasExtension = (extension != nullptr);

    VideoCopy *copy = BackupSidebar();
	driver->draw_string(12, 19, 0x0F, "                        ");
    int x = 13;
    if (hasPrompt) {
        driver->draw_string(x, 19, 0x1F, prompt);
        x += strlen(prompt) + 1;
    }

    driver->draw_string(x, 19, 0x0F, "        ");
    if (hasExtension) {
        driver->draw_string(x + 8, 19, 0x0F, extension);
    }

    PromptString(x, 19, 0, 0x0F, hasExtension ? 8 : 11, mode, filename, filenameLen);

    RestoreSidebar(copy);
}

static void write_number(Driver *driver, int16_t x, int16_t y, uint8_t col, int val, int len) {
	char s[8];
	int16_t pos = 0;
	int16_t i = val < 0 ? -val : val;

	while (i >= 10) {
		s[pos++] = '0' + (i % 10);
		i /= 10;
	}
	s[pos++] = '0' + (i % 10);
	if (val < 0) {
		s[pos++] = '-';
	}

	for (i = len - 1; i >= pos; i--) {
		driver->draw_char(x - i, y, col, 0);
	}

	for (i = pos - 1; i >= 0; i--) {
		driver->draw_char(x - i, y, col, s[i]);
	}
}

void UserInterfaceSuperZZT::SidebarGameDraw(Game &game, uint32_t flags) {
    if (flags == SIDEBAR_REDRAW) {
		driver->draw_string(0, 0, 0x1D, "\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC");
		driver->draw_string(0, 1, 0x6F, " Commands:  ");
		driver->draw_string(0, 2, 0x1D, "\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF");

		driver->draw_string(0, 8, 0x1D, "\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC");
		driver->draw_string(0, 9, 0x6F, "  Status:  ");
		driver->draw_string(0, 10, 0x1D, "\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF");

		if (game.gameStateElement == EPlayer) {
			driver->draw_string(0, 3, 0x6F, " \x18\x19\x1A\x1B       ");
			driver->draw_string(0, 4, 0x6E, "   Move     ");
			driver->draw_string(0, 5, 0x6F, " A + \x18\x19\x1A\x1B   ");
			driver->draw_string(0, 6, 0x6B, "   Shoot    ");
			driver->draw_string(0, 7, 0x6F, " START ");
			driver->draw_string(7, 7, 0x6E, "Menu ");

			driver->draw_string(0, 11, 0x6F, "Health      ");
			driver->draw_string(0, 12, 0x6F, "            ");
			driver->draw_string(0, 13, 0x6F, "Gems        ");
			driver->draw_char(5, 13, 0x62, game.elementDef(EGem).character);
			driver->draw_string(0, 14, 0x6F, "Ammo        ");
			driver->draw_char(5, 14, 0x6B, game.elementDef(EAmmo).character);
			driver->draw_string(0, 15, 0x6F, "Keys        ");
			driver->draw_string(0, 16, 0x6F, "            ");
			driver->draw_string(0, 17, 0x6F, "Score       ");
			driver->draw_string(0, 18, 0x6F, "            ");
			driver->draw_string(0, 19, 0x6F, "            ");
        } else if (game.gameStateElement == EMonitor) {
			for (int iy = 3; iy < 8; iy++) {
				driver->draw_string(0, iy, 0x60, "           ");
			}
			driver->draw_string(0, 5, 0x6F, " START ");
			driver->draw_string(7, 5, 0x6E, "Menu ");
			for (int iy = 11; iy < 20; iy++) {
				driver->draw_string(0, iy, 0x60, "           ");
			}
		}

		for (int iy = 0; iy < 20; iy++) {
			driver->draw_char(11, iy, 0x01, '\xDD');
		}
    }

    if (flags & SIDEBAR_FLAG_UPDATE) {
		if (game.gameStateElement == EPlayer) {
			if (game.world.info.health < 0) {
				game.world.info.health = 0;
			}
			for (int i = 1; i <= 5; i++) {
				if ((i * 20) <= game.world.info.health) {
					driver->draw_char(i + 5, 11, 0x6E, 219);
				} else if (((i * 20) - 10) > game.world.info.health) {
					driver->draw_char(i + 5, 11, 0x6E, ' ');
				} else {
					driver->draw_char(i + 5, 11, 0x6E, 221);
				}
			}

			write_number(driver, 10, 13, 0x6E, game.world.info.gems, 5);
			write_number(driver, 10, 14, 0x6E, game.world.info.ammo, 5);
			write_number(driver, 10, 17, 0x6E, game.world.info.score, 5);

			driver->draw_string(0, 18, 0x6F, "           ");
			for (int i = 0; i < game.engineDefinition.flagCount; i++) {
				if (game.world.info.flags[i][0] == 'Z') {
					driver->draw_string(0, 18, 0x6F, game.world.info.flags[i] + 1);
				}
			}
			if (game.world.info.stones_of_power >= 0) {
				write_number(driver, 10, 18, 0x6E, game.world.info.stones_of_power, 5);
			}

			{
				for (int i = 0; i < 7; i++) {
					if (game.world.info.keys[i])
						driver->draw_char((i & 3) + 6, (i >> 2) + 15, 0x68 + i, game.elementDef(EKey).character);
					else
						driver->draw_char((i & 3) + 6, (i >> 2) + 15, 0x60, ' ');
				}
			}
		}
    }
}

void UserInterfaceSuperZZT::ConfigureViewport(int16_t &x, int16_t &y, int16_t &width, int16_t &height) {
	x = 12;
	y = 0;
	width = 24;
	height = 20;
}

void UserInterfaceSuperZZT::SidebarShowMessage(uint8_t color, const char *message, bool temporary) {
	// TODO
    if (temporary) {
        if (message == nullptr) {
            // clear
            if (message_backup != nullptr) {
                RestoreSidebar(message_backup);
                message_backup = nullptr;
            }
        } else {
            // set
            if (message_backup == nullptr) {
                message_backup = BackupSidebar();
            }
            size_t msglen = strlen(message);

            for (int i = 0; i < 24; i++) {
                driver->draw_char(12 + i, 19, (color & 0x0F), (i >= 1 && i <= msglen) ? message[i - 1] : ' ');
            }
        }
    }
}