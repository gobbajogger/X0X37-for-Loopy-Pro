# Clip arp reuse — Stage 2 proof

- **Base:** `origin/cursor/stream4-combined-schema-53a1` @ `e280c753` (stream5 tip; includes merged #292 Chicago Arp)
- **Branch:** `cursor/clip-arp-reuse-stage2-ea88`
- **Change:** one shared pick/rate engine (`arp_pick.h`); Chicago adapters call it; clip path unchanged in behaviour

## Host selectors (`X0X_TEST_JOBS=8`, all exit 0)

```
chicago-arp
smk-clip-keys
bass303-amp-adsr
bass303-ampmode
bass303-wave-gnaw
project-stage
```

Log: `host_proof.log`

## Not run

- Elite/Pro package rebuild (Stage 4)
- Device flash
