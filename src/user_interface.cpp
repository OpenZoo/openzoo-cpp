#include "user_interface.h"
#include "txtwind.h"
#include "gamevars.h"

using namespace ZZT;
using namespace ZZT::Utils;

UserInterface::UserInterface(VideoDriver *video, InputDriver *input, SoundDriver *sound) {
    this->video = video;
    this->input = input;
    this->sound = sound;
}

void UserInterface::SidebarClearLine(int y) {
    video->draw_string(60, y, 0x11, "                    ");
}

void UserInterface::SidebarClear(void) {
    for (int i = 3; i <= 24; i++) {
        SidebarClearLine(i);
    }
}

bool UserInterface::WaitYesNo(bool defaultReturn) {
    bool returnValue = defaultReturn;
    while (true) {
        sound->idle(IMUntilFrame);
        input->update_input();

        if (input->keyPressed == KeyEscape || UpCase(input->keyPressed) == 'N' || input->joy_button_pressed(JoyButtonB, false)) {
            returnValue = false;
            break;
        } else if (UpCase(input->keyPressed) == 'Y' || input->joy_button_pressed(JoyButtonA, false)) {
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
    video->draw_string(63, 5, 0x1F, message);
    video->draw_char(63 + strlen(message), 5, 0x9E, '_');

    bool returnValue = WaitYesNo(defaultReturn);

    SidebarClearLine(5);
    return returnValue;
}

void UserInterface::PromptString(int16_t x, int16_t y, uint8_t arrowColor, uint8_t color, int16_t width, PromptMode mode, char *buffer, int buflen) {
    sstring<255> oldBuffer;
    StrCopy(oldBuffer, buffer);
    bool firstKeyPress = true;
    do {
        for (int i = 0; i < (width - 1); i++) {
            video->draw_char(x + i, y, color, ' ');
            if (arrowColor != 0) {
                video->draw_char(x + i, y - 1, arrowColor, ' ');
            }
        }
        if (arrowColor != 0) {
            video->draw_char(x + width, y - 1, arrowColor, ' ');
            video->draw_char(x + strlen(buffer), y - 1, arrowColor | 0x0F, 31);
        }
        video->draw_string(x, y, color, buffer);

        input->read_wait_key();

        if (strlen(buffer) < width && input->keyPressed >= 32 && input->keyPressed < 128) {
            if (firstKeyPress) {
                StrClear(buffer);
            }
            char chAppend = 0;
            switch (mode) {
                case PMNumeric:
                    if (input->keyPressed >= '0' && input->keyPressed <= '9') {
                        chAppend = input->keyPressed;
                    }
                    break;
                case PMAny:
                    chAppend = input->keyPressed;
                    break;
                case PMAlphanum:
                    if (
                        (input->keyPressed >= 'A' && input->keyPressed <= 'Z')
                        || (input->keyPressed >= 'a' && input->keyPressed <= 'z')
                        || (input->keyPressed >= '0' && input->keyPressed <= '9')
                        || (input->keyPressed == '-')
                    ) {
                        chAppend = UpCase(input->keyPressed);
                    }
                    break;
            }
            int pos = strlen(buffer);
            if (chAppend != 0 && pos < (buflen - 1)) {
                buffer[pos++] = chAppend;
                buffer[pos] = 0;
            }
        } else if (input->keyPressed == KeyLeft || input->keyPressed == KeyBackspace) {
            int pos = strlen(buffer);
            if (pos > 0) {
                buffer[pos - 1] = 0;
            }
        }

        firstKeyPress = false;
    } while (input->keyPressed != KeyEnter && input->keyPressed != KeyEscape);

    if (input->keyPressed == KeyEscape) {
        strcpy(buffer, oldBuffer);
    }
}

void UserInterface::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, PromptMode mode) {
    bool hasPrompt = (prompt != nullptr);
    bool hasExtension = (extension != nullptr);

    if (hasPrompt) SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
    if (hasPrompt) video->draw_string(75 - strlen(prompt), 3, 0x1F, prompt);
    video->draw_string(63, 5, 0x0F, "        ");
    if (hasExtension) video->draw_string(71, 5, 0x0F, extension);
    
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
    video->copy_chars(copy, x, y, width, 6, 0, 0);

    TextWindowDrawPattern(video, x, y, width, color, WinPatTop);
    TextWindowDrawPattern(video, x, y + 1, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 2, width, color, WinPatSeparator);
    TextWindowDrawPattern(video, x, y + 3, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 4, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 5, width, color, WinPatBottom);
    video->draw_string(x + 1 + ((width - strlen(question)) / 2), y + 1, color, question);

    StrClear(buffer);
    PromptString(x + 7, y + 4, (color & 0xF0) | 0x0F, (color & 0xF0) | 0x0E, width - 16, PMAny, buffer, buffer_len);

    video->paste_chars(copy, 0, 0, width, 6, x, y);
}

// TODO: Remove duplicate with game.cpp
static const char * menu_str_sound(Game *game) {
    return game->sound->sound_is_enabled() ? "Be quiet" : "Be noisy";
}

void UserInterface::SidebarGameDraw(Game &game, uint32_t flags) {
    if (flags == SIDEBAR_REDRAW) {
        SidebarClear();
        SidebarClearLine(0);
        SidebarClearLine(1);
        SidebarClearLine(2);
        video->draw_string(61, 0, 0x1F, "    - - - - -      ");
        video->draw_string(62, 1, 0x70, "      ZZT      ");
        video->draw_string(61, 2, 0x1F, "    - - - - -      ");
        if (game.gameStateElement == EPlayer) {
            video->draw_string(64, 7, 0x1E,  " Health:");
            video->draw_string(64, 8, 0x1E,  "   Ammo:");
            video->draw_string(64, 9, 0x1E,  "Torches:");
            video->draw_string(64, 10, 0x1E, "   Gems:");
            video->draw_string(64, 11, 0x1E, "  Score:");
            video->draw_string(64, 12, 0x1E, "   Keys:");
            video->draw_char(62, 7, 0x1F, game.elementDefs[EPlayer].character);
            video->draw_char(62, 8, 0x1B, game.elementDefs[EAmmo].character);
            video->draw_char(62, 9, 0x16, game.elementDefs[ETorch].character);
            video->draw_char(62, 10, 0x1B, game.elementDefs[EGem].character);
            video->draw_char(62, 12, 0x1F, game.elementDefs[EKey].character);
            video->draw_string(62, 14, 0x70, " T ");
            video->draw_string(65, 14, 0x1F, " Torch");
            video->draw_string(62, 15, 0x30, " B ");
            video->draw_string(62, 16, 0x70, " H ");
            video->draw_string(65, 16, 0x1F, " Help");
            video->draw_string(67, 18, 0x30, " \x18\x19\x1A\x1B ");
            video->draw_string(72, 18, 0x1F, " Move");
            video->draw_string(61, 19, 0x70, " Shift \x18\x19\x1A\x1B ");
            video->draw_string(72, 19, 0x1F, " Shoot");
            video->draw_string(62, 21, 0x70, " S ");
            video->draw_string(65, 21, 0x1F, " Save game");
            video->draw_string(62, 22, 0x30, " P ");
            video->draw_string(65, 22, 0x1F, " Pause");
            video->draw_string(62, 23, 0x70, " Q ");
            video->draw_string(65, 23, 0x1F, " Quit");
        } else if (game.gameStateElement == EMonitor) {
            game.SidebarPromptSlider(false, 66, 21, "Game speed:;FS", game.tickSpeed);
            video->draw_string(62, 21, 0x70, " S ");
            video->draw_string(62, 7, 0x30, " W ");
            video->draw_string(65, 7, 0x1E, " World:");
            video->draw_string(62, 11, 0x70, " P ");
            video->draw_string(65, 11, 0x1F, " Play");
            video->draw_string(62, 12, 0x30, " R ");
            video->draw_string(65, 12, 0x1E, " Restore game");
            video->draw_string(62, 13, 0x70, " Q ");
            video->draw_string(65, 13, 0x1E, " Quit");
            video->draw_string(62, 16, 0x30, " A ");
            video->draw_string(65, 16, 0x1F, " About ZZT!");
            video->draw_string(62, 17, 0x70, " H ");
            video->draw_string(65, 17, 0x1E, " High Scores");

            if (game.editorEnabled) {
                video->draw_string(62, 18, 0x30, " E ");
                video->draw_string(65, 18, 0x1E, " Board Editor");
            }
        }
    }

    if (flags & SIDEBAR_FLAG_SET_WORLD_NAME) {
        if (game.gameStateElement == EMonitor) {
            SidebarClearLine(8);

            // truncate
            sstring<8> name;
            StrCopy(name, StrLength(game.world.info.name) != 0 ? game.world.info.name : "Untitled");

            video->draw_string(69, 8, 0x1F, name);
        }
    }

    if (flags & SIDEBAR_FLAG_UPDATE) {
        sstring<20> s;

        if (game.gameStateElement == EPlayer) {
            if (game.board.info.time_limit_seconds > 0) {
                video->draw_string(64, 6, 0x1E, "   Time:");
                StrFormat(s, "%d ", game.board.info.time_limit_seconds - game.world.info.board_time_sec);
                video->draw_string(72, 6, 0x1E, s);
            } else {
                SidebarClearLine(6);
            }

            if (game.world.info.health < 0) {
                game.world.info.health = 0;
            }

            StrFormat(s, "%d ", game.world.info.health);
            video->draw_string(72, 7, 0x1E, s);
            StrFormat(s, "%d  ", game.world.info.ammo);
            video->draw_string(72, 8, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.torches);
            video->draw_string(72, 9, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.gems);
            video->draw_string(72, 10, 0x1E, s);
            StrFormat(s, "%d ", game.world.info.score);
            video->draw_string(72, 11, 0x1E, s);

            if (game.world.info.torch_ticks == 0) {
                video->draw_string(75, 9, 0x16, "    ");
            } else {
                int iLimit = ((game.world.info.torch_ticks * 5) / TORCH_DURATION);
                for (int i = 2; i <= 5; i++) {
                    video->draw_char(73 + i, 9, 0x16, (i <= iLimit) ? 177 : 176);
                }
            }

            for (int i = 0; i < 7; i++) {
                if (game.world.info.keys[i]) {
                    video->draw_char(72 + i, 12, 0x19 + i, game.elementDefs[EKey].character);
                } else {
                    video->draw_char(72 + i, 12, 0x1F, ' ');
                }
            }

            video->draw_string(66, 15, 0x1F, menu_str_sound(&game));

            if (game.debugEnabled) {
                // TODO: Add something interesting.
            }
        }
    }
}

void UserInterface::SidebarShowMessage(uint8_t color, const char *message) {
    SidebarClearLine(5);

    if (message != nullptr) {
        // truncate
        sstring<17> name;
        StrCopy(name, message);

        video->draw_string(62, 5, 0x10 | (color & 0x0F), message);
    }
}

int UserInterface::HandleMenu(Game &game, const MenuEntry *entries, bool simulate) {
    if (input->joy_button_pressed(JoyButtonStart, simulate)) {
        if (simulate) {
            return 1;
        }

        sstring<10> numStr;
        const MenuEntry *entry = entries;
        const char *name;
        TextWindow window = TextWindow(video, input, sound, nullptr);
        StrCopy(window.title, "Menu");
        int i = 0;
        while (entry->id >= 0) {
            name = entry->get_name(&game);
            if (name != NULL) {
                StrFromInt(numStr, i);
                window.Append(DynString("!") + numStr + ";" + name);
            }
            entry++; i++;
        }
        window.DrawOpen();
        window.Select(true, false);
        window.DrawClose();
        if (!window.rejected && !StrEmpty(window.hyperlink)) {
            int result = atoi(window.hyperlink);
            return entries[result].id;
        } else {
            return -1;
        }
    } else {
        const MenuEntry *entry = entries;
        while (entry->id >= 0) {
            if (entry->matches_key(UpCase(input->keyPressed))) {
                return entry->id;
            }
            if (entry->joy_button != 0 && input->joy_button_pressed(entry->joy_button, simulate)) {
                return entry->id;
            }
            entry++;
        }
    }
    return -1;
}