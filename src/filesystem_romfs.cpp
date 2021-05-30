#include <cstdio>
#include <cstdlib>
#include "filesystem_romfs.h"

using namespace ZZT;

// TODO: Add directory support.

static enum {
    ROMFS_TYPE_ANY = -1,
	ROMFS_TYPE_HLINK = 0,
	ROMFS_TYPE_DIR = 1,
	ROMFS_TYPE_FILE = 2,
	ROMFS_TYPE_SLINK = 3,
	ROMFS_TYPE_BLOCK = 4,
	ROMFS_TYPE_CHAR = 5,
	ROMFS_TYPE_SOCKET = 6,
	ROMFS_TYPE_FIFO = 7
} RomfsType;

static inline uint32_t romfs_read32(const uint8_t *ptr) {
	return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

RomfsFilesystemDriver::RomfsFilesystemDriver(const void *ptr)
    : PathFilesystemDriver("/", 128, '/', true) {
    this->ptr = (const uint8_t *) ptr;
    this->valid = memcmp(this->ptr, "-rom1fs-", 8) == 0;
	if (this->valid) {
		this->root_ptr = this->ptr + ((16 + strlen((char*) (this->ptr + 16)) + 16) & (~15));
	}
}

IOStream *RomfsFilesystemDriver::open_file_absolute(const char *name, bool write) {
	const uint8_t *curr_entry = this->root_ptr;
	const uint8_t *file_ptr;
	const uint8_t *base = this->ptr;
	uint32_t offset;

	if (!this->valid || write) {
        return new ErroredIOStream();
    }

	// skip root path
	if (name[0] == '/') {
		name++;
	}

	// search for file
	while (true) {
		offset = romfs_read32(curr_entry);
		if ((offset & 7) == ROMFS_TYPE_FILE && !strcasecmp(name, (char *) (curr_entry + 16))) {
			// correct file
			file_ptr = curr_entry + ((16 + strlen((char*) (curr_entry + 16)) + 16) & (~15));
            return new MemoryIOStream(file_ptr, romfs_read32(curr_entry + 8));
		}

		// advance
		curr_entry = base + (offset & (~15));
		if (curr_entry == base) {
			break;
		}
	}

	// did not find file
	return new ErroredIOStream();
}

bool RomfsFilesystemDriver::list_files(std::function<bool(FileEntry&)> callback) {
	FileEntry ent;
	const uint8_t *curr_entry = this->root_ptr;
	const uint8_t *base = this->ptr;
	uint32_t offset;

    if (!this->valid) {
        return false;
    }
	
	while (true) {
		offset = romfs_read32(curr_entry);
		if ((offset & 7) == ROMFS_TYPE_FILE) {
			ent.is_dir = ((offset & 7) == ROMFS_TYPE_DIR);
            ent.filename = (const char *) (curr_entry + 16);

			if (!callback(ent)) {
				break;
			}
		}

		curr_entry = base + (offset & (~15));
		if (curr_entry == base) {
			break;
		}
	}

	return true;
}

bool RomfsFilesystemDriver::has_parent(void) {
    return false;
}
