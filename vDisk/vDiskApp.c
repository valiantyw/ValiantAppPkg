#include <Uefi.h>

#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePathFromText.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
vDiskAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  UINTN                     NumHandles;
  UINTN                     LoopIndex;
  EFI_HANDLE                *AllHandles;

  EFI_INPUT_KEY             Key;
  UINTN                     EventIndex;
  UINTN                     DiskMode;
  CHAR16                    *DevPathString;
  EFI_DISK_INFO_PROTOCOL    *DiskInfo;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Status = gBS->LocateHandleBuffer (
             ByProtocol,
             &gEfiBlockIoProtocolGuid,
             NULL,
             &NumHandles,
             &AllHandles
           );
  ASSERT_EFI_ERROR (Status);

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Status = gBS->OpenProtocol (
              AllHandles[LoopIndex],
              &gEfiBlockIoProtocolGuid,
              (VOID **) &BlockIo,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
             );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->OpenProtocol (
              AllHandles[LoopIndex],
              &gEfiDiskInfoProtocolGuid,
              (VOID **) &DiskInfo,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
             );
    ASSERT_EFI_ERROR (Status);

    DevicePath = DevicePathFromHandle(AllHandles[LoopIndex]);
    DevPathString = ConvertDevicePathToText(DevicePath, TRUE, TRUE);

    if (CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)) { // IDE
      DiskMode = 0x0;
    } else if (CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid)) {  // AHCI
      DiskMode = 0x1;
    } else if (CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoUsbInterfaceGuid)) {  // USB
      DiskMode = 0x2;
    } else if (CompareGuid(&DiskInfo->Interface, &gEfiDiskInfoNvmeInterfaceGuid)) {  // NVMe
      DiskMode = 0x3;
    } else {
      DiskMode = 0xF; // Unknow
    }

    if (!(BlockIo->Media->LogicalPartition)) { // DISK
      Print (L"\n[%02x] ", LoopIndex);
      Print (DiskMode == 0x0 ? L"   IDE": (DiskMode == 0x1 ? L"  AHCI": (DiskMode == 0x2 ? L"   USB": (DiskMode == 0x3 ? L"  NVMe": L"Unknow")))); // Type field, 6 Chars
      Print (L" Disk        %S\n", DevPathString);
    } else { // PARTITION
      Print (L"[%02x]       ", LoopIndex);  // Align with Type field, 6 Chars
      Print (L"- Partition  %S\n", DevPathString);
      continue;
    }
  }

  // Waiting a key
  Print (L"\nPress ESC key to exit Application! \n");
  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  ASSERT_EFI_ERROR (Status);

  if (Key.UnicodeChar == 0 && Key.ScanCode == SCAN_ESC) {
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}