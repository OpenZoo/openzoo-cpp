#ifndef __PLATFORM_HACKS_H__
#define __PLATFORM_HACKS_H__

#ifdef __GBA__
#define GBA_CODE_IWRAM __attribute__((section(".iwram"), long_call, target("arm")))
#define GBA_DATA_IWRAM __attribute__((section(".iwram")))

void zoo_gba_sleep(void);
#else
#define GBA_CODE_IWRAM
#define GBA_DATA_IWRAM
#endif

#endif