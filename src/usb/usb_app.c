#include "usb/usb_app.h"
#include "logger/logger.h"
#include "usb/usbd_framework.h"

UsbDevice usb_device;
uint8_t usb_rx_buffer[64];

void usb_app_init(void)
{
    log_info("USB: initializing");
    usb_device.ptr_out_buffer = usb_rx_buffer;
    usbd_initialize(&usb_device);
}
