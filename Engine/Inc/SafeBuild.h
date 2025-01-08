
#define LEGEND_BUILD 0
#define INFOGRAMES_BUILD 0
#define TIM_HESS 0
#define ARON_DRAYER 0
#define MATT_POWERS 0
#define EPIC_BUILD 0
#define NVIDIA_BUILD 0
#define ATARI_QA 0
#define IP_CHECKING (LEGEND_BUILD || INFOGRAMES_BUILD || TIM_HESS || ARON_DRAYER || MATT_POWERS || EPIC_BUILD || NVIDIA_BUILD || ATARI_QA)
#if IP_CHECKING
#undef TEXT
#undef HANDLE
#undef HINSTANCE
#include <winsock.h>
#ifdef DrawText
#undef DrawText
#endif
#endif
