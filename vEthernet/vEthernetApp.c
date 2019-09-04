#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/Tcp4.h>
#include <Protocol/ServiceBinding.h>

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
vEthernetAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  UINTN                     NumHandles;
  UINTN                     LoopIndex;
  EFI_HANDLE                *AllHandles;

  EFI_HANDLE                      TcpHandle;
  EFI_SERVICE_BINDING_PROTOCOL    *TcpServiceBinding;
  EFI_TCP4_PROTOCOL               *TcpListener;
  EFI_IP4_MODE_DATA               Ip4ModeData;
  EFI_IPv4_ADDRESS                Ip4Address;
  EFI_IPv4_ADDRESS                Ip4SubnetMaskAddress;

  EFI_TCP4_CONFIG_DATA            TcpConfigData = {
    0x00,                                           // IPv4 Type of Service
    255,                                            // IPv4 Time to Live
    {                                               // AccessPoint:
      TRUE,                                         // Use default address
      { {0, 0, 0, 0} },                             // IP Address  (ignored - use default)
      { {0, 0, 0, 0} },                             // Subnet mask (ignored - use default)
      0x80,                             // Station port
      { {0, 0, 0, 0} },                             // Remote address: accept any
      0,                                            // Remote Port: accept any
      FALSE                                         // ActiveFlag: be a "server"
    },
    NULL                                            // Default advanced TCP options
  };


  // Ethernet
  Status = gBS->LocateHandleBuffer (
             ByProtocol,
             &gEfiTcp4ServiceBindingProtocolGuid,
             NULL,
             &NumHandles,
             &AllHandles
           );
  ASSERT_EFI_ERROR (Status);
  Print (L"\nConfig Ethernet Info ...  find %d eth instance(s)!\n", NumHandles);

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Print (L" - Ethernet [%02x]\n", LoopIndex);
    // Open a passive TCP instance
    Status = gBS->OpenProtocol (
              AllHandles[LoopIndex],
              &gEfiTcp4ServiceBindingProtocolGuid,
              (VOID **) &TcpServiceBinding,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
             );
    ASSERT_EFI_ERROR (Status);

    Status = TcpServiceBinding->CreateChild (TcpServiceBinding, &TcpHandle);
    ASSERT_EFI_ERROR (Status);

    Print (L"         Openning TCP Listener ... ");
    Status = gBS->OpenProtocol (
              TcpHandle,
              &gEfiTcp4ProtocolGuid,
              (VOID **) &TcpListener,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
             );
    ASSERT_EFI_ERROR (Status);
    Print (L"Done! \n");

    // Set up re-usable tokens
    // Configure the TCP instance
    Print (L"         TCP Listener->Config ... ");
    Status = TcpListener->Configure (TcpListener, &TcpConfigData);
    if ((Status == EFI_NO_MAPPING) || (Status == EFI_SUCCESS)) {
      Print (L"Done! \n");
  
      Print (L"         TCP Listener->GetModeData ... ");
      do {
        Status = TcpListener->GetModeData (TcpListener, NULL, NULL, &Ip4ModeData, NULL, NULL);
      } while (!Ip4ModeData.IsConfigured);
      Print (L"Done! \n");

      Status = TcpListener->Configure (TcpListener, &TcpConfigData);
      Ip4Address = Ip4ModeData.ConfigData.StationAddress;
      Print (L"         IPv4 address: %d.%d.%d.%d \n", Ip4Address.Addr[0], Ip4Address.Addr[1], Ip4Address.Addr[2], Ip4Address.Addr[3]);
      Ip4SubnetMaskAddress = Ip4ModeData.ConfigData.SubnetMask;
      Print (L"         Subnet mask: %d.%d.%d.%d \n\n", Ip4SubnetMaskAddress.Addr[0], Ip4SubnetMaskAddress.Addr[1], Ip4SubnetMaskAddress.Addr[2], Ip4SubnetMaskAddress.Addr[3]);
    } else {
      Print (L"FAILED (%r)! \n", Status);
      return Status;
    }

    // Tell the user our address and hostname
    // Start listening for a connection

    // Stop everything
    TcpListener->Configure (TcpListener, NULL);
    TcpServiceBinding->DestroyChild (TcpServiceBinding, &TcpHandle);
  }

  FreePool (AllHandles);
  return EFI_SUCCESS;
}