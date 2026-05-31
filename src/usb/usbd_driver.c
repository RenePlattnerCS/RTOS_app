#include "usb/usbd_driver.h"
#include "logger/logger.h"
#include "string.h"
#include "usb/usb_standards.h"

static void configure_rxfifo_size(uint16_t size);
static void configure_txfifo_size(uint8_t endpoint_number, uint16_t size);

void initialize_gpio_pins()
{
    // Add to initialize_gpio_pins()
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);

    // Configure PD12 as output
    MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODER12, _VAL2FLD(GPIO_MODER_MODER12, 1));
    // Enable GPIOA clock
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

    // Set alternate function 10 for PA11 (DM) and PA12 (DP)
    MODIFY_REG(
        GPIOA->AFR[1],
        GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12,
        _VAL2FLD(GPIO_AFRH_AFSEL11, 0xA) | _VAL2FLD(GPIO_AFRH_AFSEL12, 0xA));

    // Configure USB pins to work in alternate function mode
    MODIFY_REG(
        GPIOA->MODER,
        GPIO_MODER_MODER11 | GPIO_MODER_MODER12,
        _VAL2FLD(GPIO_MODER_MODER11, 2) | _VAL2FLD(GPIO_MODER_MODER12, 2));

    // Set to high speed
    MODIFY_REG(
        GPIOA->OSPEEDR,
        GPIO_OSPEEDR_OSPEED11 | GPIO_OSPEEDR_OSPEED12,
        _VAL2FLD(GPIO_OSPEEDR_OSPEED11, 3) | _VAL2FLD(GPIO_OSPEEDR_OSPEED12, 3));

    // No pull-up/pull-down
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPDR11 | GPIO_PUPDR_PUPDR12, 0);

    // PA9 = VBUS — input, no pull
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER9, 0); // input mode
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPDR9, 0); // no pull
}

void initialize_core()
{
    // Enable USB OTG FS clock
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_OTGFSEN);

    // Wait for clock to be ready
    for (volatile int i = 0; i < 10000; i++)
        ;

    // Core soft reset
    SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_CSRST);

    // Wait for reset to complete
    uint32_t timeout = 100000;
    while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_CSRST) && --timeout)
        ;

    if (timeout == 0)
    {
        log_info("USB Core Reset FAILED!");
        return;
    }

    // Wait for AHB idle
    timeout = 100000;
    while (!READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL) && --timeout)
        ;

    if (timeout == 0)
    {
        log_info("USB AHB Idle timeout!");
        return;
    }

    // Small delay after reset
    for (volatile int i = 0; i < 10000; i++)
        ;

    // Configure USB: Force device mode, set turnaround time
    MODIFY_REG(
        USB_OTG_FS->GUSBCFG,
        USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_TRDT,
        USB_OTG_GUSBCFG_FDMOD | _VAL2FLD(USB_OTG_GUSBCFG_TRDT, 0x09));

    // Wait for mode change to take effect
    for (volatile int i = 0; i < 200000; i++)
        ;

    // Device speed configuration: Full speed
    // Access device registers through the base pointer
    USB_OTG_DeviceTypeDef *dev = (USB_OTG_DeviceTypeDef *) ((uint32_t) USB_OTG_FS + USB_OTG_DEVICE_BASE);

    MODIFY_REG(dev->DCFG, USB_OTG_DCFG_DSPD, _VAL2FLD(USB_OTG_DCFG_DSPD, 0x03));

    // Make sure device address is 0
    CLEAR_BIT(dev->DCFG, USB_OTG_DCFG_DAD);

    // Power up the PHY
    SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_PWRDWN);

    // Enable VBUS sensing in device mode
    SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_VBUSBSEN);

    // Unmask core interrupts
    SET_BIT(
        USB_OTG_FS->GINTMSK,
        USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_RXFLVLM | USB_OTG_GINTMSK_IEPINT |
            USB_OTG_GINTMSK_OEPINT | USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_WUIM);

    // Unmask transfer completed interrupts for all endpoints
    SET_BIT(dev->DOEPMSK, USB_OTG_DOEPMSK_XFRCM); // OUT endpoints
    SET_BIT(dev->DIEPMSK, USB_OTG_DIEPMSK_XFRCM); // IN endpoints

    // Clear all pending interrupts
    WRITE_REG(USB_OTG_FS->GINTSTS, 0xFFFFFFFF);

    // Enable global interrupt mask
    SET_BIT(USB_OTG_FS->GAHBCFG, USB_OTG_GAHBCFG_GINT);

    log_info("USB Core initialized successfully");

    // Enable USB interrupt in NVIC
    NVIC_SetPriority(OTG_FS_IRQn, 0);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    log_info("NVIC IRQ enabled: %d", NVIC_GetEnableIRQ(OTG_FS_IRQn)); // must print 1
    log_info("NVIC priority: %d", NVIC_GetPriority(OTG_FS_IRQn));     // must print 0

    log_info("GAHBCFG GINT bit: %d", (int) READ_BIT(USB_OTG_FS->GAHBCFG, USB_OTG_GAHBCFG_GINT)); // must be 1
}

void connect()
{
    log_info("Connecting to bus...");

    USB_OTG_DeviceTypeDef *dev = (USB_OTG_DeviceTypeDef *) ((uint32_t) USB_OTG_FS + USB_OTG_DEVICE_BASE);

    // Clear soft disconnect
    CLEAR_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);

    // Give time for pull-up to activate
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;
    // Verify it actually cleared
    log_info("DCTL after connect: 0x%08X", (unsigned int) dev->DCTL);
    log_info("SDIS bit: %d", (int) READ_BIT(dev->DCTL, USB_OTG_DCTL_SDIS)); // must be 0
}

void disconnect()
{
    USB_OTG_DeviceTypeDef *dev = (USB_OTG_DeviceTypeDef *) ((uint32_t) USB_OTG_FS + USB_OTG_DEVICE_BASE);

    // Set soft disconnect
    SET_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);

    // Power down PHY
    CLEAR_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_PWRDWN);
}

static void set_device_address(uint8_t address)
{
    MODIFY_REG(USB_OTG_FS_DEVICE->DCFG, USB_OTG_DCFG_DAD, _VAL2FLD(USB_OTG_DCFG_DAD, address));
}

/** \brief Pops data from the RxFIFO and stores it in the buffer.
 * \param buffer Pointer to the buffer, in which the popped data will be stored.
 * \param size Count of bytes to be popped from the dedicated RxFIFO memory.
 */
static void read_packet(void *buffer, uint16_t size)
{
    // Note: There is only one RxFIFO.
    uint32_t *fifo = FIFO(0);

    for (; size >= 4; size -= 4, buffer += 4)
    {
        // Pops one 32-bit word of data (until there is less than one word remaining).
        uint32_t data = *fifo;
        // Stores the data in the buffer.
        *((uint32_t *) buffer) = data;
    }

    if (size > 0)
    {
        // Pops the last remaining bytes (which are less than one word).
        uint32_t data = *fifo;

        for (; size > 0; size--, buffer++, data >>= 8)
        {
            // Stores the data in the buffer with the correct alignment.
            *((uint8_t *) buffer) = 0xFF & data;
        }
    }
}

/** \brief Pushes a packet into the TxFIFO of an IN endpoint.
 * \param endpoint_number The number of the endpoint, to which the data will be written.
 * \param buffer Pointer to the buffer contains the data to be written to the endpoint.
 * \param size The size of data to be written in bytes.
 */
static void write_packet(uint8_t endpoint_number, void const *buffer, uint16_t size)
{
    __IO uint32_t *fifo                    = FIFO(endpoint_number);
    USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);

    MODIFY_REG(
        in_endpoint->DIEPTSIZ,
        USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
        _VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, size));

    // Enable endpoint and clear NAK — no frame bits for interrupt endpoints
    MODIFY_REG(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_STALL, USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);

    uint16_t word_count = (size + 3) / 4;
    uint16_t remaining  = size;
    for (; word_count > 0; word_count--, buffer += 4)
    {
        uint32_t word  = 0;
        uint16_t bytes = remaining > 4 ? 4 : remaining;
        memcpy(&word, buffer, bytes);
        *fifo = word;
        remaining -= bytes;
    }
}
/*
static void write_packet(uint8_t endpoint_number, void const *buffer, uint16_t size)
{
        uint32_t *fifo = FIFO(endpoint_number);
        USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);

        // Configures the transmission (1 packet that has `size` bytes).
        MODIFY_REG(in_endpoint->DIEPTSIZ,
                USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
                _VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, size)
        );

        // Enables the transmission after clearing both STALL and NAK of the endpoint.
        MODIFY_REG(in_endpoint->DIEPCTL,
                USB_OTG_DIEPCTL_STALL,
                USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA
        );

        // Gets the size in term of 32-bit words (to avoid integer overflow in the loop).
        size = (size + 3) / 4;

        for (; size > 0; size--, buffer += 4)
        {
                // Pushes the data to the TxFIFO.
                *fifo = *((uint32_t *)buffer);
        }
}
*/

static void refresh_fifo_start_addresses()
{
    // The first changeable start address begins after the region of RxFIFO.
    // All TX0FSA / NPTXFSA fields are in 32-bit WORD units (RM0090 §34.14.2-3),
    // so no byte conversion — use the word count directly.
    uint16_t start_address = _FLD2VAL(USB_OTG_GRXFSIZ_RXFD, USB_OTG_FS->GRXFSIZ);

    // Updates the start address of the TxFIFO0.
    MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FSA, _VAL2FLD(USB_OTG_TX0FSA, start_address));

    // The next start address is after where the last TxFIFO ends.
    start_address += _FLD2VAL(USB_OTG_TX0FD, USB_OTG_FS->DIEPTXF0_HNPTXFSIZ);

    // Updates the start addresses of the rest TxFIFOs.
    for (uint8_t txfifo_number = 0; txfifo_number < ENDPOINT_COUNT - 1; txfifo_number++)
    {
        MODIFY_REG(USB_OTG_FS->DIEPTXF[txfifo_number], USB_OTG_NPTXFSA, _VAL2FLD(USB_OTG_NPTXFSA, start_address));
        start_address += _FLD2VAL(USB_OTG_NPTXFD, USB_OTG_FS->DIEPTXF[txfifo_number]);
    }
}

/** \brief Configures the RxFIFO of all OUT endpoints.
 * \param size The size of the largest OUT endpoint in bytes.
 * \note The RxFIFO is shared between all OUT endpoints.
 */
static void configure_rxfifo_size(
    uint16_t size) // only 320 allowed for FS as size!!!! => pass something sensible like size = 256
{
    log_info(">> RX FIFO  size: %d configuring", size);
    // Considers the space required to save status packets in RxFIFO and gets the size in term of 32-bit words.
    size = 10 + (2 * ((size / 4) + 1));
    assert(
        size <=
        (FS_FIFO_TOTAL_WORDS - MIN_TX_FIFO_HEADROOM)); // cases where the RX FIFO is so large it leaves no room for TX
    log_info(">> RX FIFO size set to %d words", size);

    // Configures the depth of the FIFO.
    MODIFY_REG(USB_OTG_FS->GRXFSIZ, USB_OTG_GRXFSIZ_RXFD, _VAL2FLD(USB_OTG_GRXFSIZ_RXFD, size));

    refresh_fifo_start_addresses();
}

static void configure_txfifo_size(uint8_t endpoint_number, uint16_t size)
{
    log_info(">> TX FIFO EP%d siconfiguring", endpoint_number);
    // Gets the FIFO size in term of 32-bit words.
    size = (size + 3) / 4;
    if (size < 4)
        size = 4;
    assert(size >= 4 && size <= FS_FIFO_TOTAL_WORDS);
    log_info(">> TX FIFO EP%d size set to %d words", endpoint_number, size);

    // OTG_FS only has 4 IN endpoints (EP0..EP3)
    assert(endpoint_number <= 3);
    // Sanity check size fits within FS SRAM budget
    assert(size >= MIN_TX_FIFO_WORDS && size <= FS_FIFO_TOTAL_WORDS);
    log_info(">> TX FIFO EP%d size set to %d words", endpoint_number, size);

    // Configures the depth of the TxFIFO.
    if (endpoint_number == 0)
    {
        MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FD, _VAL2FLD(USB_OTG_TX0FD, size));
    }
    else
    {
        MODIFY_REG(USB_OTG_FS->DIEPTXF[endpoint_number - 1], USB_OTG_NPTXFD, _VAL2FLD(USB_OTG_NPTXFD, size));
    }
    refresh_fifo_start_addresses();
}

static void flush_rxfifo()
{
    log_info("flushing rx");
    SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
    while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH))
        ;
}

static void flush_txfifo(uint8_t endpoint_number)
{
    log_info("flushing tx");
    assert(endpoint_number <= 3); // OTG_FS: EP0..EP3 only

    MODIFY_REG(
        USB_OTG_FS->GRSTCTL,
        USB_OTG_GRSTCTL_TXFNUM,
        _VAL2FLD(USB_OTG_GRSTCTL_TXFNUM, endpoint_number) | USB_OTG_GRSTCTL_TXFFLSH);
    while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH))
        ;
}

static void configure_endpoint0(uint8_t endpoint_size) // endpoint_size should be 16 probably??
{
    log_info("configure in endpoint 0.");
    // unmask IN endpoint0 and OUT endpoint0 interrupts
    // Bit 0  = IN  endpoint 0 (INEP0)
    // Bit 16 = OUT endpoint 0 (OUTEP0)
    SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, (1 << 0) | (1 << 16));

    // unmask STUP, XFRC on DOEPMSK and XFRC, TOC on DIEPMSK
    SET_BIT(
        USB_OTG_FS_DEVICE->DOEPMSK,
        USB_OTG_DOEPMSK_STUPM |   // SETUP phase done
            USB_OTG_DOEPMSK_XFRCM // Transfer complete
    );
    SET_BIT(
        USB_OTG_FS_DEVICE->DIEPMSK,
        USB_OTG_DIEPMSK_XFRCM | // Transfer complete
            USB_OTG_DIEPMSK_TOM // Timeout (TOC)
    );

    // Configure IN endpoint 0
    // - USBAEP: mark endpoint as active
    // - MPSIZ: maximum packet size (64 bytes for Full Speed)
    // - SNAK:  respond NAK until we have data ready to send
    MODIFY_REG(
        IN_ENDPOINT(0)->DIEPCTL,
        USB_OTG_DIEPCTL_MPSIZ,
        USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK);

    // Configure OUT endpoint 0
    // - EPENA: enable the endpoint so it can receive data
    // - CNAK:  clear NAK so it actually accepts incoming packets
    SET_BIT(OUT_ENDPOINT(0)->DOEPCTL, USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);

    // Full Speed EP0 max packet size is always 64 bytes
    // FIFO sizes are in 32-bit words: 64 bytes = 16 words -> word conversion happens inside the function!
    configure_rxfifo_size(128);
    configure_txfifo_size(0, endpoint_size); // configure endpoint 0
}

static void configure_in_endpoint(uint8_t endpoint_number, UsbEndpointType endpoint_type, uint16_t endpoint_size)
{
    log_info("configure in endpoint %d.", endpoint_number);
    // Unmask interrupts for this specific IN endpoint in DAINTMSK
    // IN endpoints live in bits [15:0], so just shift by endpoint number
    SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 1 << endpoint_number);

    // Configure the endpoint control register
    MODIFY_REG(
        IN_ENDPOINT(endpoint_number)->DIEPCTL,
        USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP | USB_OTG_DIEPCTL_TXFNUM, // fields to clear first
        USB_OTG_DIEPCTL_USBAEP |                                                // mark endpoint as active
            _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) |                    // max packet size
            USB_OTG_DIEPCTL_SNAK |                              // SNAK — Not Ready Yet -> until we have data
            _VAL2FLD(USB_OTG_DIEPCTL_EPTYP, endpoint_type) |    // bulk/interrupt/isochronous
            _VAL2FLD(USB_OTG_DIEPCTL_TXFNUM, endpoint_number) | // assign TX FIFO number
            USB_OTG_DIEPCTL_SD0PID_SEVNFRM                      // start on DATA0 ~for erros checking?
    );

    /* Typical values fpr packet size:

Bulk: 64 bytes (Full Speed max)
Interrupt: 1–64 bytes
Isochronous: 1–1023 bytes */

    configure_txfifo_size(endpoint_number, endpoint_size);
}

/** \brief Deconfigures IN and OUT endpoints of a specific endpoint number.
 * \param endpoint_number The number of the IN and OUT endpoints to deconfigure.
 */
static void deconfigure_endpoint(uint8_t endpoint_number)
{
    USB_OTG_INEndpointTypeDef *in_endpoint   = IN_ENDPOINT(endpoint_number);
    USB_OTG_OUTEndpointTypeDef *out_endpoint = OUT_ENDPOINT(endpoint_number);

    // Masks all interrupts of the targeted IN and OUT endpoints.
    CLEAR_BIT(USB_OTG_FS_DEVICE->DAINTMSK, (1 << endpoint_number) | (1 << 16 << endpoint_number));

    // Clears all interrupts of the endpoint.
    SET_BIT(in_endpoint->DIEPINT, 0x29FF);
    SET_BIT(out_endpoint->DOEPINT, 0x71FF);

    // Disables the endpoints if possible.
    if (in_endpoint->DIEPCTL & USB_OTG_DIEPCTL_EPENA)
    {
        // Disables endpoint transmission.
        SET_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_EPDIS);
    }

    // Deactivates the endpoint.
    CLEAR_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_USBAEP);

    if (endpoint_number != 0)
    {
        if (out_endpoint->DOEPCTL & USB_OTG_DOEPCTL_EPENA)
        {
            // Disables endpoint transmission.
            SET_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
        }

        // Deactivates the endpoint.
        CLEAR_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_USBAEP);
    }

    // Flushes the FIFOs.
    flush_txfifo(endpoint_number);
    flush_rxfifo();
}

static void usbrst_handler()
{
    log_info("USB reset signal was detected.");

    for (uint8_t i = 0; i < ENDPOINT_COUNT; i++)
    {
        deconfigure_endpoint(i);
    }
    // Clear remote wakeup
    USB_OTG_DeviceTypeDef *dev = (USB_OTG_DeviceTypeDef *) ((uint32_t) USB_OTG_FS + USB_OTG_DEVICE_BASE);
    CLEAR_BIT(dev->DCTL, USB_OTG_DCTL_RWUSIG);
    /* DCTL is the Device Control register. RWUSIG is the Remote Wakeup Signaling bit — if your device was trying to
     * wake the host up, you must stop that signal when a reset occurs. Clearing it is mandatory housekeeping. */

    USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_USBRST;
    /*Writing the bit back to GINTSTS clears it, acknowledging the interrupt. If you don't do this, the ISR fires again
     * forever. */

    usb_events.on_usb_reset_received();
}

static void enumdne_handler()
{
    log_info("USB device speed enumeration done.");
    configure_endpoint0(64);
}

static void rxflvl_handler()
{
    uint32_t receive_status = USB_OTG_FS->GRXSTSP;
    uint8_t endpoint_number = _FLD2VAL(USB_OTG_GRXSTSP_EPNUM, receive_status);
    uint16_t bcnt           = _FLD2VAL(USB_OTG_GRXSTSP_BCNT, receive_status);
    uint16_t pktsts         = _FLD2VAL(USB_OTG_GRXSTSP_PKTSTS, receive_status);

    log_info(">> RXFLVL: EP=%d, bytes=%d, pktsts=0x%02X", endpoint_number, bcnt, pktsts);

    // Sanity check — garbage data means FIFO was read spuriously
    if (endpoint_number > 3 || (pktsts != 0x02 && pktsts != 0x03 && pktsts != 0x04 && pktsts != 0x06))
    {
        log_info(">> RXFLVL: invalid status, ignoring.");
        return;
    }

    switch (pktsts)
    {
        case 0x06: // SETUP packet
            usb_events.on_setup_data_received(endpoint_number, bcnt);
            break;
        case 0x02: // OUT data
            break;
        case 0x04: // SETUP stage complete
            SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
            break;
        case 0x03: // OUT transfer complete
            SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
            break;
    }
}

static void iepint_handler()
{
    uint32_t daint = USB_OTG_FS_DEVICE->DAINT & 0xFFFF;

    while (daint)
    {
        uint8_t endpoint_number = ffs(daint) - 1;
        uint32_t diepint        = IN_ENDPOINT(endpoint_number)->DIEPINT;
        uint32_t diepctl        = IN_ENDPOINT(endpoint_number)->DIEPCTL;
        log_info(
            "iepint: EP%d DIEPINT=0x%08X DIEPCTL=0x%08X",
            endpoint_number,
            (unsigned int) diepint,
            (unsigned int) diepctl);

        if (diepint & USB_OTG_DIEPINT_XFRC)
        {
            usb_events.on_in_transfer_completed(endpoint_number);
            SET_BIT(IN_ENDPOINT(endpoint_number)->DIEPINT, USB_OTG_DIEPINT_XFRC);
        }
        /*
        // Also check for TXFE (FIFO empty) on EP1
        if (endpoint_number != 0 && (diepint & USB_OTG_DIEPINT_TXFE))
        {
            //log_info("EP%d TXFE set — FIFO drained without XFRC", endpoint_number);
        }
        */
        daint &= ~(1 << endpoint_number);
    }
}

static void oepint_handler()
{
    uint32_t daint = USB_OTG_FS_DEVICE->DAINT >> 16;

    while (daint)
    {
        uint8_t endpoint_number = ffs(daint) - 1;

        if (OUT_ENDPOINT(endpoint_number)->DOEPINT & USB_OTG_DOEPINT_XFRC)
        {
            usb_events.on_out_transfer_completed(endpoint_number);
            SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPINT, USB_OTG_DOEPINT_XFRC);
        }
        if (OUT_ENDPOINT(endpoint_number)->DOEPINT & USB_OTG_DOEPINT_STUP)
        {
            SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPINT, USB_OTG_DOEPINT_STUP);
        }

        daint &= ~(1 << endpoint_number);
    }
}

void OTG_FS_IRQHandler(void)
{
    uint32_t gintsts = USB_OTG_FS->GINTSTS & USB_OTG_FS->GINTMSK;

    USB_OTG_DeviceTypeDef *dev = (USB_OTG_DeviceTypeDef *) ((uint32_t) USB_OTG_FS + USB_OTG_DEVICE_BASE);
    /*USB OTG peripheral's registers are split into several blocks in memory.
     *
     USB_OTG_FS points to the global registers, but device-mode registers (like DCTL)
     live at a fixed offset (USB_OTG_DEVICE_BASE) from that base address. This cast gives you a
         typed pointer to that sub-block so you can access device registers cleanly by name.

          */

    //---------------------------------------------------------------------------------------------

    /* The host pulled D+ and D- low for >10ms, signaling a bus reset.
     * This is one of the first things that happens when you plug in,
     * the host resets the device to a known state */
    // USB Reset - HIGHEST PRIORITY
    if (gintsts & USB_OTG_GINTSTS_USBRST)
    {
        usbrst_handler();
    }

    //---------------------------------------------------------------------------------------------

    // RX FIFO Non-Empty - CRITICAL FOR DATA RECEPTION
    if (gintsts & USB_OTG_GINTSTS_RXFLVL)
    {
        log_info("RX FIFO has data!");
        /* The receive FIFO has data in it — this could be a SETUP packet (the start of a control transfer) or OUT data
         * from the host. */

        // TODO: Read from RX FIFO (SETUP packets, OUT data)
        rxflvl_handler();

        // NOTE: RXFLVL is read-only, cleared by reading FIFO
        // Don't try to clear it manually!
    }

    //---------------------------------------------------------------------------------------------

    // IN Endpoint Interrupt
    if (gintsts & USB_OTG_GINTSTS_IEPINT)
    {
        log_info("IN endpoint interrupt!");
        /* An IN endpoint sends data from device to host
         * (e.g. HID reports, CDC serial data).
         * This fires when the hardware has finished transmitting,
         * or needs more data loaded into the TX FIFO. */

        // TODO: Handle IN endpoint transfers
        iepint_handler();

        // Cleared by clearing endpoint-specific interrupt
    }

    //---------------------------------------------------------------------------------------------

    // OUT Endpoint Interrupt
    if (gintsts & USB_OTG_GINTSTS_OEPINT)
    {
        log_info("OUT endpoint interrupt!");
        /* After a reset, the host probes the device's speed capability.
         * When that negotiation is complete, ENUMDNE fires. This tells
         * you the confirmed USB speed (Full Speed in your case) and is your
         * signal to configure endpoints and get ready for real traffic.
         * This is where you'd read DSTS to confirm the negotiated speed and set up EP0. */

        /* An OUT endpoint receives data from host to device.
         * This fires when a transfer completes into an OUT endpoint's buffer. */

        // TODO: Handle OUT endpoint transfers
        oepint_handler();

        // Cleared by clearing endpoint-specific interrupt
    }

    //--------------

    // Add this temporarily
    if (gintsts & USB_OTG_GINTSTS_IEPINT)
    {
        uint32_t daint = USB_OTG_FS_DEVICE->DAINT;
        // log_info("IEPINT fired: DAINT=0x%08X", (unsigned int) daint);
    }
    //---------------------------------------------------------------------------------------------

    // Enumeration done
    if (gintsts & USB_OTG_GINTSTS_ENUMDNE)
    {
        enumdne_handler();
        USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;
    }

    //---------------------------------------------------------------------------------------------

    // USB Suspend
    if (gintsts & USB_OTG_GINTSTS_USBSUSP)
    {
        /*  host stopped sending SOF (Start of Frame) packets for >3ms, meaning it wants the device to enter
         * suspend/low-power mode */
        log_info("USB SUSPEND!");
        USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_USBSUSP;
    }

    //---------------------------------------------------------------------------------------------
    // Wakeup
    if (gintsts & USB_OTG_GINTSTS_WKUINT)
    {
        /* A resume condition was detected on the bus (the host or your device signaled wakeup). Both are cleared the
         * same way — write-1-to-clear in `GINTSTS` */
        log_info("USB WAKEUP!");
        USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_WKUINT;
    }

    usb_events.on_usb_polled();
}

const UsbDriver usb_driver = {
    .initialize_core       = &initialize_core,
    .initialize_gpio_pins  = &initialize_gpio_pins,
    .set_device_address    = &set_device_address,
    .connect               = &connect,
    .disconnect            = &disconnect,
    .flush_rxfifo          = &flush_rxfifo,
    .flush_txfifo          = &flush_txfifo,
    .configure_in_endpoint = &configure_in_endpoint,
    .read_packet           = &read_packet,
    .write_packet          = &write_packet,
    .poll                  = &OTG_FS_IRQHandler};
