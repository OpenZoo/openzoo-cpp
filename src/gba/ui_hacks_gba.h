#ifndef __UI_HACKS_GBA_H__
#define __UI_HACKS_GBA_H__

#include <cstdint>
#include "../user_interface.h"
#include "../user_interface_super_zzt.h"
#include "../txtwind.h"
#include "driver_gba.h"

namespace ZZT {
	class TextWindowSuperZZTGBA : public TextWindow {
	public:
        TextWindowSuperZZTGBA(Driver *driver, FilesystemDriver *filesystem, int16_t w_x, int16_t w_y, int16_t w_width, int16_t w_height)
			: TextWindow(driver, filesystem, w_x, w_y, w_width, w_height) { }

        virtual void DrawOpen(void) override;
        virtual void DrawClose(void) override;
	};

    class UserInterfaceSuperZZTGBA: public UserInterfaceSuperZZT {
    public:
        UserInterfaceSuperZZTGBA(Driver *driver);

		virtual TextWindow *CreateTextWindow(FilesystemDriver *fsDriver) override;
		void PopupPromptString(const char *question, char *buffer, size_t buffer_len) override;
    };
}

#endif