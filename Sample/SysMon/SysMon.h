#pragma once
#include "SysMonCommon.h"
#include "../StaticCommonLib/StaticCommonLib.h"

template<typename T> struct FullItem {
	LIST_ENTRY Entry;
	T Data;
};

struct Globals {
	LIST_ENTRY ItemsHead;
	int ItemCount;
	int MaxItemCount;
	FastMutex Mutex;
};
