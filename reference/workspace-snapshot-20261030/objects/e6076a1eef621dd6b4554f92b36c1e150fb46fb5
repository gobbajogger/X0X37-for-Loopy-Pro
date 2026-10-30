/* SPDX-License-Identifier: GPL-3.0-only
 * Number Of Completed Packets: multi-handle HCI events must credit the
 * associated BLE connection, never another handle's completed count.
 * Native: cc -std=c99 -O1 -g -Wall -Wextra -Werror -Ifirmware/src/ble \
 *   tests/host/ble_hci_completed_packets_test.c \
 *   firmware/src/ble/ble_midi_host.c -o /tmp/ble_hci_completed_packets_test
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)
static int accept_packet(uint8_t kind, const uint8_t *packet, uint16_t size, void *ctx)
{ (void)kind; (void)packet; (void)size; (void)ctx; return 0; }
static void setup(bleh_t *h, uint16_t conn, uint8_t total, uint8_t free_buffers)
{
    bleh_init(h, accept_packet, NULL, "X0X37", NULL);
    h->state = BLEH_CONNECTED;
    h->conn = conn;
    h->acl_total = total;
    h->credits = free_buffers;
}
static void completed(bleh_t *h, const uint8_t *pairs, uint8_t count)
{
    uint8_t event[2 + 1 + 4 * 3] = {0x13, 0};
    CHECK(count <= 3);
    if (count > 3) return;
    event[1] = (uint8_t)(1 + 4 * count);
    event[2] = count;
    if (count) memcpy(event + 3, pairs, (size_t)4 * count);
    bleh_rx(h, BLEH_H4_EVT, event, (uint16_t)(3 + 4 * count));
}
static void test_one_handle(void)
{
    bleh_t h;
    const uint8_t entries[] = {0x40, 0x00, 0x02, 0x00};
    setup(&h, 0x40, 5, 1);
    completed(&h, entries, 1);
    CHECK(h.credits == 3);
}
static void test_two_handles_target_first(void)
{
    bleh_t h;
    /* 0x40 completed 2; unrelated 0x41 completed 3. */
    const uint8_t entries[] = {0x40, 0x00, 0x02, 0x00, 0x41, 0x00, 0x03, 0x00};
    setup(&h, 0x40, 10, 0);
    completed(&h, entries, 2);
    CHECK(h.credits == 2);
}
static void test_two_handles_target_second(void)
{
    bleh_t h;
    const uint8_t entries[] = {0x41, 0x00, 0x01, 0x00, 0x40, 0x00, 0x03, 0x00};
    setup(&h, 0x40, 10, 0);
    completed(&h, entries, 2);
    CHECK(h.credits == 3);
}
static void test_unrelated_count_cannot_spoof_handle(void)
{
    bleh_t h;
    /* No 0x40 handle here. A 0x40 completed count is not a handle. */
    const uint8_t entries[] = {0x41, 0x00, 0x40, 0x00, 0x42, 0x00, 0x01, 0x00};
    setup(&h, 0x40, 8, 0);
    completed(&h, entries, 2);
    CHECK(h.credits == 0);
}
static void test_three_handles_middle(void)
{
    bleh_t h;
    const uint8_t entries[] = {0x41, 0x00, 0x01, 0x00, 0x40, 0x00, 0x02, 0x00, 0x42, 0x00, 0x03, 0x00};
    setup(&h, 0x40, 10, 0);
    completed(&h, entries, 3);
    CHECK(h.credits == 2);
}
static void test_credit_clamped_and_truncated_event(void)
{
    bleh_t h;
    const uint8_t entries[] = {0x40, 0x00, 0xFE, 0xFF};
    const uint8_t truncated[] = {0x13, 9, 2, 0x40, 0, 2, 0, 0x41};
    setup(&h, 0x40, 4, 1);
    completed(&h, entries, 1);
    CHECK(h.credits == 4);
    setup(&h, 0x40, 4, 1);
    bleh_rx(&h, BLEH_H4_EVT, truncated, sizeof truncated);
    CHECK(h.credits == 1);
    completed(&h, NULL, 0);
    CHECK(h.credits == 1);
}
int main(void)
{
    test_one_handle();
    test_two_handles_target_first();
    test_two_handles_target_second();
    test_unrelated_count_cannot_spoof_handle();
    test_three_handles_middle();
    test_credit_clamped_and_truncated_event();
    printf("BLE HCI Number Of Completed Packets: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
