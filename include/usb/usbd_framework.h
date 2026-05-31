

#ifndef USBD_FRAMEWORK_H_
#define USBD_FRAMEWORK_H_

#include "shared_state.h"
#include "usb/usb_device.h"
#include "usb/usbd_driver.h"

void usbd_initialize(UsbDevice *usb_device);
void usbd_poll();
void usbd_update_joystick(JoystickState *state);

#endif /* USBD_FRAMEWORK_H_ */
