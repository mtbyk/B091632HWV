#include <ntddk.h>

void SampleUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint(("unloaded\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = SampleUnload;

	RTL_OSVERSIONINFOW info;
	RtlGetVersion(&info);
	KdPrint(("majar: %lu\nminor: %lu\nbuild: %lu\n", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber));

	return STATUS_SUCCESS;
}
