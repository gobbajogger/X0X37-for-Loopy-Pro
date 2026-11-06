#include <assert.h>
#include <string.h>
#include <stdio.h>
static int ble_central_role;
static struct { char nm[21]; } ble_r;
static unsigned count;
static char fixture[2][21];
static unsigned plat_ble_peer_names(char names[2][21]) { memcpy(names,fixture,sizeof fixture);return count; }
const char *old_name(void)
{
    if (!ble_central_role) return ble_r.nm;
    static char names[43];
    char peers[2][21];
    unsigned count = plat_ble_peer_names(peers), n = 0;
    for (unsigned p = 0; p < count; p++) {
        if (p) { names[n++] = ','; names[n++] = ' '; }
        for (unsigned i = 0; peers[p][i]; i++) names[n++] = peers[p][i];
    }
    names[n] = 0;
    return names;
}
const char *new_name(char *names)
{
    if (!ble_central_role) return ble_r.nm;
    char peers[2][21];
    unsigned count = plat_ble_peer_names(peers), n = 0;
    for (unsigned p = 0; p < count; p++) {
        if (p) { names[n++] = ','; names[n++] = ' '; }
        for (unsigned i = 0; peers[p][i]; i++) names[n++] = peers[p][i];
    }
    names[n] = 0;
    return names;
}

int main(void) {
 unsigned checks=0;
 for(int central=0;central<2;central++) for(count=0;count<=2;count++)
 for(unsigned a=0;a<=20;a++) for(unsigned b=0;b<=20;b++) {
  ble_central_role=central;memcpy(ble_r.nm,"Device Name",12);
  memset(fixture,0,sizeof fixture);memset(fixture[0],'W',a);memset(fixture[1],',',b);
  struct { unsigned before;char text[43];unsigned after; } q={0x12345678,{0},0x87654321};
  const char *expected=old_name(); const char *actual=new_name(q.text);
  assert(!strcmp(expected,actual));assert(q.before==0x12345678 && q.after==0x87654321);checks++;
 }
 printf("PASS %u old/new name outputs and canaries\n",checks);
}
