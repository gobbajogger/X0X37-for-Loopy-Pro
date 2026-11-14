# M12 Non-table Identity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Select the canonical non-table COMP1 request without inventing original table provenance or waiving evidence obligations.

**Architecture:** Add optional unique Request/source parsing and an explicit policy identity selector. Preserve the existing source-section/original-row selector and all downstream proof rules.

**Tech Stack:** Python 3.12, Git object reads, real-CLI unittest fixtures.

**Spec:** ../specs/2026-11-14-m12-nontable-identity-design.md

## Global Constraints

- Base #926 `0e2b7b78d223e10af1e9172db6dee657a8d590cb`; isolated branch `wip/m12-nontable-identity-20261114`.
- Only adapter, tests and stage docs change. No product-policy selection, existing runner, firmware, package, preview, publisher, publication or device changes.
- Preserve #119/26.7/M5 holds and source/behavior/host/native/device distinctions.

## Review Focus

- Non-table selection cannot bypass preserved table identities.
- Fenced or duplicate source lines and mixed selectors refuse validation.
- Wrong queue/source digest refuses validation.
- Missing/negative/higher-level proof remains fail-closed.
- COMP1 parser exercise must not use private/denied companion payloads.

## Task 1: Explicit non-table identity

**Files:** `tools/m12_acceptance.py`, `tests/host/test_m12_acceptance.py`, adapter README and sanitized stage receipts.

**Interface:** Selection may use `identity: {kind: request-source, document: canonical queue path, sha256: SHA256(exact Request/source value)}` instead of `source_section` and `row`. Hash the exact nonempty one-line value after `**Request/source:** `, UTF-8 without newline. Require a unique value and no preserved table source identity. Reject mixed selectors.

- [x] Add real-Git CLI tests covering valid mixed table/non-table policy, missing/duplicate/fenced source, changed value/hash, wrong document/kind, mixed selectors and table downgrade. Run and observe intended RED failures.
- [x] Add parsing and identity validation only; retain existing obligation rules.
- [x] Run focused suite and syntax checks. Add non-table missing/negative/higher-level evidence checks.
- [x] Exercise exact current #887 COMP1 metadata using an explicitly synthetic source policy; retain all product/real-device/publication limitations.
- [x] Freeze hashes, obtain independent Sol 6.1 ultra review, and fix substantive findings with RED→GREEN proof.
- [ ] Deliver reviewed source-only branch/PR with exact remote readback and route to existing firmware acceptance owner. No source delivery until verified.
