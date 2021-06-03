#include "user_interface_super_zzt.h"
#include "txtwind.h"
#include "gamevars.h"

using namespace ZZT;

UserInterfaceSuperZZT::UserInterfaceSuperZZT(Driver *driver, int width, int height)
: UserInterface(driver) {
	this->width = width;
	this->height = height;
    message_backup = nullptr;
}

VideoCopy *UserInterfaceSuperZZT::BackupSidebar(void) {
    VideoCopy *copy = new VideoCopy(width, 1);
    driver->copy_chars(*copy, 0, height - 1, width, 1, 0, 0);
    return copy;
}

void UserInterfaceSuperZZT::RestoreSidebar(VideoCopy *copy) {
    driver->paste_chars(*copy, 0, 0, width, 1, 0, height - 1);
    delete copy;
}

inline void UserInterfaceSuperZZT::ClearSidebar() {
	if (height >= 25) return;

	for (int i = 12; i < width; i++) {
		driver->draw_char(i, height - 1, 0x0F, ' ');
	}
}

inline uint8_t UserInterfaceSuperZZT::GetSidebarColor() {
	return height >= 25 ? 0x10 : 0x00;
}

bool UserInterfaceSuperZZT::SidebarPromptYesNo(const char *message, bool defaultReturn) {
    VideoCopy *copy = BackupSidebar();
	uint8_t color = GetSidebarColor();

    driver->draw_string(13, height - 1, color | 0x0F, message);
    driver->draw_char(13 + strlen(message), height - 1, color | 0x0E, '_');

    bool returnValue = WaitYesNo(defaultReturn);

    RestoreSidebar(copy);
    return returnValue;
}

void UserInterfaceSuperZZT::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, InputPromptMode mode) {
    bool hasPrompt = (prompt != nullptr);
    bool hasExtension = (extension != nullptr);

    VideoCopy *copy = BackupSidebar();
	uint8_t color = GetSidebarColor();

	ClearSidebar();
    int x = 13;
    if (hasPrompt) {
        driver->draw_string(x, height - 1, color | 0x0F, prompt);
        x += strlen(prompt) + 1;
    }

    driver->draw_string(x, height - 1, 0x0F, "        ");
    if (hasExtension) {
        driver->draw_string(x + 8, height - 1, color | 0x0F, extension);
    }

    PromptString(x, height - 1, 0, 0x0F, hasExtension ? 8 : 11, mode, filename, filenameLen);

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
	bool short_sidebar = width <= 36;
	int short_offset = short_sidebar ? 0 : 1;
	int status_offset = 8;

    if (flags == SIDEBAR_REDRAW) {
		for (int iy = 0; iy < height; iy++) {
			for (int ix = 12; ix < width; ix++) {
				if (ix >= game.viewport.x && iy >= game.viewport.y
					&& ix < (game.viewport.x + game.viewport.width) && iy < (game.viewport.y + game.viewport.height)) {
					continue;
				}

				driver->draw_char(ix, iy, 0x10, 0);
			}
		}

		// draw border
		if (width >= 40 && height >= 22) {
			for (int ix = -1; ix <= game.viewport.width; ix++) {
				driver->draw_char(
					game.viewport.x + ix,
					game.viewport.y - 1,
					0x1F, 220
				);
				driver->draw_char(
					game.viewport.x + ix,
					game.viewport.y + game.viewport.height,
					ix >= 0 ? 0x7F : 0x1F, 223
				);
			}
			driver->draw_char(
				game.viewport.x + game.viewport.width + 1,
				game.viewport.y + game.viewport.height,
				0x07, 219
			);
			for (int iy = 0; iy < game.viewport.height; iy++) {
				driver->draw_char(
					game.viewport.x - 1,
					game.viewport.y + iy,
					0x0F, 219
				);
				driver->draw_char(
					game.viewport.x + game.viewport.width,
					game.viewport.y + iy,
					0x0F, 219
				);
				driver->draw_char(
					game.viewport.x + game.viewport.width + 1,
					game.viewport.y + iy,
					0x07, 219
				);
			}
		}

		// draw sidebar
		for (int iy = 0; iy < height; iy++) {
			driver->draw_char(0, iy, (iy >= 20 && iy == (height - 1)) ? 0x10 : 0x60, ' ');
		}

		driver->draw_string(0, 0, 0x1D, "\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC");
		driver->draw_string(0, 1, 0x6F, short_sidebar ? " Commands:  " : "  Commands  ");
		driver->draw_string(0, 2, 0x1D, "\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF");

		driver->draw_string(0, status_offset, 0x1D, "\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC");
		driver->draw_string(0, status_offset + 1, 0x6F, short_sidebar ? "  Status:   " : "   Status   ");;
		driver->draw_string(0, status_offset + 2, 0x1D, "\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xDF");

		if (game.gameStateElement == EPlayer) {
			driver->draw_string(short_offset, 3, 0x6F, "\x18\x19\x1A\x1B       ");
			driver->draw_string(short_offset, 4, 0x6E, "  Move     ");
			driver->draw_string(short_offset, 5, 0x6F, "A + \x18\x19\x1A\x1B   ");
			driver->draw_string(short_offset, 6, 0x6B, "  Shoot    ");
			driver->draw_string(short_offset, 7, 0x6F, "START ");
			driver->draw_string(short_offset + 6, 7, 0x6E, "Menu ");

			driver->draw_string(0, status_offset + 3, 0x6F, "Health      ");
			driver->draw_string(short_offset, status_offset + 4, 0x6F, "           ");
			driver->draw_string(short_offset, status_offset + 5, 0x6F, "Gems       ");
			driver->draw_char(short_offset + 5, status_offset + 5, 0x62, game.elementDef(EGem).character);
			driver->draw_string(short_offset, status_offset + 6, 0x6F, "Ammo       ");
			driver->draw_char(short_offset + 5, status_offset + 6, 0x6B, game.elementDef(EAmmo).character);
			driver->draw_string(short_offset, status_offset + 7, 0x6F, "Keys       ");
			driver->draw_string(short_offset, status_offset + 8, 0x6F, "           ");
			driver->draw_string(short_offset, status_offset + 9, 0x6F, "Score      ");
			driver->draw_string(short_offset, status_offset + 10, 0x6F, "           ");
			driver->draw_string(short_offset, status_offset + 11, 0x6F, "           ");
        } else if (game.gameStateElement == EMonitor) {
			for (int iy = 3; iy < status_offset; iy++) {
				driver->draw_string(1, iy, 0x60, "           ");
			}
			driver->draw_string(short_offset, 5, 0x6F, "START ");
			driver->draw_string(short_offset + 6, 5, 0x6E, "Menu ");
			for (int iy = status_offset + 3; iy < status_offset + 11; iy++) {
				driver->draw_string(1, iy, 0x60, "           ");
			}
		}

		for (int iy = status_offset + 11; iy < height; iy++) {
			driver->draw_string(1, iy, iy == (height - 1) ? 0x10 : 0x60, "           ");
		}

		if (short_sidebar) {
			for (int iy = 0; iy < height; iy++) {
				driver->draw_char(11, iy, 0x01, '\xDD');
			}
		}
    }

    if (flags & SIDEBAR_FLAG_UPDATE) {
		if (game.gameStateElement == EPlayer) {
			if (game.world.info.health < 0) {
				game.world.info.health = 0;
			}
			for (int i = 1; i <= 5; i++) {
				if ((i * 20) <= game.world.info.health) {
					driver->draw_char(i + 5 + short_offset, 11, 0x6E, 219);
				} else if (((i * 20) - 10) > game.world.info.health) {
					driver->draw_char(i + 5 + short_offset, 11, 0x6E, ' ');
				} else {
					driver->draw_char(i + 5 + short_offset, 11, 0x6E, 221);
				}
			}

			write_number(driver, 10 + short_offset, 13, 0x6E, game.world.info.gems, 5);
			write_number(driver, 10 + short_offset, 14, 0x6E, game.world.info.ammo, 5);
			write_number(driver, 10 + short_offset, 17, 0x6E, game.world.info.score, 5);

			driver->draw_string(short_offset, 18, 0x6F, "           ");
			for (int i = 0; i < game.engineDefinition.flagCount; i++) {
				if (game.world.info.flags[i][0] == 'Z') {
					driver->draw_string(short_offset, 18, 0x6F, game.world.info.flags[i] + 1);
				}
			}
			if (game.world.info.stones_of_power >= 0) {
				write_number(driver, 10 + short_offset, 18, 0x6E, game.world.info.stones_of_power, 5);
			}

			{
				for (int i = 0; i < 7; i++) {
					if (game.world.info.keys[i])
						driver->draw_char((i & 3) + 6 + short_offset, (i >> 2) + 15, 0x68 + i, game.elementDef(EKey).character);
					else
						driver->draw_char((i & 3) + 6 + short_offset, (i >> 2) + 15, 0x60, ' ');
				}
			}
		}
    }
}

void UserInterfaceSuperZZT::ConfigureViewport(int16_t &v_x, int16_t &v_y, int16_t &v_width, int16_t &v_height) {
	v_width = 24;
	v_height = 20;

	v_x = 12 + ((width - 12 - v_width) >> 1);
	v_y = (height - v_height) >> 1;
}

TextWindow *UserInterfaceSuperZZT::CreateTextWindow(FilesystemDriver *fsDriver) {
	int t_width = (width >= 40) ? 38 : width - 2;
	int t_height = (height >= 25) ? 22 : height - 3;
	return new TextWindow(driver, fsDriver, (width - t_width) / 2, (height - t_height) / 2, t_width, t_height);
}

void UserInterfaceSuperZZT::SidebarShowMessage(uint8_t color, const char *message, bool temporary) {
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
			uint8_t bgcolor = GetSidebarColor();

			ClearSidebar();
            for (int i = 0; i < width - 12; i++) {
                driver->draw_char(12 + i, height - 1, bgcolor | (color & 0x0F), (i >= 1 && i <= msglen) ? message[i - 1] : ' ');
            }
        }
    }
}