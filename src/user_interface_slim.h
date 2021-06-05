#ifndef __USER_INTERFACE_SLIM_H__
#define __USER_INTERFACE_SLIM_H__

#include <cstdint>
#include "user_interface.h"

namespace ZZT {
    class UserInterfaceSlim: public UserInterface {
    private:
        VideoCopy *message_backup;

        VideoCopy *BackupSidebar();
        void RestoreSidebar(VideoCopy *copy);

    public:
        UserInterfaceSlim(Driver *driver);

		virtual void HackRunGameSpeedSlider(Game &game, bool editable, uint8_t &val) override;

        virtual bool SidebarPromptYesNo(const char *message, bool defaultReturn) override;
        virtual void SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, InputPromptMode mode) override;
        virtual void SidebarGameDraw(Game &game, uint32_t flags) override;
        virtual void SidebarShowMessage(uint8_t color, const char *message, bool temporary) override;
    };
}

#endif