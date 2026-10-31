#!/usr/bin/env python3
"""Hardware-free scalar/address regressions for the native pi32v2 ROM guard."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import rom_guard as guard


def assembly(*rows):
    """Labels are local test branch targets; rows retain objdump-like syntax."""
    labels, offset = {}, 0x2000100
    for row in rows:
        if row.endswith(":"):
            labels[row[:-1]] = offset
        elif row not in ("}", "} else {"):
            offset += 4
    output, offset = ["test_function:"], 0x2000100
    for row in rows:
        if row.endswith(":"):
            continue
        for label, address in labels.items():
            row = row.replace("@" + label, f"4 <test_function : {address:x} >")
        if row in ("}", "} else {"):
            output.append("\t" + row)
        else:
            output.append(f" {offset:x}: 00 00 \t{row}")
            offset += 4
    return "\n".join(output) + "\n"


class RomGuardTests(unittest.TestCase):
    def accepted(self, *rows):
        self.assertEqual(guard.references(assembly(*rows)), [])

    def rejected(self, *rows):
        self.assertTrue(guard.references(assembly(*rows)))

    def test_scalar_compare_both_paths_kill_constant(self):
        self.accepted("r0 = 4290772993", "ifs (r14 >= r0) goto @alternate",
                      "r0 = 0", "goto @done", "alternate:", "r0 = 1", "done:", "rts")

    def test_gain_arithmetic_narrows_before_export(self):
        self.accepted("r1 = 32767", "r3 = r2 * r1", "r2 = 4291690596",
                      "r2 += r3", "r3 = 27", "r2 = r2 / r3 (u)",
                      "r1 += r2", "[r0+40] = r1", "r0 = 1", "rts")

    def test_reaching_constant_through_conditional_path(self):
        self.accepted("r1 = 32767", "if (r4 > 0) goto @start", "r1 = 8", "rts",
                      "start:", "r2 = 4291690596", "r2 += r3", "r3 = 27",
                      "r2 = r2 / r3 (u)", "r1 += r2", "[r0+40] = r1", "r0 = 1", "rts")

    def test_no_literal_or_function_whitelist(self):
        for value in (guard.ROM_LO, guard.ROM_LO + 184, guard.ROM_HI - 1):
            with self.subTest(value=value):
                self.accepted(f"r2 = {value}", "r3 = 31", "r2 = r2 / r3 (u)",
                              "[r0+4] = r2", "rts")
                self.rejected(f"r2 = {value}", "r0 = [r2+0]", "rts")

    def test_masked_scalar_export(self):
        self.accepted("r2 = 0xffc01000", "r2 &= 127", "[r0+4] = r2", "rts")

    def test_direct_rom_call_and_branch(self):
        self.rejected("call -4 <rom : ffc00020 >", "rts")
        self.rejected("goto -4 <rom : ffc00020 >", "rts")

    def test_direct_absolute_rom_load_and_store(self):
        self.rejected("r0 = [0xffc00100]", "rts")
        self.rejected("[4290772992] = r1", "rts")

    def test_pc_relative_memory_annotation_is_a_real_address(self):
        self.rejected("r0 = [pc+4] <rom : ffc00020 >", "rts")
        self.rejected("[pc+4] = r0 <rom : ffc00020 >", "rts")

    def test_original_decimal_negative_and_hex_addresses(self):
        for value in ("4290772993", "-4194303", "0xffc00001"):
            with self.subTest(value=value):
                self.rejected(f"r2 = {value}", "r0 = [r2+0]", "rts")

    def test_indirect_call_and_register_alias(self):
        self.rejected("r2 = 4290772993", "call r2", "rts")
        self.rejected("r2 = 4290772993", "r5 = r2", "r2 = 0", "call r5", "rts")
        self.rejected("r2 = 4290772993", "r5 = r2", "r2 = 0", "r0 = [r5+4]", "rts")

    def test_pointer_spill_and_return_are_rejected(self):
        self.rejected("r2 = 4290772993", "[sp+4] = r2", "r2 = 0", "rts")
        self.rejected("r0 = 4290772993", "rts")
        self.rejected("r2 = 4290772993", "r1 = r2", "r2 = 0", "rts")

    def test_even_bounded_tainted_address_use_is_rejected(self):
        self.rejected("r2 = 4290772993", "r2 &= 127", "r0 = [r2+4]", "rts")
        self.rejected("r2 = 4290772993", "r2 &= 127", "sp = r2", "rts")

    def test_direct_call_with_live_scalar_is_rejected(self):
        self.rejected("r2 = 4290772993", "r2 &= 127", "call 4 <consumer : 2001000 >", "rts")

    def test_unknown_consumer_and_unknown_predicate(self):
        self.rejected("r2 = 4290772993", "r2 = mystery(r2)", "r2 = 0", "rts")
        self.rejected("r2 = 4290772993", "ifs (mystery(r2)) goto @done", "r2 = 0", "done:", "rts")
        self.rejected("r2 = 4290772993", "<unknown instruction>", "r2 = 0", "rts")

    def test_branch_bypassing_kill_is_rejected(self):
        self.rejected("r2 = 4290772993", "if (r3 != 0) goto @escape", "r2 = 0",
                      "escape:", "[r0+4] = r2", "rts")

    def test_braced_alternatives_both_checked(self):
        self.accepted("r2 = 4290772993", "ifs (r3 >= r2) {", "r2 = 1", "} else {",
                      "r2 = 2", "}", "rts")
        self.rejected("r2 = 4290772993", "ifs (r3 >= r2) {", "r2 = 1", "} else {",
                      "[r0+4] = r2", "}", "r2 = 0", "rts")

    def test_braced_no_else_has_live_false_path(self):
        self.rejected("r2 = 4290772993", "if (r3 != 0) {", "r2 = 0", "}", "[r0+4] = r2", "rts")

    def test_unresolved_branch_and_truncated_context(self):
        self.rejected("r2 = 4290772993", "goto 4 <missing : 2008000 >", "r2 = 0", "rts")
        self.rejected("r2 = 4290772993", "r2 = 0")
        self.rejected("r2 = 4290772993", "if (r1 == 0) {", "r2 = 0", "rts")
        self.assertTrue(guard.references(assembly("r2 = 4290772993", "r2 = 0", "rts").split("\n", 1)[1]))

    def test_opaque_incoming_jump_invalidates_reaching_constant(self):
        self.rejected("if (r5 == 0) goto @normal", "goto r7", "normal:", "r3 = 27",
                      "r2 = 4290772993", "r2 = r2 / r3 (u)", "[r0+4] = r2", "rts")

    def test_control_register_write_invalidates_incoming_edges(self):
        for jump in ("pc = r7", "pc ^= r7", "rets = r7", "rets |= r7", "reti"):
            with self.subTest(jump=jump):
                self.rejected("if (r5 == 0) goto @normal", jump, "normal:", "r3 = 27",
                              "r2 = 4290772993", "r2 = r2 / r3 (u)", "[r0+4] = r2", "rts")

    def test_join_disagreement_invalidates_divisor(self):
        self.rejected("r3 = 27", "if (r5 == 0) goto @start", "r3 = 1", "start:",
                      "r2 = 4290772993", "r2 = r2 / r3 (u)", "[r0+4] = r2", "rts")

    def test_prior_auto_update_cannot_supply_stale_divisor(self):
        for update in ("r0 = [r3--]", "[r3--] = r0", "r0 = r3--"):
            with self.subTest(update=update):
                self.rejected("r3 = 2", update, "r2 = 4290772993", "r2 = r2 / r3 (u)",
                              "[r0+4] = r2", "rts")

    def test_prior_unknown_assignment_cannot_supply_constant(self):
        self.rejected("r3 = 27", "r0 = mysterious(r3)", "r2 = 4290772993",
                      "r2 = r2 / r3 (u)", "[r0+4] = r2", "rts")

    def test_unknown_and_zero_divisors_do_not_prove_scalar(self):
        for setup in ("r3 = [r0+4]", "r3 = 0"):
            with self.subTest(setup=setup):
                self.rejected(setup, "r2 = 4290772993", "r2 = r2 / r3 (u)", "[r0+4] = r2", "rts")

    def test_signed_divide_and_ambiguous_shift_do_not_narrow(self):
        self.rejected("r2 = 4290772993", "r3 = 27", "r2 = r2 / r3", "[r0+4] = r2", "rts")
        self.rejected("r2 = 4290772993", "r2 += r3", "r2 = r2 >> 1", "[r0+4] = r2", "rts")

    def test_recreating_rom_after_narrowing_is_rejected(self):
        self.rejected("r2 = 4290772993", "r2 &= 1", "r3 = 0xffbfffff", "r2 += r3", "[r0+4] = r2", "rts")

    def test_carry_escape_remains_live_after_register_kill(self):
        self.rejected("r2 = 4290772993", "r2 += r3", "r2 = 0", "r0 = r1 + c", "rts")

    def test_untainted_arithmetic_kills_carry_dependency(self):
        self.accepted("r2 = 4290772993", "r2 += r3", "r2 = 0", "r0 = r4 - r5", "rts")

    def test_parallel_candidate_and_alias_hazards_rejected(self):
        self.rejected("r2 = 4290772993 #", "r3 = r2", "r2 = 0", "r3 = 0", "rts")
        self.rejected("r2 = 4290772993", "r3 = r2 #", "r2 = 0", "r3 = 0", "rts")

    def test_parallel_arithmetic_cannot_hide_old_carry_use(self):
        self.rejected("r2 = 4290772993", "r2 += r3", "r2 = 0", "r0 = r4 - r5 #", "r1 = r6 + c", "rts")

    def test_live_cycle_fails_closed(self):
        self.rejected("r2 = 4290772993", "again:", "if (r1 != 0) goto @again", "r2 = 0", "rts")

    def test_annotation_does_not_override_operand_bit_zero(self):
        self.rejected("r2 = 4290772993 <object : ffc00000 >", "r0 = [r2+0]", "rts")
        self.accepted("r2 = 0 <object : ffc00000 >", "r0 = [r2+0]", "rts")


if __name__ == "__main__":
    unittest.main(verbosity=2)
