#ifndef __USER_INTERFACE_SUPER_ZZT_H__
#define __USER_INTERFACE_SUPER_ZZT_H__

#include <cstdint>
#include "user_interface.h"

namespace ZZT {
    class UserInterfaceSuperZZT: public UserInterface {
    private:
        VideoCopy *message_backup;

        VideoCopy *BackupSidebar();
        void RestoreSidebar(VideoCopy *copy);
		void ClearSidebar();
		uint8_t GetSidebarColor();

	protected:
		int16_t width, height;

    public:
        UserInterfaceSuperZZT(Driver *driver, int width, int height);

		virtual void ConfigureViewport(int16_t &x, int16_t &y, int16_t &width, int16_t &height) override;
		virtual TextWindow *CreateTextWindow(FilesystemDriver *fsDriver) override;

        virtual bool SidebarPromptYesNo(const char *message, bool defaultReturn) override;
        virtual void SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, InputPromptMode mode) override;
        virtual void SidebarGameDraw(Game &game, uint32_t flags) override;
        virtual void SidebarShowMessage(uint8_t color, const char *message, bool temporary) override;
    };
}

#endif