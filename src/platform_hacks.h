#ifndef __PLATFORM_HACKS_H__
#define __PLATFORM_HACKS_H__

#ifdef __GBA__
#define GBA_CODE_IWRAM __attribute__((section(".iwram"), long_call, target("arm")))
#else
#define GBA_CODE_IWRAM
#endif

#endif