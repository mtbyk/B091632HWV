#include "../StaticCommonLib/pch.h"
#include "ZeroCommon.h"
#define DRIVER_PREFIX "Zero: "

//DriverEntry

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

NTSTATUS AddBytes(PDEVICE_OBJECT DeviceObject, ULONG length, UCHAR code) {
	auto bytes = (Bytes*)DeviceObject->DeviceExtension;
	if (!bytes) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	switch (code)
	{
	case IRP_MJ_READ:
		bytes->ReadBytes += length;
		break;
	case IRP_MJ_WRITE:
		bytes->WriteBytes += length;
		break;
	default:
		return STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	return STATUS_SUCCESS;
}

NTSTATUS ZeroRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0) {
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);
	}
	auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);
	}
	memset(buffer, 0, len);
	AddBytes(DeviceObject, len, stack->MajorFunction);

	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}

NTSTATUS ZeroWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Write.Length;
	AddBytes(DeviceObject, len, stack->MajorFunction);

	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}

void ZeroUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Zero");
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("unloaded\n"));
}

NTSTATUS ZeroCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS ZeroDeviceContorol(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;
	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_GET_BYTES: {
		KdPrint(("debug1\n"));
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(Bytes)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = (Bytes*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		auto bytes = (Bytes*)DeviceObject->DeviceExtension;
		data->ReadBytes = bytes->ReadBytes;
		data->WriteBytes = bytes->WriteBytes;
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS

DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = ZeroUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = ZeroCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = ZeroRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = ZeroWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ZeroDeviceContorol;
	
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Zero");
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Zero");
	PDEVICE_OBJECT DeviceObject = nullptr;
	auto status = STATUS_SUCCESS;

	do {
		status = IoCreateDevice(DriverObject, sizeof(Bytes), &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (1x%08X)\n", status));
			break;
		}

		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (1x%08X)\n", status));
		}
	} while (FALSE);

	if (!NT_SUCCESS(status)) {
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}

	return status;
}

