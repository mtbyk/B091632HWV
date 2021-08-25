#include "../StaticCommonLib/StaticCommonLib.h"
#include "SysMon.h"
#define DRIVER_TAG 'SyM'
#define DRIVER_PREFIX "SysMon: "

Globals g_Globals;

void PushItem(LIST_ENTRY* entry) {
	AutoLock<FastMutex> lock(g_Globals.Mutex);
	if (g_Globals.ItemCount > 1024) {
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}

void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(Process);
	if (CreateInfo) {
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
	    USHORT commandLineSize = 0;
		USHORT imageFileSize = 0;
		if (CreateInfo->CommandLine) {
			commandLineSize = CreateInfo->CommandLine->Length;
		}
		if (CreateInfo->ImageFileName) {
			imageFileSize = CreateInfo->ImageFileName->Length;
		}
		allocSize = commandLineSize + imageFileSize;
		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, (ULONG)DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX"failed to allocation\n"));
			return;
		}
		auto& item = info->Data;
		KeQuerySystemTime(&item.Time);
		item.Type = ItemType::ProcessCreate;
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize + imageFileSize;
		item.ProcessId = HandleToUlong(ProcessId);
		item.ParentProcessId = HandleToUlong(CreateInfo->ParentProcessId);

		if (commandLineSize > 0) {
			::memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}
		else {
			item.CommandLineLength = 0;
		}
		if (imageFileSize > 0) {
			::memcpy((UCHAR*)&item + sizeof(item) + commandLineSize, CreateInfo->ImageFileName->Buffer, imageFileSize);
			item.ImageFileNameLength = imageFileSize / sizeof(WCHAR);
			item.ImageFileNameOffset = sizeof(item) + commandLineSize;
		}
		PushItem(&info->Entry);
	}
	else {
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessExitInfo>), (ULONG)DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX"failed to allocation\n"));
			return;
		}
		auto& item = info->Data;
		KeQuerySystemTime(&item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToUlong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);

		PushItem(&info->Entry);
	}
}

void SysMonUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
	//TODO ListEntry‚Ì”jŠü

	KdPrint(("unloaded\n"));
}

int GetMaxRecordCount() {
	int maxCount = 1024;
	auto status = STATUS_SUCCESS;
	UNICODE_STRING valName= RTL_CONSTANT_STRING(L"maxCount");
	UNICODE_STRING regPath = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SOFTWARE\\SysMon\\");
	OBJECT_ATTRIBUTES objectAttributes;
	PKEY_VALUE_PARTIAL_INFORMATION pvalInfo = NULL;

	InitializeObjectAttributes(&objectAttributes, &regPath, 0, NULL, NULL);
	HANDLE hkey;
	do {
		status = ZwOpenKey(&hkey, KEY_READ, &objectAttributes);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to query open key (0x%08X)\n", status));
			break;
		}
		ULONG size = 0;
		status = ZwQueryValueKey(hkey, &valName, KeyValuePartialInformation, NULL, 0, &size);
		if (status != STATUS_BUFFER_TOO_SMALL) {
			KdPrint((DRIVER_PREFIX "failed to query value key size (0x%08X)\n", status));
			break;
		}
		pvalInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, size, (ULONG)DRIVER_TAG);
		if (pvalInfo == nullptr) {
			KdPrint((DRIVER_PREFIX "failed to allocation (0x%08X)\n", status));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		status = ZwQueryValueKey(hkey, &valName, KeyValuePartialInformation, pvalInfo, size, &size);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to query value key (0x%08X)\n", status));
			break;
		}
		if (pvalInfo->Type != REG_DWORD) {
			KdPrint((DRIVER_PREFIX "value type error (0x%08X)\n", status));
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		maxCount = *((int*)pvalInfo->Data);
	} while (false);
	if (hkey) {
		ZwClose(hkey);
	}
	if (pvalInfo) {
		ExFreePool(pvalInfo);
	}
	return maxCount;
}

extern "C" NTSTATUS

DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	auto status = STATUS_SUCCESS;

	g_Globals.MaxItemCount = GetMaxRecordCount();

	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\sysmon");
	bool symLinkCreated = false;

	do {
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\sysmon");
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}
		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create sym link (0x%08X)\n", status));
		}
		symLinkCreated = true;

		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to process callback (0x%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (symLinkCreated) {
			IoDeleteSymbolicLink(&symLink);
		}
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}
	DriverObject->DriverUnload = SysMonUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE];
	DriverObject->MajorFunction[IRP_MJ_CLOSE];
	DriverObject->MajorFunction[IRP_MJ_READ];

	return status;
}
