/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the real A/B NOR and production project harness without replaying its
 * unchanged cases. These checks model boot I/O failures, not device boots. */
#define X0X_BLE 1
#define main project_stage_coverage_main
#include "project_stage_test.c"
#undef main

static unsigned radio_changes;
static int radio_on;
void plat_ble_set(int on) { radio_on = !!on; radio_changes++; }
uint32_t plat_ms(void) { return 0; } /* retained DSP helpers under ASan/UBSan */

enum { R_SMC_ROLE = 1, R_SMC_MAP = 2 };
static void smc_edit(int resource, int v)
{
    switch (resource) {
#define say(message, detail) ((void)0)
#include "settings_role_calls.inc"
#undef say
    default: REQUIRE(0); break;
    }
}

static void recovery_expect(int ok, const char *what, int line)
{
    checks++;
    if (!ok) {
        fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, line, test_name, what);
        failures++;
    }
}
#undef CHECK
#define CHECK(x) recovery_expect(!!(x), #x, __LINE__)

static void boot_read_failure(const char *name, uint8_t roles)
{
    fixture(name);
    musical_fixture();
    proj.set.bt = (uint8_t)(roles | PROJ_BT_CLK_IN | (SEQ_CLK_EXT << 1));
    REQUIRE(project_save() == 0);
    memset(&proj, 0, sizeof proj);
    memset(stored_known, 0, sizeof stored_known);
    project_defaults();
    settings_read_failure = -2;
    REQUIRE(project_load() == -2);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == 0);
    radio_on = !!(proj.set.bt & 1u); /* main_fm1 applies the fallback at boot */
    radio_changes = 0;
    snapshot();
}

static void adopt_reset(void)
{
    project_reset_stage();
    REQUIRE(project_validate_stage() == 0);
    project_commit_stage();
    check_no_writes();
    CHECK(proj.set.palette == 1);
    CHECK(PROJ_CLK_SRC(proj.set.bt) == SEQ_CLK_AUTO);
    CHECK(!sm_any(proj.pat[0].drum[1].hit[10]));
}

static void test_recovered_reset(void)
{
    const uint8_t roles = PROJ_DEVICE_BT;
    boot_read_failure("reset after read recovery retains stored device roles", roles);
    settings_read_failure = -1;
    adopt_reset();
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    CHECK(radio_on && radio_changes == 1);
    REQUIRE(project_save() == 0);
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    CHECK(radio_on && radio_changes == 1);
    CHECK(proj.set.palette == 1);
    check_protected_flash();
}

static void test_reset_retry(void)
{
    const uint8_t roles = PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    boot_read_failure("reset during read failure stays saveable after recovery", roles);
    adopt_reset();
    CHECK(project_save() == -8);
    check_no_writes();
    settings_read_failure = -1;
    REQUIRE(project_save() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    CHECK(!radio_on && !radio_changes);
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    CHECK(proj.set.palette == 1);
    CHECK(!sm_any(proj.pat[0].drum[1].hit[10]));
    check_protected_flash();
}

static void test_recovered_load(void)
{
    const uint8_t roles = PROJ_DEVICE_BT;
    boot_read_failure("staged load recovers previously unread device roles", roles);
    settings_read_failure = -1;
    REQUIRE(project_load_stage() == 0);
    CHECK(project_stage.set.palette == 4);
    project_commit_stage();
    check_no_writes();
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    CHECK(proj.set.palette == 4);
    CHECK(PROJ_CLK_SRC(proj.set.bt) == SEQ_CLK_EXT);
    REQUIRE(project_save() == 0);
    check_protected_flash();
}

static void test_import_retry_and_bt_intent(void)
{
    uint8_t header[PROJECT_FILE_HDR];
    const uint8_t stored_roles = PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER;
    boot_read_failure("import retry retains music and pending Bluetooth intent", stored_roles);
    project_device_bt_set(1);
    CHECK(project_device_bt_pending());
    project_capture_stage();
    project_stage.set.palette = 3;
    project_stage.set.bt = PROJ_BT_CLK_IN; /* another keyboard's portable roles */
    sm_set(project_stage.pat[3].drum[0].hit[2], 7);
    project_file_header(header);
    REQUIRE(project_file_validate(header) == 0);
    project_commit_stage();
    check_no_writes();
    CHECK(project_save() == -8);
    check_no_writes();
    settings_read_failure = -1;
    REQUIRE(project_save() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == (stored_roles | 1u));
    CHECK(proj.set.palette == 3);
    CHECK(sm_get(proj.pat[3].drum[0].hit[2], 7));
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == (stored_roles | 1u));
    CHECK(proj.set.palette == 3);
    CHECK(sm_get(proj.pat[3].drum[0].hit[2], 7));
    check_protected_flash();
}

static void test_bt_flush_cannot_lose_role_provenance(void)
{
    const uint8_t roles = PROJ_DEVICE_BT;
    boot_read_failure("Bluetooth-only retry does not erase unread SMC provenance", roles);
    project_device_bt_set(1);
    settings_read_failure = -1;
    REQUIRE(project_device_bt_save() == 0);
    CHECK(!project_device_bt_pending());
    CHECK(project_save() == -8);
    snapshot();
    adopt_reset();
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == roles);
    REQUIRE(project_save() == 0);
    check_protected_flash();
}

static void test_no_adoption_guard(void)
{
    boot_read_failure("recovered reads cannot save unadopted fallback music", PROJ_DEVICE_BT);
    settings_read_failure = -1;
    CHECK(project_save() == -8);
    check_no_writes();
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == PROJ_DEVICE_BT);
    CHECK(proj.set.palette == 4);
    REQUIRE(project_save() == 0);
    check_protected_flash();
}

static void test_local_intent_during_retry(void)
{
    boot_read_failure("local SMC edits and Bluetooth round trip survive recovery", 1u);
    project_device_bt_set(1);
    project_device_bt_set(0);
    CHECK(project_device_bt_pending());
    smc_edit(R_SMC_ROLE, 1);
    smc_edit(R_SMC_MAP, 1);
    adopt_reset();
    CHECK(project_save() == -8);
    check_no_writes();
    settings_read_failure = -1;
    REQUIRE(project_save() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == (PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER));
    REQUIRE(project_device_bt_save() == 0);
    CHECK(!project_device_bt_pending());
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == (PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER));
    check_protected_flash();
}

static void test_explicit_damaged_adoption(void)
{
    boot_read_failure("confirmed damaged settings allow explicit reset adoption", PROJ_DEVICE_BT);
    adopt_reset();
    for (unsigned copy = 0; copy < 2; ++copy)
        memset(nor + st_sector(OBJ_SET, copy), 0, sizeof(st_hdr_t));
    settings_read_failure = -1;
    REQUIRE(project_save() == 0);
    REQUIRE(project_load() == 0);
    CHECK(proj.set.palette == 1);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == 0);
    check_protected_flash();
}

static void test_smc_round_trip(void)
{
    boot_read_failure("explicit SMC role round trips retain local OFF intent", PROJ_DEVICE_BT);
    smc_edit(R_SMC_ROLE, 1);
    smc_edit(R_SMC_ROLE, 0);
    smc_edit(R_SMC_MAP, 1);
    smc_edit(R_SMC_MAP, 0);
    adopt_reset();
    CHECK(project_save() == -8);
    check_no_writes();
    settings_read_failure = -1;
    REQUIRE(project_save() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == 1u);
    CHECK(radio_on && radio_changes == 1);
    REQUIRE(project_load() == 0);
    CHECK((proj.set.bt & PROJ_DEVICE_BT) == 1u);
    check_protected_flash();
}

int main(void)
{
    test_recovered_reset();
    test_reset_retry();
    test_recovered_load();
    test_import_retry_and_bt_intent();
    test_bt_flush_cannot_lose_role_provenance();
    test_no_adoption_guard();
    test_local_intent_during_retry();
    test_explicit_damaged_adoption();
    test_smc_round_trip();
    printf("project settings recovery: %u checks, %u failures (9 cases)\n", checks, failures);
    return failures ? 1 : 0;
}
