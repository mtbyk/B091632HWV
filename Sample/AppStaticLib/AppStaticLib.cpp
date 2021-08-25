// AppStaticLib.cpp : スタティック ライブラリ用の関数を定義します。
//

#include "pch.h"
#include "framework.h"

// TODO: これは、ライブラリ関数の例です

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}
