#define WINDOWS_IGNORE_PACKING_MISMATCH
#include <ntifs.h>

#include "communication.h"
#include "mem/mem.h"
#include "mouse/mouse.h"

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;

#define OFFSET_DTB 0x28
#define OFFSET_SECTION_BASE 0x2b0

extern "C"
{
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
}

namespace driver
{
	NTSTATUS on_create(PDEVICE_OBJECT, PIRP irp)
	{
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	NTSTATUS on_close(PDEVICE_OBJECT, PIRP irp)
	{
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	NTSTATUS ioctl_handler(PDEVICE_OBJECT device_object, PIRP irp)
	{
		UNREFERENCED_PARAMETER(device_object);

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
		PVOID buffer = irp->AssociatedIrp.SystemBuffer;
		ULONG bytes_io = 0;

		if (!stack || !buffer)
		{
			irp->IoStatus.Status = status;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		switch (stack->Parameters.DeviceIoControl.IoControlCode)
		{
		case codes::get_cr3:
		{
			if (stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(request::cr3))
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			request::cr3* req = reinterpret_cast<request::cr3*>(buffer);

			PEPROCESS eprocess = mem::get_eprocess_by_pid(req->pid);
			if (eprocess)
			{
				req->cr3 = *(QWORD*)((BYTE*)eprocess + OFFSET_DTB);
				status = STATUS_SUCCESS;
			}

			bytes_io = sizeof(request::cr3);
			break;
		}
		case codes::get_base:
		{
			if (stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(request::base))
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			request::base* req = reinterpret_cast<request::base*>(buffer);

			PEPROCESS eprocess = mem::get_eprocess_by_pid(req->pid);
			if (eprocess)
			{
				req->base = reinterpret_cast<uintptr_t>(*(PVOID*)((BYTE*)eprocess + OFFSET_SECTION_BASE));
				status = STATUS_SUCCESS;
			}

			bytes_io = sizeof(request::base);
			break;
		}
		case codes::read:
		{
			if (stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(request::mem_op))
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			request::mem_op* req = reinterpret_cast<request::mem_op*>(buffer);
			if (!req->cr3 || !req->buffer || !req->target)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
				

			status = mem::read_virtual(req->cr3, req->target, req->buffer, req->size);
			
			bytes_io = sizeof(request::mem_op);
			break;
		}
		case codes::write:
		{
			if (stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(request::mem_op))
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			request::mem_op* req = reinterpret_cast<request::mem_op*>(buffer);
			if (!req->cr3 || !req->buffer || !req->target)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
				

			status = mem::write_virtual(req->cr3, req->target, req->buffer, req->size);

			bytes_io = sizeof(request::mem_op);
			break;
		}
		case codes::mouse_move:
		{
			if (stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(request::mouse_move))
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			request::mouse_move* req = reinterpret_cast<request::mouse_move*>(buffer);

			mouse::move(req->x, req->y, req->flags);

			bytes_io = sizeof(request::mouse_move);
			status = STATUS_SUCCESS;
			break;
		}
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = bytes_io;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}
}

NTSTATUS entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING path)
{
	UNREFERENCED_PARAMETER(path);

	UNICODE_STRING device_name;
	RtlInitUnicodeString(&device_name, L"\\Device\\reak");

	UNICODE_STRING sym_link;
	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\reak");

	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	if (!NT_SUCCESS(status))
		return status;

	status = IoCreateSymbolicLink(&sym_link, &device_name);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(device_object);
		return status;
	}
		

	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::on_create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::on_close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::ioctl_handler;

	if (!mouse::init())
	{
		IoDeleteSymbolicLink(&sym_link);
		IoDeleteDevice(device_object);

		return STATUS_UNSUCCESSFUL;
	}

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry()
{
	UNICODE_STRING driver_name;
	RtlInitUnicodeString(&driver_name, L"\\Driver\\reak");
	return IoCreateDriver(&driver_name, &entry);
}