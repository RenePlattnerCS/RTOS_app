#ifndef USBD_DESCRIPTORS_H_
#define USBD_DESCRIPTORS_H_

#include "Hid/usb_hid_standards.h"
#include "usb/usb_standards.h"

const UsbDeviceDescriptor device_descriptor = {
    .bLength            = sizeof(UsbDeviceDescriptor),
    .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB             = 0x0200, // 0xJJMN
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = USB_SUBCLASS_NONE,
    .bDeviceProtocol    = USB_PROTOCOL_NONE,
    .bMaxPacketSize0    = 64, // was 8
    .idVendor           = 0x6666,
    .idProduct          = 0x13AC,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0,
    .iProduct           = 0,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1,
};

// usbd_descriptors.h

const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)

    0x05, 0x09, //     Usage Page (Buttons)
    0x19, 0x01, //     Usage Minimum (1)
    0x29, 0x03, //     Usage Maximum (3)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data,Var,Abs) — 3 button bits

    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x03, //     Input (Const) — 5 padding bits → byte 0 complete

    0x05, 0x01, //     Usage Page (Generic Desktop)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x06, //     Input (Data,Var,Rel) — X byte 1, Y byte 2

    0x95, 0x01, //     Report Count (1)   ← NEW
    0x75, 0x08, //     Report Size (8)    ← NEW
    0x81, 0x03, //     Input (Const)      ← NEW — padding byte 3

    0xC0, //   End Collection
    0xC0  // End Collection
};

typedef struct __attribute__((packed))
{
    uint8_t buttons;
    int8_t x;
    int8_t y;
    uint8_t reserved;
} HidReport;

typedef struct
{
    UsbConfigurationDescriptor usb_configuration_descriptor;
    UsbInterfaceDescriptor usb_interface_descriptor;
    UsbHidDescriptor usb_mouse_hid_descriptor;
    UsbEndpointDescriptor usb_mouse_endpoint_descriptor;
} UsbConfigurationDescriptorCombination;

const UsbConfigurationDescriptorCombination configuration_descriptor_combination = {
    .usb_configuration_descriptor =
        {
            .bLength         = sizeof(UsbConfigurationDescriptor),
            .bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION,
            .wTotalLength    = sizeof(UsbConfigurationDescriptorCombination),
            .bNumInterfaces  = 1, // should be 0!!!? Only 1 function => mouse 0> need only 1 group of endpoints => if i
                                  // also want a keyboard => need 2 interfaces
            .bConfigurationValue = 1,
            .iConfiguration      = 0,
            .bmAttributes        = 0x80,
            .bMaxPower           = 25 // 50mA,
        },
    .usb_interface_descriptor =
        {.bLength            = sizeof(UsbInterfaceDescriptor),
         .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
         .bInterfaceNumber   = 0,
         .bAlternateSetting  = 0,
         .bNumEndpoints      = 1,
         .bInterfaceClass    = USB_CLASS_HID,
         .bInterfaceSubClass = USB_PROTOCOL_NONE,
         .bInterfaceProtocol = USB_PROTOCOL_NONE,
         .iInterface         = 0},
    .usb_mouse_hid_descriptor =
        {.bLength            = sizeof(UsbHidDescriptor),
         .bDescriptorType    = USB_DESCRIPTOR_TYPE_HID,
         .bcdHID             = 0x0100,
         .bCountryCode       = USB_HID_COUNTRY_NONE,
         .bNumDescriptors    = 1,
         .bDescriptorType0   = USB_DESCRIPTOR_TYPE_HID_REPORT,
         .wDescriptorLength0 = sizeof(hid_report_descriptor)},
    .usb_mouse_endpoint_descriptor =
        {
            // this entpoint will transver the data ov the mouse => position and buttons
            .bLength          = sizeof(UsbEndpointDescriptor),
            .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
            .bEndpointAddress = 0x81, // choose In entpoint(8) number 1
            .bmAttributes     = USB_ENDPOINT_TYPE_INTERRUPT,
            .wMaxPacketSize   = 4, // mouse only needs 4 bytes was 64!!
            .bInterval        = 64 // poll every 10ms (was 50ms = very sluggish)
        }

};

#endif /* USBD_DESCRIPTORS_H_ */
