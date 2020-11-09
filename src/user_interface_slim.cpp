#include "user_interface_slim.h"
#include "txtwind.h"
#include "gamevars.h"

using namespace ZZT;
using namespace ZZT::Utils;

UserInterfaceSlim::UserInterfaceSlim(VideoDriver *video, InputDriver *input, SoundDriver *sound)
: UserInterface(video, input, sound) {

}

VideoCopy *UserInterfaceSlim::BackupSidebar(void) {
    VideoCopy *copy = new VideoCopy(60, 1);
    video->copy_chars(*copy, 0, 25, 60, 1, 0, 0);
    for (int i = 0; i < 60; i++) {
        video->draw_char(i, 25, 0x10, ' ');
    }
    return copy;
}

void UserInterfaceSlim::RestoreSidebar(VideoCopy *copy) {
    video->paste_chars(*copy, 0, 0, 60, 1, 0, 25);
    delete copy;
}

bool UserInterfaceSlim::SidebarPromptYesNo(const char *message, bool defaultReturn) {
    VideoCopy *copy = BackupSidebar();

    video->draw_string(1, 25, 0x1F, message);
    video->draw_char(1 + strlen(message), 25, 0x9E, '_');

    bool returnValue = WaitYesNo(defaultReturn);

    RestoreSidebar(copy);
    return returnValue;
}

void UserInterfaceSlim::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, PromptMode mode) {
    bool hasPrompt = (prompt != nullptr);
    bool hasExtension = (extension != nullptr);

    VideoCopy *copy = BackupSidebar();
    int x = 1;
    if (hasPrompt) {
        video->draw_string(1, 25, 0x1F, prompt);
        x += strlen(prompt) + 1;
    }

    video->draw_string(x, 25, 0x0F, "        ");
    if (hasExtension) {
        video->draw_string(x + 8, 25, 0x0F, extension);
    }

    PromptString(x, 25, 0, 0x0F, hasExtension ? 8 : 11, mode, filename, filenameLen);

    RestoreSidebar(copy);
}

static void write_number(VideoDriver *video, int16_t x, int16_t y, uint8_t col, int val, int len) {
	char s[8];
	int16_t pos = sizeof(s);
	int16_t i = val < 0 ? -val : val;

	while (i >= 10) {
		s[--pos] = '0' + (i % 10);
		i /= 10;
	}
	s[--pos] = '0' + (i % 10);
	if (val < 0) {
		s[--pos] = '-';
	}

	for (i = pos; i < sizeof(s); i++) {
		video->draw_char(x + i - pos, y, col, s[i]);
	}
	for (i = sizeof(s) - pos; i < len; i++) {
		video->draw_char(x + i, y, col, ' ');
	}
}

static void write_number_torch_bg(int16_t torch_ticks, VideoDriver *video, int16_t x, int16_t y, uint8_t col, int val, int len) {
	char s[8];
	int16_t pos = sizeof(s);
	int16_t i = val < 0 ? -val : val;
	uint8_t torch_col;
	int16_t torch_pos;

	while (i >= 10) {
		s[--pos] = '0' + (i % 10);
		i /= 10;
	}
	s[--pos] = '0' + (i % 10);
	if (val < 0) {
		s[--pos] = '-';
	}

	torch_col = (col & 0x0F) | 0x60;
	torch_pos = torch_ticks > 0
		? (torch_ticks + 39) / 40
		: 0;
	if (torch_pos > 5) torch_pos = 5;

	for (i = pos; i < sizeof(s); i++) {
		int16_t nx = x + i - pos;
		video->draw_char(nx, y, nx < torch_pos ? torch_col : col, s[i]);
	}

	for (i = sizeof(s) - pos; i < torch_pos; i++) {
        video->draw_char(x + i, y, torch_col, ' ');
	}
	for (; i < len; i++) {
		video->draw_char(x + i, y, col, ' ');
	}
}

void UserInterfaceSlim::SidebarGameDraw(Game &game, uint32_t flags) {
    int i;
	int x = 1;
    bool redraw = flags == SIDEBAR_REDRAW;

	if (game.gameStateElement == EMonitor) {
		if (redraw) {
			for (i = 0; i < 60; i++) {
				video->draw_char(i, 25, 0x0F, ' ');
			}
		}
		return;
	}

	// left-aligned

    if (flags & SIDEBAR_FLAG_UPDATE) {
        {
            video->draw_char(x, 25, 0x1C, '\x03');
            video->draw_char(x + 1, 25, 0x1C, ' ');
            write_number(video, x + 2, 25, 0x1F, game.world.info.health, 6);
        }
        x += 8;

        {
            video->draw_char(x, 25, 0x1B, '\x84');
            video->draw_char(x + 1, 25, 0x1B, ' ');
            write_number(video, x + 2, 25, 0x1F, game.world.info.ammo, 6);
        }
        x += 8;

        {
            video->draw_char(x, 25, 0x1E, '\x9D');
            video->draw_char(x + 1, 25, 0x1E, ' ');
            write_number_torch_bg(game.world.info.torch_ticks, video, x + 2, 25, 0x1F, game.world.info.torches, 6);
        }
        x += 8;

        {
            video->draw_char(x, 25, 0x19, '\x04');
            video->draw_char(x + 1, 25, 0x19, ' ');
            write_number(video, x + 2, 25, 0x1F, game.world.info.gems, 6);
        }
        x += 8;

        {
            video->draw_char(x, 25, 0x17, '\x9E');
            video->draw_char(x + 1, 25, 0x17, ' ');
            write_number(video, x + 2, 25, 0x1F, game.world.info.score, 6);
        }
        x += 8;

        {
            for (i = 0; i < 7; i++) {
                if (game.world.info.keys[i])
                    video->draw_char(x + i, 25, 0x19 + i, '\x0C');
                else
                    video->draw_char(x + i, 25, 0x1F, ' ');
            }
            video->draw_char(x + 7, 25, 0x1F, ' ');
        }
        x += 8;

        {
            if (game.board.info.time_limit_seconds > 0) {
                video->draw_char(x, 25, 0x1E, 'T');
                video->draw_char(x + 1, 25, 0x1E, ' ');
                write_number(video, x + 2, 25, 0x1F, game.board.info.time_limit_seconds - game.world.info.board_time_sec, 6);
            } else {
                for (i = 0; i < 8; i++) {
                    video->draw_char(x + i, 25, 0x1E, ' ');
                }
            }
        }
        x += 8;

        if (redraw) {
            video->draw_char(0, 25, 0x1F, ' ');
            for (i = x; i < 60; i++) {
                video->draw_char(i, 25, 0x1F, ' ');
            }
        }
    }
}

void UserInterfaceSlim::SidebarShowMessage(uint8_t color, const char *message) {
    // TODO
}