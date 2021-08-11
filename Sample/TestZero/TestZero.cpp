// TestZero.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <windows.h>
#include <stdio.h>
#include "../Zero/ZeroCommon.h"

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

int main()
{
	HANDLE hDevice = CreateFile(L"\\\\.\\Zero", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		return Error("Failed to open device");
	}
    
	BYTE buffer[64];

	for (int i = 0; i < sizeof(buffer); i++) {
		buffer[i] = i + 1;
	}
	DWORD bytes;
	BOOL ok = ::ReadFile(hDevice, buffer, sizeof(buffer), &bytes, nullptr);
	if (!ok) {
		return Error("failed to read");
	}
	if (bytes != sizeof(buffer)) {
		printf("Wrong Number of bytes\n");
	}
	long total = 0;
	for (auto n : buffer) {
		total += n;
	}
	if (total != 0) {
		printf("Wrong data\n");
	}

	BYTE buffer2[1024];
	ok = ::WriteFile(hDevice, buffer2, sizeof(buffer2), &bytes, nullptr);
	if (!ok) {
		return Error("failed to write");
	}
	if (bytes != sizeof(buffer2)) {
		printf("Wrong Number of bytes\n");
	}

	Bytes data;
	DWORD returned;
	BOOL success = DeviceIoControl(hDevice,
		IOCTL_GET_BYTES,
		&data, sizeof(data),
		nullptr, 0,
		&returned, nullptr);
	if (success) {
		printf("Read %ld  bytes\n", data.ReadBytes);
		printf("Write %ld  bytes\n", data.WriteBytes);
	}
	else {
		Error("Get bytes failed");
	}

	::CloseHandle(hDevice);
}

// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します
