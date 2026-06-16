#include "mouse.h"

PDEVICE_OBJECT mouse_device_object = nullptr;
pMouseClassServiceCallback MouseClassServiceCallback = nullptr;

extern "C"
{
	NTSYSCALLAPI POBJECT_TYPE* IoDriverObjectType;
	NTSYSCALLAPI NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectName, ULONG Attributes,
		PACCESS_STATE AccessState, ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType,
		KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* Object);
}

bool mouse::init()
{
	UNICODE_STRING mouclass_string;
	RtlInitUnicodeString(&mouclass_string, L"\\Driver\\MouClass");

	PDRIVER_OBJECT mouclass_driver_object = nullptr;
	NTSTATUS status = ObReferenceObjectByName(&mouclass_string, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, nullptr, (PVOID*)&mouclass_driver_object);
	if (!NT_SUCCESS(status))
	{
		if (mouclass_driver_object)
			ObDereferenceObject(mouclass_driver_object);

		return false;
	}
		

	UNICODE_STRING mouhid_string;
	RtlInitUnicodeString(&mouhid_string, L"\\Driver\\MouHID");
	
	PDRIVER_OBJECT mouhid_driver_object = nullptr;

	status = ObReferenceObjectByName(&mouhid_string, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, nullptr, (PVOID*)&mouhid_driver_object);
	if (!NT_SUCCESS(status))
	{
		if (mouclass_driver_object)
			ObDereferenceObject(mouclass_driver_object);
		if (mouhid_driver_object)
			ObDereferenceObject(mouhid_driver_object);

		return false;
	}
		

	PDEVICE_OBJECT mouhid_device_object = mouhid_driver_object->DeviceObject;
	while (mouhid_device_object && !MouseClassServiceCallback)
	{
		PDEVICE_OBJECT mouclass_device_object = mouclass_driver_object->DeviceObject;
		while (mouclass_device_object && !MouseClassServiceCallback)
		{
			PULONG_PTR device_extension = (PULONG_PTR)mouhid_device_object->DeviceExtension;
			ULONG_PTR device_extension_size = ((ULONG_PTR)mouhid_device_object->DeviceObjectExtension - (ULONG_PTR)mouhid_device_object->DeviceExtension) / 8;
			for (size_t i = 0; i < (device_extension_size - 1); ++i)
			{
				if (device_extension[i] == (ULONG_PTR)mouclass_device_object && device_extension[i + 1] > (ULONG_PTR)mouclass_driver_object->DriverStart)
				{
					MouseClassServiceCallback = (pMouseClassServiceCallback)device_extension[i + 1];
					mouse_device_object = mouclass_device_object;
					break;
				}
			}
			mouclass_device_object = mouclass_device_object->NextDevice;
		}
		mouhid_device_object = mouhid_device_object->AttachedDevice;
	}

	ObDereferenceObject(mouhid_driver_object);
	ObDereferenceObject(mouclass_driver_object);
	
	return(MouseClassServiceCallback != nullptr);
}

void mouse::move(int x, int y, unsigned short flags)
{
	if (!MouseClassServiceCallback || !mouse_device_object)
		return;

	KIRQL irql;

	MOUSE_INPUT_DATA data{};
	data.LastX = x;
	data.LastY = y;
	data.s.ButtonFlags = flags;

	KeRaiseIrql(DISPATCH_LEVEL, &irql);

	ULONG consumed = 0;
	MouseClassServiceCallback(mouse_device_object, &data, &data + 1, &consumed);

	KeLowerIrql(irql);
}
