#include "usb/usbd_framework.h"
#include "Helpers/math.h"
#include "logger/logger.h"
#include "stddef.h"
#include "usb/usb_app.h"
#include "usb/usb_device.h"
#include "usb/usb_standards.h"
#include "usb/usbd_descriptors.h"
#include "usb/usbd_driver.h"
#include <usb/Hid/usb_hid_standards.h>

static UsbDevice *usbd_handle;
static void process_control_transfer_stage();
static void write_mouse_report();

extern uint8_t usb_rx_buffer[64];

static uint8_t mouse_ready     = 0;
static uint8_t hid_ready       = 0;
static uint8_t ep1_needs_prime = 0;

void usbd_initialize(UsbDevice *usb_device)
{
    usbd_handle = usb_device;
    usb_driver.initialize_gpio_pins();
    usb_driver.initialize_core();
    usb_driver.connect();

    for (volatile int i = 0; i < 1000000; i++)
        ;
}

void usbd_poll()
{
    usb_driver.poll();
}

static void usb_reset_received_handler()
{
    usbd_handle->in_data_size           = 0;
    usbd_handle->out_data_size          = 0;
    usbd_handle->configuration_value    = 0;
    usbd_handle->device_state           = USB_DEVICE_STATE_DEFAULT;
    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
    usbd_handle->ptr_out_buffer         = usb_rx_buffer;
    usb_driver.set_device_address(0);

    mouse_ready     = 0;
    hid_ready       = 0;
    ep1_needs_prime = 0;
}

// In usbd_framework.c — defer the EP1 prime until after EP0 status is done
void usbd_configure()
{
    usb_driver.configure_in_endpoint(
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F),
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bmAttributes & 0x03),
        configuration_descriptor_combination.usb_mouse_endpoint_descriptor.wMaxPacketSize);
    // Explicitly unmask XFRC for EP1 in DIEPMSK (should already be set but verify)
    SET_BIT(USB_OTG_FS_DEVICE->DIEPMSK, USB_OTG_DIEPMSK_XFRCM);

    log_info("DIEPMSK: 0x%08X", (unsigned int) USB_OTG_FS_DEVICE->DIEPMSK);
    log_info("DIEPEMPMSK: 0x%08X", (unsigned int) USB_OTG_FS_DEVICE->DIEPEMPMSK);
    log_info("DAINTMSK after configure: 0x%08X", (unsigned int) USB_OTG_FS_DEVICE->DAINTMSK);
    log_info("EP1 DIEPCTL: 0x%08X", (unsigned int) IN_ENDPOINT(1)->DIEPCTL);
    mouse_ready = 1; // mark ready; first poll after configure will send
}

static void process_standard_device_request()
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bRequest)
    {
        case USB_STANDARD_GET_DESCRIPTOR:
            log_info("Standard Get Descriptor request received.");
            const uint8_t descriptor_type    = request->wValue >> 8;
            const uint16_t descriptor_length = request->wLength;

            switch (descriptor_type)
            {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                    log_info("- Get Device Descriptor.");
                    usbd_handle->ptr_in_buffer = &device_descriptor;
                    usbd_handle->in_data_size  = MIN(descriptor_length, sizeof(device_descriptor));
                    ;

                    log_info("Switching control transfer stage to IN-DATA.");
                    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
                    break;
                case USB_DESCRIPTOR_TYPE_CONFIGURATION:
                    log_info("- Get Configuration Descriptor.");
                    usbd_handle->ptr_in_buffer = &configuration_descriptor_combination;
                    usbd_handle->in_data_size  = MIN(descriptor_length, sizeof(configuration_descriptor_combination));
                    log_info("Switching control transfer stage to IN-DATA.");
                    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
                    break;
                case USB_DESCRIPTOR_TYPE_QUALIFIER: // 0x06
                    // We are Full Speed only — stall this request
                    SET_BIT(IN_ENDPOINT(0)->DIEPCTL, USB_OTG_DIEPCTL_STALL);
                    SET_BIT(OUT_ENDPOINT(0)->DOEPCTL, USB_OTG_DOEPCTL_STALL);
                    break;
            }
            break;
        case USB_STANDARD_SET_ADDRESS:
            log_info("Standard Set Address request received.");
            const uint16_t device_address = request->wValue;
            usb_driver.set_device_address(device_address);
            usbd_handle->device_state = USB_DEVICE_STATE_ADDRESSED;
            log_info("Switching control transfer stage to IN-STATUS.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN;
            break;
        case USB_STANDARD_SET_CONFIG:
            log_info("Standard Set Configuration request received.");
            usbd_handle->configuration_value = request->wValue;
            usbd_configure();
            usbd_handle->device_state = USB_DEVICE_STATE_CONFIGURED;
            log_info("Switching control transfer stage to IN-STATUS.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN;
            break;
    }
}

static void process_class_interface_request()
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bRequest)
    {
        case USB_HID_SETIDLE:
            log_info("Switching control transfer stage to IN-STATUS.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_IN; // received request succesgully
            break;
    }
}

static void process_standard_interface_request()
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;
    uint8_t descriptor_type   = request->wValue >> 8;

    log_info("Standard interface request: wValue=0x%04X type=0x%02X", request->wValue, descriptor_type);

    switch (descriptor_type)
    {
        case USB_DESCRIPTOR_TYPE_HID_REPORT:
            log_info("Sending HID report descriptor (%d bytes)", sizeof(hid_report_descriptor));
            usbd_handle->ptr_in_buffer          = &hid_report_descriptor;
            usbd_handle->in_data_size           = sizeof(hid_report_descriptor);
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
            hid_ready                           = 1;
            break;

        case USB_DESCRIPTOR_TYPE_HID:
            usbd_handle->ptr_in_buffer = &configuration_descriptor_combination.usb_mouse_hid_descriptor;

            usbd_handle->in_data_size = sizeof(UsbHidDescriptor);

            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
            break;

        default:
            log_info("Unhandled interface descriptor type: 0x%02X", descriptor_type);
            break;
    }
}

static void process_request()
{
    UsbRequest const *request = usbd_handle->ptr_out_buffer;

    switch (request->bmRequestType & (USB_BM_REQUEST_TYPE_TYPE_MASK | USB_BM_REQUEST_TYPE_RECIPIENT_MASK))
    {
        case USB_BM_REQUEST_TYPE_TYPE_STANDARD | USB_BM_REQUEST_TYPE_RECIPIENT_DEVICE:
            process_standard_device_request();
            break;
        case USB_BM_REQUEST_TYPE_TYPE_CLASS |
            USB_BM_REQUEST_TYPE_RECIPIENT_INTERFACE: // request for the HID intefyce -> SET_IDDLE
            process_class_interface_request();

            break;
        case USB_BM_REQUEST_TYPE_TYPE_STANDARD |
            USB_BM_REQUEST_TYPE_RECIPIENT_INTERFACE: // target not whole devixe, but only specific interface
            process_standard_interface_request();

            break;
    }
}

static void process_control_transfer_stage()
{
    switch (usbd_handle->control_transfer_stage)
    {
        case USB_CONTROL_STAGE_SETUP:
            break;
        case USB_CONTROL_STAGE_DATA_IN:
            log_info("Processing IN-DATA stage.");

            uint8_t data_size = MIN(usbd_handle->in_data_size, device_descriptor.bMaxPacketSize0);

            usb_driver.write_packet(0, usbd_handle->ptr_in_buffer, data_size);
            usbd_handle->in_data_size -= data_size;
            usbd_handle->ptr_in_buffer += data_size;

            log_info("Switching control stage to IN-DATA IDLE.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN_IDLE;

            if (usbd_handle->in_data_size == 0)
            {
                if (data_size == device_descriptor.bMaxPacketSize0)
                {
                    log_info("Switching control stage to IN-DATA ZERO.");
                    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN_ZERO;
                }
                else
                {
                    log_info("Switching control stage to OUT-STATUS.");
                    usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_OUT;
                }
            }

            break;
        case USB_CONTROL_STAGE_DATA_IN_IDLE:
            break;
        case USB_CONTROL_STAGE_STATUS_OUT:
            log_info("Switching control stage to SETUP.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
            break;
        case USB_CONTROL_STAGE_STATUS_IN:
            log_info("STATUS_IN: sending ZLP on EP0...");

            usb_driver.write_packet(0, NULL, 0);
            log_info("STATUS_IN: ZLP sent, switching to SETUP.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;

            // If we just finished SET_CONFIG, kick off the first HID report
            if (usbd_handle->device_state == USB_DEVICE_STATE_CONFIGURED && mouse_ready)
            {
                log_info("STATUS_IN: device configured, NOT sending  first mouse report.");
            }
            break;
    }
}

static void write_mouse_report()
{
    HidReport report = {
        .buttons  = 0,
        .x        = 5,
        .y        = 0,
        .reserved = 0,
    };

    log_info(
        "HidReport size=%d wMaxPacketSize=%d",
        (int) sizeof(report),
        (int) configuration_descriptor_combination.usb_mouse_endpoint_descriptor.wMaxPacketSize);

    usb_driver.write_packet(
        (configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F),
        &report,
        sizeof(report));

    if (!(IN_ENDPOINT(1)->DIEPCTL & USB_OTG_DIEPCTL_EPENA))
    {
        SET_BIT(IN_ENDPOINT(1)->DIEPCTL, USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK);
    }

    // Read back EP1 state immediately after write
    log_info("EP1 DIEPCTL after write: 0x%08X", (unsigned int) IN_ENDPOINT(1)->DIEPCTL);
    log_info("EP1 DTXFSTS (free words): %lu", IN_ENDPOINT(1)->DTXFSTS & 0xFFFF);
}

// In usbd_framework.c — fix the handler to separate EP0 and EP1 logic
static void in_transfer_completed_handler(uint8_t endpoint_number)
{
    log_info("IN transfer completed for EP%d", endpoint_number);

    uint8_t mouse_ep = configuration_descriptor_combination.usb_mouse_endpoint_descriptor.bEndpointAddress & 0x0F;

    if (endpoint_number == 0)
    {
        log_info(
            "EP0 IN transfer complete. in_data_size=%d stage=%d",
            usbd_handle->in_data_size,
            usbd_handle->control_transfer_stage);

        if (usbd_handle->in_data_size)
        {
            log_info("More control data to send, switching to IN-DATA.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
        }
        else if (usbd_handle->control_transfer_stage == USB_CONTROL_STAGE_DATA_IN_ZERO)
        {
            usb_driver.write_packet(0, NULL, 0);
            log_info("Sent ZLP, switching to OUT-STATUS.");
            usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_STATUS_OUT;
        }
        // When HID report descriptor transfer completes, set prime flag
        else if (hid_ready && usbd_handle->control_transfer_stage == USB_CONTROL_STAGE_STATUS_OUT)
        {
            log_info("HID prime condition met: stage=%d hid_ready=%d", usbd_handle->control_transfer_stage, hid_ready);
            ep1_needs_prime = 1;
            hid_ready       = 0;
        }
    }
    else if (endpoint_number == mouse_ep)
    {
        log_info("EP1 IN transfer completed — sending next report.");
        write_mouse_report();
    }
}

static void out_transfer_completed_handler(uint8_t endpoint_number)
{
}

static void setup_data_received_handler(uint8_t endpoint_number, uint16_t byte_count)
{
    usb_driver.read_packet(usbd_handle->ptr_out_buffer, byte_count);

    // Prints out the received data.
    log_debug_array("SETUP data: ", usbd_handle->ptr_out_buffer, byte_count);

    process_request();
}

static void usb_polled_handler()
{
    process_control_transfer_stage();

    if (ep1_needs_prime && usbd_handle->control_transfer_stage == USB_CONTROL_STAGE_SETUP)
    {
        ep1_needs_prime = 0;
        log_info("Priming EP1 from poll handler.");
        write_mouse_report();
    }
}

UsbEvents usb_events = {
    .on_usb_reset_received     = &usb_reset_received_handler,
    .on_setup_data_received    = &setup_data_received_handler,
    .on_usb_polled             = &usb_polled_handler,
    .on_in_transfer_completed  = &in_transfer_completed_handler,
    .on_out_transfer_completed = &out_transfer_completed_handler};
