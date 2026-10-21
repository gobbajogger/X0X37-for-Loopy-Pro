/* SPDX-License-Identifier: MPL-2.0
 * Combined application profile + mailbox test. No SDK, HCI producer, or radio. */
#define main original_subscription_contract_main
#define ctl_vendor_address profile_stub_address
#define ctl_vendor_link_copy profile_stub_link_copy
#define ctl_vendor_notification_copy profile_stub_notification_copy
#include "ctl_vendor_subscription_test.c"
#undef main
#undef ctl_vendor_address
#undef ctl_vendor_link_copy
#undef ctl_vendor_notification_copy
#include "x0x_ble_abi.h"
#include "x0x_smc_host_messages.h"
static xb_mbox_t box;
static const xb_boot_t *ctl_bp;
static xb_mbox_t *ctl_mb=&box;
static u32 ctl_port_irq_save(void) { return 0; }
static void ctl_port_irq_restore(u32 f) { (void)f; }
#include "../../ble/ctl/src/ctl_vendor_bridge.c"
int main(void) {
    xsm_boot_t boot={0}; boot.base.abi=XSM_ABI;
    boot.ext_magic=XSM_BOOT_EXT_MAGIC; boot.ext_size=sizeof boot;
    boot.radio_desired=1; ctl_bp=&boot.base;
    reset(); discover(); finish(); ctl_vendor_mailbox_reset(); ctl_vendor_mailbox_poll();
    CHECK(sends==1 && radio_seen==1);
    u8 auth=5; done(9,0,0,1,&auth);
    u8 e[6]={8,4,0,0,0,1}; packet_event(4,0,e,6); CHECK(subscription==SUB_RETRY);
    u8 stale_on=1, midi[3]={0xb0,7,127};
    CHECK(xb_frame_put(&box.h2c,XSM_MIDI_OUT,midi,3)==0);
    CHECK(xb_frame_put(&box.h2c,XSM_RADIO,&stale_on,1)==0);
    boot.radio_desired=0; ctl_vendor_mailbox_poll();
    CHECK(sends==1 && !wanted && retiring && radio_seen==0 && !have_pending);
    ctl_vendor_mailbox_poll(); CHECK(sends==1 && !wanted && radio_seen==0);
    if(failures) return 1;
    puts("combined application mailbox/profile OFF ordering PASS: authoritative ABI4 OFF precedes queued security retry, MIDI, and stale ON");
    return 0;
}
