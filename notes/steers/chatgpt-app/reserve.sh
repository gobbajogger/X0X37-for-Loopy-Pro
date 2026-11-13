#!/bin/sh
# One-owner reservation for the shared, signed-in ChatGPT app on the box (display :14).
# Usage: reserve.sh claim <owner> <task> [minutes=20] | release <owner> | status
# Rules: one owner at a time; bounded timeout; release at each UI handoff; a stale
# reservation may be reclaimed only after checking the owner is not live in the app.
# Never close the app or another session. No credential copying.
D=/workspace/steers/chatgpt-app; R=$D/reservation; L=$D/log.txt; now=$(date +%s)
exec 9>$D/.lock; flock 9
case "$1" in
 status) [ -f $R ] && cat $R || echo FREE ;;
 claim) o=$2; t=$3; m=${4:-20}
  if [ -f $R ]; then . $R
    if [ "$now" -lt "$EXPIRES" ] && [ "$OWNER" != "$o" ]; then echo "BUSY owner=$OWNER task=$TASK until=$(date -d @$EXPIRES '+%H:%M %Z')"; exit 1; fi
    if [ "$now" -ge "$EXPIRES" ] && [ "$OWNER" != "$o" ]; then echo "STALE reservation by $OWNER (task $TASK): check live ownership in the app before reclaiming; rerun with RECLAIM=1"; [ "$RECLAIM" = 1 ] || exit 2; fi
  fi
  printf 'OWNER=%s\nTASK="%s"\nSTARTED=%s\nEXPIRES=%s\n' "$o" "$t" $now $((now+m*60)) > $R
  echo "$(date '+%F %T %Z') CLAIM $o \"$t\" ${m}m" >> $L; echo "CLAIMED by $o for ${m} min" ;;
 release) [ -f $R ] && . $R; if [ -f $R ] && [ "$OWNER" = "$2" ]; then rm -f $R; echo "$(date '+%F %T %Z') RELEASE $2" >> $L; echo RELEASED; else echo "NOT OWNER (holder: ${OWNER:-none})"; exit 1; fi ;;
 *) echo "usage: $0 claim <owner> <task> [min] | release <owner> | status"; exit 64 ;;
esac
