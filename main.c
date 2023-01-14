/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Raj Nakarja - Silicon Witchery AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "shared/runtime/pyexec.h"
#include "shared/readline/readline.h"
#include "nrf_sdm.h"
#include "nrf_nvic.h"
#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"
#include "nrfx_spim.h"
#include "ble.h"
#include "modmachine.h"
#include "main.h"

/**
 * @brief Variable that holds the Softdevice NVIC state.
 */
nrf_nvic_state_t nrf_nvic_state = {{0}, 0};

/**
 * @brief This is the ram start pointer as set in the nrf52811.ld file
 */
extern uint32_t _ram_start;

/**
 * @brief To avoid pointer juggling, we dereference _ram_start and store its
 *        address into a uint32_t variable.
 */
static uint32_t ram_start = (uint32_t)&_ram_start;

/**
 * @brief This is the top of stack pointer as set in the nrf52811.ld file
 */
extern uint32_t _stack_top;

/**
 * @brief This is the bottom of stack pointer as set in the nrf52811.ld file
 */
extern uint32_t _stack_bot;

/**
 * @brief This is the start of the heap as set in the nrf52811.ld file
 */
extern uint32_t _heap_start;

/**
 * @brief This is the end of the heap as set in the nrf52811.ld file
 */
extern uint32_t _heap_end;

/**
 * @brief Instance for the SPI connection to the FPGA and Flash IC.
 */
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(0);

/**
 * @brief This struct holds the handles for the conenction and characteristics.
 */
static struct
{
    uint16_t connection;
    uint8_t advertising;
    ble_gatts_char_handles_t rx_characteristic;
    ble_gatts_char_handles_t tx_characteristic;
} ble_handles = {
    .connection = BLE_CONN_HANDLE_INVALID,
    .advertising = BLE_GAP_ADV_SET_HANDLE_NOT_SET,
};

/**
 * @brief Advertising data which needs to stay in scope between connections.
 */
static struct
{
    uint8_t length;
    uint8_t payload[31];
} adv = {
    .length = 0,
    .payload = {0},
};

/**
 * @brief Maximum MTU size that our device will support
 */
#define MAX_MTU_LENGTH 128

/**
 * @brief This is the negotiated MTU length. Not used for anything currently.
 */
static uint16_t negotiated_mtu;

/**
 * @brief Buffer sizes for REPL ring buffers. +45 allows for a bytearray(256) to
 *        be printed in one go.
 */
#define RING_BUFFER_LENGTH (1024 + 45)

/**
 * @brief Ring buffers for the repl rx and tx data which goes over BLE.
 */
static struct
{
    uint8_t buffer[RING_BUFFER_LENGTH];
    uint16_t head;
    uint16_t tail;
} rx = {
    .buffer = "",
    .head = 0,
    .tail = 0,
},
  tx = {
      .buffer = "",
      .head = 0,
      .tail = 0,
};

/**
 * @brief Help text that is shown with the help() command.
 */
const char help_text[] = {
    "Welcome to MicroPython!\n\n"
    "For micropython help, visit: https://docs.micropython.org\n"
    "For hardware help, visit: https://docs.siliconwitchery.com\n\n"
    "Control commands:\n"
    "  Ctrl-A - enter raw REPL mode\n"
    "  Ctrl-B - enter normal REPL mode\n"
    "  CTRL-C - interrupt a running program\n"
    "  Ctrl-D - reset the device\n"
    "  Ctrl-E - enter paste mode\n\n"
    "To list available modules, type help('modules')\n"
    "For details on a specific module, import it, and then type help(module_name)\n"};

/**
 * @brief Assert function.
 * @param err: If err is not 0, the error will be logged, and chip will reset.
 */
void assert_if(uint32_t err)
{
    // Only care about the bottom 16 bits, as the top half is the error type
    if (err & 0x0000FFFF)
    {
        // Trigger a breakpoint when debugging
        if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
        {
            __BKPT();
        }

        // TODO Do we want to log any fault code?

        // Reset the system
        NVIC_SystemReset();
    }
}

/**
 * @brief Softdevice assert handler. Called whenever softdevice crashes.
 */
void softdevice_assert_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    assert_if(id);
}

/**
 * @brief Called if an exception is raised outside all C exception-catching handlers.
 */
void nlr_jump_fail(void *val)
{
    assert_if((uint32_t)val);
    for (;;)
    {
    }
}

/**
 * @brief Sends data to BLE central device.
 * @param str: String to send.
 * @param len: Length of string.
 */
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len)
{
    // For the entire incoming string
    for (uint16_t length = 0;
         length < len;
         length++)
    {
        // Check the next position we want to write at
        uint16_t next = tx.head + 1;

        // Roll back to 0 once we hit the limit
        if (next == RING_BUFFER_LENGTH)
        {
            next = 0;
        }

        // Break if the ring buffer is full, we can't write more
        if (next == tx.tail)
        {
            // TODO Figure out a way to defer writes when the buffer is full
            break;
        }

        // Copy a character into the ring buffer
        tx.buffer[tx.head] = str[length];

        // Update the head to the incremented value
        tx.head = next;
    }
}

/**
 * @brief Sends all buffered data in the tx ring buffer over BLE.
 */
void ble_send_pending_data(void)
{
    // Local buffer for sending data
    uint8_t out_buffer[MAX_MTU_LENGTH] = "";
    uint16_t out_len = 0;

    // If there's no data to send, simply return
    while (tx.head == tx.tail)
    {
        return;
    }

    // For all the remaining characters, i.e until the heads come back together
    while (tx.tail != tx.head)
    {
        // Next is where the tail will point to after the read
        uint16_t next = tx.tail + 1;

        // Roll next back to 0 once we hit the end of the max buffer length
        if (next == RING_BUFFER_LENGTH)
        {
            next = 0;
        }

        // Copy over a character from the tail to the outgoing buffer
        out_buffer[out_len++] = tx.buffer[tx.tail];

        // Apply the increment for next read
        tx.tail = next;

        // Break if we over-run the negotiated MTU size, send the rest later
        if (out_len >= negotiated_mtu)
        {
            break;
        }
    }

    // Initialise the handle value parameters
    ble_gatts_hvx_params_t hvx_params = {0};
    hvx_params.handle = ble_handles.tx_characteristic.value_handle;
    hvx_params.p_data = out_buffer;
    hvx_params.p_len = (uint16_t *)&out_len;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

// TODO Is there a cleaner way to retry sending data?
hvx_try_again:
{
}
    // Send the data
    uint32_t err = sd_ble_gatts_hvx(ble_handles.connection, &hvx_params);

    // Ignore errors if not connected
    if (err == NRF_ERROR_INVALID_STATE || err == BLE_ERROR_INVALID_CONN_HANDLE)
    {
        return;
    }

    // If there is an overflow
    if (err == NRF_ERROR_RESOURCES)
    {
        // Try to send again after 100us
        NRFX_DELAY_US(100);

        // TODO Is there a cleaner way to retry sending data?
        goto hvx_try_again;
    }

    // Catch other errors
    assert_if(err);
}

/**
 * @brief Takes a single character from the received data buffer, and sends it
 *        to the micropython parser.
 */
int mp_hal_stdin_rx_chr(void)
{
    // Wait until data is ready
    while (rx.head == rx.tail)
    {
        // While waiting for incoming data, we can push outgoing data
        ble_send_pending_data();

        // If there's nothing to do
        if (tx.head == tx.tail &&
            rx.head == rx.tail)
        {
            // Wait for events to save power
            sd_app_evt_wait();
        }
    }

    // Next is where the tail will point to after the read
    uint16_t next = rx.tail + 1;

    // Roll next back to 0 once we hit the end of the max buffer length
    if (next == RING_BUFFER_LENGTH)
    {
        next = 0;
    }

    // Read a character from the tail
    int character = rx.buffer[rx.tail];

    // Apply the increment
    rx.tail = next;

    // Return character
    return character;
}

/**
 * @brief Initialises the softdevice and Bluetooth functionality.
 */
static void ble_init(void)
{
    // Error code variable
    uint32_t err;

    // Init LF clock
    nrf_clock_lf_cfg_t clock_config = {
        .source = NRF_CLOCK_LF_SRC_XTAL,
        .rc_ctiv = 0,
        .rc_temp_ctiv = 0,
        .accuracy = NRF_CLOCK_LF_ACCURACY_20_PPM};

    // Enable the softdevice
    err = sd_softdevice_enable(&clock_config, softdevice_assert_handler);
    assert_if(err);

    // Enable softdevice interrupt
    err = sd_nvic_EnableIRQ((IRQn_Type)SD_EVT_IRQn);
    assert_if(err);

    // Enable the DC-DC convertor
    err = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    assert_if(err);

    // Add GAP configuration to the BLE stack
    ble_cfg_t ble_conf;
    ble_conf.conn_cfg.conn_cfg_tag = 1;
    ble_conf.conn_cfg.params.gap_conn_cfg.conn_count = 1;
    ble_conf.conn_cfg.params.gap_conn_cfg.event_length = 3;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_conf, ram_start);
    assert_if(err);

    // Set BLE role to peripheral only
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.gap_cfg.role_count_cfg.periph_role_count = 1;
    err = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_conf, ram_start);
    assert_if(err);

    // Set max MTU size
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.conn_cfg.conn_cfg_tag = 1;
    ble_conf.conn_cfg.params.gatt_conn_cfg.att_mtu = MAX_MTU_LENGTH;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_conf, ram_start);
    assert_if(err);

    // Configure a single queued transfer
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.conn_cfg.conn_cfg_tag = 1;
    ble_conf.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 1;
    err = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_conf, ram_start);
    assert_if(err);

    // Configure number of custom UUIDs
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.common_cfg.vs_uuid_cfg.vs_uuid_count = 1;
    err = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_conf, ram_start);
    assert_if(err);

    // Configure GATTS attribute table
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.gatts_cfg.attr_tab_size.attr_tab_size = 1408;
    err = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_conf, ram_start);
    assert_if(err);

    // No service changed attribute needed
    memset(&ble_conf, 0, sizeof(ble_conf));
    ble_conf.gatts_cfg.service_changed.service_changed = 0;
    err = sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &ble_conf, ram_start);
    assert_if(err);

    // Start bluetooth. The ram_start returned is the total required RAM for SD
    err = sd_ble_enable(&ram_start);
    assert_if(err);

    // Set security to open
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // Set device name. Last four characters are taken from MAC address
    char device_name[] = "S1-XXXX";

    uint8_t str_pos = 6, glyph;
    uint16_t quotent = (uint16_t)NRF_FICR->DEVICEADDR[0];
    while (quotent != 0)
    {
        glyph = quotent % 16;
        glyph < 10 ? (glyph += 48) : (glyph += 55);
        device_name[str_pos--] = glyph;
        quotent /= 16;
    }

    err = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)device_name,
                                     (uint16_t)strlen((const char *)device_name));
    assert_if(err);

    // Set connection parameters
    ble_gap_conn_params_t gap_conn_params = {0};
    gap_conn_params.min_conn_interval = (15 * 1000) / 1250;
    gap_conn_params.max_conn_interval = (15 * 1000) / 1250;
    gap_conn_params.slave_latency = 3;
    gap_conn_params.conn_sup_timeout = (2000 * 1000) / 10000;
    err = sd_ble_gap_ppcp_set(&gap_conn_params);
    assert_if(err);

    // Add the Nordic UART service long UUID
    ble_uuid128_t uuid128 = {.uuid128 = {0x9E, 0xCA, 0xDC, 0x24,
                                         0x0E, 0xE5, 0xA9, 0xE0,
                                         0x93, 0xF3, 0xA3, 0xB5,
                                         0x00, 0x00, 0x40, 0x6E}};

    // Set the 16 bit UUIDs for the service and characteristics
    ble_uuid_t service_uuid = {.uuid = 0x0001};
    ble_uuid_t rx_uuid = {.uuid = 0x0002};
    ble_uuid_t tx_uuid = {.uuid = 0x0003};

    // Temporary NUS handle
    uint16_t nordic_uart_service_handle;

    err = sd_ble_uuid_vs_add(&uuid128, &service_uuid.type);
    assert_if(err);

    err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                   &service_uuid, &nordic_uart_service_handle);

    // Copy the service UUID type to both rx and tx UUID
    rx_uuid.type = service_uuid.type;
    tx_uuid.type = service_uuid.type;

    // Add rx characterisic
    ble_gatts_char_md_t rx_char_md = {0};
    rx_char_md.char_props.write = 1;
    rx_char_md.char_props.write_wo_resp = 1;

    ble_gatts_attr_md_t rx_attr_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rx_attr_md.write_perm);
    rx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
    rx_attr_md.vlen = 1;

    ble_gatts_attr_t rx_attr = {0};
    rx_attr.p_uuid = &rx_uuid;
    rx_attr.p_attr_md = &rx_attr_md;
    rx_attr.init_len = sizeof(uint8_t);
    rx_attr.max_len = MAX_MTU_LENGTH - 3;

    err = sd_ble_gatts_characteristic_add(nordic_uart_service_handle,
                                          &rx_char_md,
                                          &rx_attr,
                                          &ble_handles.rx_characteristic);
    assert_if(err);

    // Add tx characterisic
    ble_gatts_char_md_t tx_char_md = {0};
    tx_char_md.char_props.notify = 1;

    ble_gatts_attr_md_t tx_attr_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&tx_attr_md.write_perm);
    tx_attr_md.vloc = BLE_GATTS_VLOC_STACK;
    tx_attr_md.vlen = 1;

    ble_gatts_attr_t tx_attr = {0};
    tx_attr.p_uuid = &tx_uuid;
    tx_attr.p_attr_md = &tx_attr_md;
    tx_attr.init_len = sizeof(uint8_t);
    tx_attr.max_len = MAX_MTU_LENGTH - 3;

    err = sd_ble_gatts_characteristic_add(nordic_uart_service_handle,
                                          &tx_char_md,
                                          &tx_attr,
                                          &ble_handles.tx_characteristic);
    assert_if(err);

    // Add name to advertising payload
    adv.payload[adv.length++] = strlen((const char *)device_name) + 1;
    adv.payload[adv.length++] = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&adv.payload[adv.length],
           device_name,
           sizeof(device_name));
    adv.length += strlen((const char *)device_name);

    // Set discovery mode flag
    adv.payload[adv.length++] = 0x02;
    adv.payload[adv.length++] = BLE_GAP_AD_TYPE_FLAGS;
    adv.payload[adv.length++] = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    // Add Nordic UART service to advertising data
    uint8_t encoded_uuid_length;
    err = sd_ble_uuid_encode(&service_uuid,
                             &encoded_uuid_length,
                             &adv.payload[adv.length + 2]);
    assert_if(err);

    adv.payload[adv.length++] = 0x01 + encoded_uuid_length;
    adv.payload[adv.length++] = BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE;
    adv.length += encoded_uuid_length;

    ble_gap_adv_data_t adv_data = {
        .adv_data.p_data = adv.payload,
        .adv_data.len = adv.length,
        .scan_rsp_data.p_data = NULL,
        .scan_rsp_data.len = 0};

    // Set up advertising parameters
    ble_gap_adv_params_t adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.primary_phy = BLE_GAP_PHY_AUTO;
    adv_params.secondary_phy = BLE_GAP_PHY_AUTO;
    adv_params.interval = (20 * 1000) / 625;

    // Configure the advertising set
    err = sd_ble_gap_adv_set_configure(&ble_handles.advertising, &adv_data, &adv_params);
    assert_if(err);

    // Start advertising
    err = sd_ble_gap_adv_start(ble_handles.advertising, 1);
    assert_if(err);
}

/**
 * @brief Initialises the hardware level drivers and IO.
 */
static void hardware_init(void)
{
    // Initialise the ADC driver, used by both the ADC and PMIC modules
    nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);

    // Initialise the GPIO driver used by both the Pin and FPGA modules
    nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);

    // RTC is initialised in RTC module
    machine_rtc_init();

    // I2C driver is initialised inside the PMIC module
    machine_pmic_init();

    // Initialise the GPIO needed for the FPGA
    machine_fpga_init();
}

/**
 * @brief Function for communicating with the Flash and FPGA.
 * @param tx_buffer: Pointer to the transmit data buffer
 * @param tx_len: How many bytes to transmit
 * @param rx_buffer: Pointer to the received data buffer
 * @param rx_len: How many bytes to receive
 * @param device: Which device to communicate with
 */
void spim_tx_rx(uint8_t *tx_buffer, size_t tx_len,
                uint8_t *rx_buffer, size_t rx_len, spi_device_t device)
{
    // Use a default SPI configuration and set the pins
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG(15, 11, 8, 12);

    // If the FPGA is selected, we use an inverted chip select
    if (device == FPGA)
    {
        spi_config.ss_active_high = true;
    }

    // Initialise the SPI if it was not already
    nrfx_spim_init(&spi, &spi_config, NULL, NULL);

    // Configure the transfer descriptor
    nrfx_spim_xfer_desc_t spi_xfer = NRFX_SPIM_XFER_TRX(tx_buffer, tx_len,
                                                        rx_buffer, rx_len);

    // Initiate the transfer
    nrfx_err_t err = nrfx_spim_xfer(&spi, &spi_xfer, 0);
    assert_if(err);
}

/**
 * @brief Main application called from Reset_Handler().
 */
int main()
{
    // Initialise BLE
    ble_init();

    // Configure the hardware and IO pins
    hardware_init();

    // Initialise the stack pointer for the main thread
    mp_stack_set_top(&_stack_top);

    // Set the stack limit as smaller than the real stack so we can recover
    mp_stack_set_limit((char *)&_stack_top - (char *)&_stack_bot - 400);

    // Initialise the garbage collector
    gc_init(&_heap_start, &_heap_end); // TODO optimize away GC if space needed later

    // Initialise the micropython runtime
    mp_init();

    // Initialise the readline module for REPL
    readline_init0();

    // REPL mode can change, or it can request a soft reset
    for (;;)
    {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
            if (pyexec_raw_repl() != 0)
            {
                break;
            }
        }
        else
        {
            if (pyexec_friendly_repl() != 0)
            {
                break;
            }
        }
    }

    // Garbage collection ready to exit
    gc_sweep_all(); // TODO optimize away GC if space needed later

    // Deinitialize the runtime.
    mp_deinit();

    // Stop the softdevice
    sd_softdevice_disable();

    // Reset chip
    NVIC_SystemReset();
}

/**
 * @brief BLE event handler.
 */
void SWI2_IRQHandler(void)
{
    uint32_t evt_id;
    uint8_t ble_evt_buffer[sizeof(ble_evt_t) + MAX_MTU_LENGTH];

    // While any softdevice events are pending, handle flash operations
    while (sd_evt_get(&evt_id) != NRF_ERROR_NOT_FOUND)
    {
        switch (evt_id)
        {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            // TODO In case we add a filesystem in the future
            break;

        case NRF_EVT_FLASH_OPERATION_ERROR:
            // TODO In case we add a filesystem in the future
            break;

        default:
            break;
        }
    }

    // While any BLE events are pending
    while (1)
    {
        // Pull an event from the queue
        uint16_t buffer_len = sizeof(ble_evt_buffer);
        uint32_t status = sd_ble_evt_get(ble_evt_buffer, &buffer_len);

        // If we get the done status, we can exit the handler
        if (status == NRF_ERROR_NOT_FOUND)
        {
            break;
        }

        // Check for other errors
        assert_if(status);

        // Error flag for use later
        uint32_t err;

        // Make a pointer from the buffer which we can use to find the event
        ble_evt_t *ble_evt = (ble_evt_t *)ble_evt_buffer;

        // Otherwise on NRF_SUCCESS, we handle the new event
        switch (ble_evt->header.evt_id)
        {

        // When connected
        case BLE_GAP_EVT_CONNECTED:
        {
            // Set the connection handle
            ble_handles.connection = ble_evt->evt.gap_evt.conn_handle;

            // Update connection parameters
            ble_gap_conn_params_t conn_params;

            err = sd_ble_gap_ppcp_get(&conn_params);
            assert_if(err);

            err = sd_ble_gap_conn_param_update(ble_evt->evt.gap_evt.conn_handle,
                                               &conn_params);
            assert_if(err);

            break;
        }

        // When disconnected
        case BLE_GAP_EVT_DISCONNECTED:
        {
            // Clear the connection handle
            ble_handles.connection = BLE_CONN_HANDLE_INVALID;

            // Start advertising
            err = sd_ble_gap_adv_start(ble_handles.advertising, 1);
            assert_if(err);

            break;
        }

        // On a phy update request, set the phy speed automatically
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys = {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };

            err = sd_ble_gap_phy_update(ble_evt->evt.gap_evt.conn_handle, &phys);
            assert_if(err);

            break;
        }

        // Handle requests for changing MTU length
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
        {
            // The client's desired MTU size
            uint16_t client_mtu =
                ble_evt->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu;

            // Respond with our max MTU size
            err = sd_ble_gatts_exchange_mtu_reply(
                ble_evt->evt.gatts_evt.conn_handle,
                MAX_MTU_LENGTH);
            assert_if(err);

            // Choose the smaller MTU as the final length we'll use
            // -3 bytes to accommodate for Op-code and attribute handle
            negotiated_mtu = MAX_MTU_LENGTH < client_mtu
                                 ? MAX_MTU_LENGTH - 3
                                 : client_mtu - 3;

            break;
        }

        // When data arrives, we can write it to the buffer
        case BLE_GATTS_EVT_WRITE:
        {
            // For the entire incoming string
            for (uint16_t length = 0;
                 length < ble_evt->evt.gatts_evt.params.write.len;
                 length++)
            {
                // Check the next position we want to write at
                uint16_t next = rx.head + 1;

                // Roll back to 0 once we hit the limit
                if (next == RING_BUFFER_LENGTH)
                {
                    next = 0;
                }

                // Break if the ring buffer is full, we can't write more
                if (next == rx.tail)
                {
                    break;
                }

                // Copy a character into the ring buffer
                rx.buffer[rx.head] =
                    ble_evt->evt.gatts_evt.params.write.data[length];

                // Update the head to the incremented value
                rx.head = next;
            }

            break;
        }

        // Disconnect on GATT Client timeout
        case BLE_GATTC_EVT_TIMEOUT:
        {
            // TODO Remove this if it doesn't ever happen
            assert(1);

            err = sd_ble_gap_disconnect(ble_evt->evt.gattc_evt.conn_handle,
                                        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            assert_if(err);

            break;
        }

        // Disconnect on GATT Server timeout
        case BLE_GATTS_EVT_TIMEOUT:
        {
            err = sd_ble_gap_disconnect(ble_evt->evt.gatts_evt.conn_handle,
                                        BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            assert_if(err);

            break;
        }

        // Updates system attributes after a new connection event
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        {
            err = sd_ble_gatts_sys_attr_set(ble_evt->evt.gatts_evt.conn_handle,
                                            NULL, 0, 0);
            assert_if(err);

            break;
        }

        // We don't support pairing, so reply with that message
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        {
            err = sd_ble_gap_sec_params_reply(ble_evt->evt.gatts_evt.conn_handle,
                                              BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                              NULL, NULL);
            assert_if(err);

            break;
        }

        // Ignore unused events
        default:
            break;
        }
    }
}

/**
 * @brief Garbage collection route for nRF.
 */
void gc_collect(void)
{
    // start the GC
    gc_collect_start();

    // Get stack pointer
    uintptr_t sp;
    __asm__("mov %0, sp\n"
            : "=r"(sp));

    // Trace the stack, including the registers
    // (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)&_stack_top - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}