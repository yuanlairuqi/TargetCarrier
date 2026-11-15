#include "armor_can.h"
#include "armor_wire.h"
#include "tc_board.h"
#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdatomic.h>

enum {
    ARMOR_CAN_TX = GPIO_NUM_6,
    ARMOR_CAN_RX = GPIO_NUM_7,
    /* Touch_led stock: HSI/2 x16, APB1=/2, BRP=4, 18 TQ.
     * 32 MHz / (4 * 18) ~= 444444 bit/s. Keep that board unchanged. */
    ARMOR_CAN_BITRATE = 444444,
    ARMOR_COMMAND_ID = 0x200,
    ARMOR_STATUS_ID = 0x100,
    ARMOR_REPORT_MS = 450
};

_Static_assert(ARMOR_CAN_TX != TC_ESP_SDA && ARMOR_CAN_TX != TC_ESP_SCL &&
               ARMOR_CAN_TX != TC_ESP_READY && ARMOR_CAN_RX != TC_ESP_SDA &&
               ARMOR_CAN_RX != TC_ESP_SCL && ARMOR_CAN_RX != TC_ESP_READY,
               "Lamp CAN pins conflict with the target I2C link");

/* The bridge worker services the lamp bus under the bridge state lock.
 * The ISR only queues board observations. */
static twai_node_handle_t node;
static QueueHandle_t observations;
static uint8_t observed = ARMOR_UNKNOWN;
static uint32_t pending_ticket, requested_at;
static uint8_t requested;
static bool ready, recovering;
static uint8_t tx_color;
enum { TX_PENDING, TX_ACKNOWLEDGED, TX_FAILED };
static atomic_int tx_result;

static bool tx_done_isr(twai_node_handle_t handle, const twai_tx_done_event_data_t *event,
                        void *ctx)
{
    (void)handle;
    (void)ctx;
    atomic_store(&tx_result, event->is_tx_success ? TX_ACKNOWLEDGED : TX_FAILED);
    return false;
}
static twai_frame_t tx = {
    .header = {.id = ARMOR_COMMAND_ID, .dlc = 1},
    .buffer = &tx_color,
    .buffer_len = 1
};

static bool receive_isr(twai_node_handle_t handle, const twai_rx_done_event_data_t *event,
                        void *ctx)
{
    (void)event;
    (void)ctx;
    uint8_t data[8];
    twai_frame_t frame = {.buffer = data, .buffer_len = sizeof(data)};
    if (twai_node_receive_from_isr(handle, &frame) != ESP_OK ||
        frame.header.id != ARMOR_STATUS_ID || frame.header.ide ||
        frame.header.rtr || frame.header.fdf || frame.header.dlc != 5 ||
        (data[0] > 2 && data[0] != ARMOR_UNKNOWN)) return false;
    BaseType_t wake = pdFALSE;
    xQueueSendFromISR(observations, &data[0], &wake);
    return wake == pdTRUE;
}

void armor_can_init(void)
{
    const twai_onchip_node_config_t config = {
        .io_cfg = {.tx = ARMOR_CAN_TX, .rx = ARMOR_CAN_RX,
                   .quanta_clk_out = -1, .bus_off_indicator = -1},
        .clk_src = TWAI_CLK_SRC_APB,
        /* Allocate using a supported automatic timing first. The auto solver
         * picks an odd BRP for 444444 on S3. Apply the exact timing before enable. */
        .bit_timing = {.bitrate = 500000},
        .tx_queue_depth = 1,
        .fail_retry_cnt = 0,
        .flags.no_receive_rtr = true
    };
    const twai_mask_filter_config_t filter = {.id = ARMOR_STATUS_ID, .mask = TWAI_STD_ID_MASK};
    /* 80 MHz / (10 * 18), sample point 16/18. Match the bxCAN timing. */
    const twai_timing_advanced_config_t timing = {
        .brp = 10, .tseg_1 = 15, .tseg_2 = 2, .sjw = 2
    };
    const twai_event_callbacks_t callbacks = {.on_rx_done = receive_isr, .on_tx_done = tx_done_isr};
    observations = xQueueCreate(8, sizeof(uint8_t));
    esp_err_t err = observations ? twai_new_node_onchip(&config, &node) : ESP_ERR_NO_MEM;
    if (err == ESP_OK) err = twai_node_reconfig_timing(node, &timing, NULL);
    if (err == ESP_OK) err = twai_node_config_mask_filter(node, 0, &filter);
    if (err == ESP_OK) err = twai_node_register_event_callbacks(node, &callbacks, NULL);
    if (err == ESP_OK) err = twai_node_enable(node);
    if (err == ESP_OK) {
        ready = true;
        ESP_LOGI("armor_can", "Lamp CAN ready: G6 TX, G7 RX, %u bit/s", ARMOR_CAN_BITRATE);
        return;
    }
    if (node) (void)twai_node_delete(node);
    node = NULL;
    if (observations) vQueueDelete(observations);
    ESP_LOGE("armor_can", "Lamp CAN unavailable: %s", esp_err_to_name(err));
}

bool armor_can_ready(void) { return ready; }
uint8_t armor_can_color(void) { return observed; }

void armor_can_cancel(void)
{
    pending_ticket = 0;
}

static bool finish(armor_result_t *reply, uint8_t result, uint32_t ticket)
{
    *reply = (armor_result_t){.ticket = ticket, .result = result, .color = observed};
    return true;
}

bool armor_can_request(uint8_t color, uint32_t ticket, uint32_t now, armor_result_t *reply)
{
    if (!ready) return finish(reply, ARMOR_CAN_ERROR, ticket);
    if (pending_ticket || twai_node_transmit_wait_all_done(node, 0) != ESP_OK)
        return finish(reply, ARMOR_BUSY, ticket);

    /* Discard observations already waiting before this transmission. They
     * can update the last-known state, but cannot confirm this request. */
    for (unsigned i = 0; i < 8; ++i) {
        uint8_t previous;
        if (xQueueReceive(observations, &previous, 0) != pdTRUE) break;
        observed = previous;
    }
    tx_color = color;
    atomic_store(&tx_result, TX_PENDING);
    if (twai_node_transmit(node, &tx, 0) != ESP_OK)
        return finish(reply, ARMOR_CAN_ERROR, ticket);
    requested = color;
    requested_at = now;
    pending_ticket = ticket;
    ESP_LOGI("armor_can", "Queued CAN 0x200 color=%u ticket=%lu", color, (unsigned long)ticket);
    return false;
}

bool armor_can_poll(uint32_t now, armor_result_t *reply)
{
    if (!node) return false;
    twai_node_status_t status;
    if (twai_node_get_info(node, &status, NULL) == ESP_OK) {
        ready = status.state != TWAI_ERROR_BUS_OFF;
        if (ready) recovering = false;
        else {
            observed = ARMOR_UNKNOWN;
            if (!recovering) recovering = twai_node_recover(node) == ESP_OK;
            if (pending_ticket) {
                uint32_t ticket = pending_ticket;
                pending_ticket = 0;
                return finish(reply, ARMOR_CAN_ERROR, ticket);
            }
            return false;
        }
    }
    if (pending_ticket && atomic_load(&tx_result) == TX_FAILED) {
        uint32_t ticket = pending_ticket;
        pending_ticket = 0;
        ESP_LOGW("armor_can", "CAN 0x200 transmission failed ticket=%lu", (unsigned long)ticket);
        return finish(reply, ARMOR_CAN_ERROR, ticket);
    }
    if (pending_ticket && now - requested_at >= ARMOR_REPORT_MS) {
        uint32_t ticket = pending_ticket;
        pending_ticket = 0;
        ESP_LOGW("armor_can", "No matching CAN 0x100 ticket=%lu tx_result=%d",
                 (unsigned long)ticket, atomic_load(&tx_result));
        return finish(reply, ARMOR_TIMEOUT, ticket);
    }

    bool have_report = false;
    for (unsigned i = 0; i < 8; ++i) {
        uint8_t color;
        if (xQueueReceive(observations, &color, 0) != pdTRUE) break;
        if (observed != color || (pending_ticket && color == requested))
            ESP_LOGI("armor_can", "Received CAN 0x100 color=%u", color);
        observed = color;
        have_report = true;
        if (pending_ticket && color == requested) {
            uint32_t ticket = pending_ticket;
            pending_ticket = 0;
            return finish(reply, ARMOR_CONFIRMED, ticket);
        }
    }
    return have_report && finish(reply, ARMOR_OBSERVED, 0);
}
