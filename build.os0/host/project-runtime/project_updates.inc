        {   /* Project transfers cannot queue an update across COMMIT or ABORT. */
            int held = ui_project_busy();
            ota_pending = ed_service();             /* classify the shared pending frame once */
            if (held || ui_project_busy()) {
                usb.ota_req = 0;
                usb.uboot_req = 0;
                if (ed_install_pending) ed_install_forget();
            }
        }
        if (ed_install_pending) {
            /* A validated COMMIT owns this iteration. Drop legacy IRQ requests
             * rather than inheriting one or running a reboot before admission. */
            usb.ota_req = usb.uboot_req = 0;
            ed_reboot_at = 0;
            if (ed_install_pending != 1u) ed_install_pending = 0;
        }
        if (!ui_project_busy() && ed_reboot_at && fm1_ms - ed_reboot_at > 200u) {
            ed_install_forget();
            bootguard.pending = 0;                  /* intentional reset: not a failed boot */
            usb_detach();                           /* ED_BT_TEST restart: the reply had 200 ms */
            fm1_delay_ms(30);
            fm1_reboot();
        }
        if (ota_pending && !ui_project_busy())
            ota_service();                          /* M-UPGRADE uses the same main-loop scratch as project staging */
        if (!ui_project_busy() && (usb.ota_req || ed_install_pending)) {
            int guarded = ed_install_pending;
            ed_install_forget();                     /* consume before OTA; no editor/UI call may intervene */
            usb.ota_req = 0;
            seq_stop(&seq);
            if (flash_ok)
                ota_session();                      /* returns only if nothing was committed */
            ed_install_forget();
            if (guarded) {
                usb.ota_req = usb.uboot_req = 0;       /* no request received in the session may replay */
                ed_reboot_at = 0;
            }
            lcd_fill(0, 0, 240, 240, C_BLACK);
            ui_init();
        }
        if (!ui_project_busy() && usb.uboot_req) {
            ed_install_forget();
            enter_uboot("UBOOT (USB)");
        }
