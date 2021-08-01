#include <ntddk.h>

NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);
	return STATUS_SUCCESS;
}

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint(("unloaded\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = PriorityBoosterUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;

	RTL_OSVERSIONINFOW info;
	RtlGetVersion(&info);
	KdPrint(("majar: %lu\nminor: %lu\nbuild: %lu\n", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber));

	return STATUS_SUCCESS;
}
