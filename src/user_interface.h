#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

#include <cstdint>
#include "utils.h"
#include "driver.h"

#define SIDEBAR_FLAG_UPDATE 1
#define SIDEBAR_FLAG_SET_WORLD_NAME 2
#define SIDEBAR_REDRAW 0xFFFFFFFF

namespace ZZT {
    class Game;

    typedef enum {
        PMNumeric = 0,
        PMAlphanum = 1,
        PMAny = 2
    } PromptMode;

    struct MenuEntry {
        const int id;
        const uint16_t keys[4];
        const JoyButton joy_button;
        const char *name;
        const char *(*name_func)(Game*);

        const char *get_name(Game *game) const {
            return name_func != nullptr ? name_func(game) : name;
        }

        bool matches_key(uint16_t key) const {
            const uint16_t *keyptr = keys;
            while (*keyptr != 0) {
                if (key == *keyptr) {
                    return true;
                }
                keyptr++;
            }
            return false;
        }
    };

    class UserInterface {
    private:
        void SidebarClearLine(int y);
        void SidebarClear();

    protected:
        Driver *driver;

        void PromptString(int16_t x, int16_t y, uint8_t arrowColor, uint8_t color, int16_t width, PromptMode mode, char *buffer, int buflen);
        bool WaitYesNo(bool defaultReturn);

    public:
        UserInterface(Driver *driver);
        virtual ~UserInterface() { }

        virtual bool SidebarPromptYesNo(const char *message, bool defaultReturn);
        virtual void SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, PromptMode mode);
        virtual void PopupPromptString(const char *question, char *buffer, size_t buffer_len);
        virtual void SidebarGameDraw(Game &game, uint32_t flags);
        virtual void SidebarShowMessage(uint8_t color, const char *message);
        virtual int HandleMenu(Game &game, const MenuEntry *entries, bool simulate);

        inline void SidebarHideMessage() {
            SidebarShowMessage(0x0F, nullptr);
        }
    };
}

#endif