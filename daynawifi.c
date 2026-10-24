/*
 * daynawifi - the Wi-Fi of a BlueSCSI/ZuluSCSI DaynaPORT, from a Shell.
 *
 *   daynawifi            what the adapter says it is joined to, and its signal
 *   daynawifi REJOIN     reset its packet buffer and join the network again -
 *                        for an adapter that says "connected" while the router
 *                        has dropped it (no packets come in, and nothing short
 *                        of a power cycle used to bring it back)
 *   daynawifi SCAN       the networks it can see
 *
 * The adapter, its SCSI driver and ID, and the network name and key come from
 * ENV:scsidayna.prefs (SSID= and KEY= lines, as for AUTOCONNECT); SSID= and
 * KEY= on the command line override them. Stop the TCP/IP stack first when it
 * is using the adapter (the driver polls the adapter while a stack is up).
 *
 * Same licence as scsidayna.device (licence.txt).
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/rdargs.h>
#include <string.h>
#include <stdio.h>
#include "scsiwifi.h"

static const char version[] = "$VER: daynawifi 1.0 (24.10.2026)";

struct Library *UtilityBase;

static void show(SCSIWIFIDevice dev)
{
    struct SCSIWifi_NetworkEntry net;
    struct SCSIWifi_MACAddress mac;
    memset(&net, 0, sizeof(net));
    if (SCSIWifi_getNetwork(dev, &net) && net.rssi)
        printf("joined to \"%s\", signal %d dB, channel %d\n", net.ssid, (int)net.rssi, (int)net.channel);
    else
        printf("not joined to any network\n");
    if (SCSIWifi_getMACAddress(dev, &mac) && mac.valid)
        printf("adapter MAC %02x:%02x:%02x:%02x:%02x:%02x\n", mac.address[0], mac.address[1], mac.address[2],
               mac.address[3], mac.address[4], mac.address[5]);
}

static int scan(SCSIWIFIDevice dev)
{
    enum SCSIWifi_ScanStatus st;
    struct SCSIWifi_ScanResults *res;
    int i, t;
    if (!SCSIWifi_scan(dev, &st)) { printf("the adapter refused to scan\n"); return 10; }
    for (t = 0; t < 30; t++) {
        Delay(25);
        if (!SCSIWifi_scanComplete(dev, &st) || st != swssBusy) break;
    }
    res = AllocVec(sizeof(*res), MEMF_PUBLIC | MEMF_CLEAR);
    if (!res) return 20;
    if (SCSIWifi_getScanResults(dev, res)) {
        printf("%d network%s:\n", (int)res->count, res->count == 1 ? "" : "s");
        for (i = 0; i < res->count && i < SCSIWIFI_MAX_NETWORK_COUNT; i++)
            printf("  %-32s %4d dB  ch %d\n", res->networks[i].ssid, (int)res->networks[i].rssi,
                   (int)res->networks[i].channel);
    } else
        printf("no scan results\n");
    FreeVec(res);
    return 0;
}

int main(void)
{
    LONG args[4] = { 0 };
    struct RDArgs *rda;
    struct ScsiDaynaSettings s;
    struct SCSIDevice_OpenData od;
    enum SCSIWifi_OpenResult err;
    SCSIWIFIDevice dev;
    int rc = 0;

    rda = ReadArgs("REJOIN/S,SCAN/S,SSID/K,KEY/K", args, NULL);
    if (!rda) { PrintFault(IoErr(), "daynawifi"); return 10; }
    UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase) { FreeArgs(rda); return 20; }

    SCSIWifi_defaultSettings(&s);
    if (!SCSIWifi_loadSettings((void *)UtilityBase, (void *)DOSBase, &s))
        printf("no ENV:scsidayna.prefs - using the defaults (%s)\n", s.deviceName);
    if (args[2]) { strncpy(s.ssid, (char *)args[2], sizeof(s.ssid) - 1); s.ssid[sizeof(s.ssid) - 1] = 0; }
    if (args[3]) { strncpy(s.key, (char *)args[3], sizeof(s.key) - 1); s.key[sizeof(s.key) - 1] = 0; }

    memset(&od, 0, sizeof(od));
    od.sysBase = SysBase;
    od.utilityBase = (struct UtilityBase *)UtilityBase;
    od.dosBase = (struct DosBase *)DOSBase;
    od.scsiMode = s.scsiMode;
    od.deviceDriverName = s.deviceName;
    if (s.deviceID >= 0 && s.deviceID <= 7) {
        od.deviceID = s.deviceID;
        dev = SCSIWifi_open(&od, &err);
    } else {
        /* DEVICEID=-1: the adapter is on whichever unit answers as one (as the driver looks) */
        int id;
        for (id = 0, dev = NULL; id < 8 && !dev; id++) {
            od.deviceID = id;
            dev = SCSIWifi_open(&od, &err);
        }
        if (dev) printf("adapter on %s unit %d\n", s.deviceName, (int)od.deviceID);
    }
    if (!dev) {
        printf("no DaynaPORT Wi-Fi adapter on %s (unit %d): open result %d\n", s.deviceName, (int)s.deviceID, (int)err);
        rc = 10;
    } else if (args[1]) {
        rc = scan(dev);
    } else if (args[0]) {
        struct SCSIWifi_JoinRequest req;
        struct SCSIWifi_NetworkEntry net;
        int t;
        if (!s.ssid[0]) {
            printf("no SSID: put SSID= and KEY= in ENV:scsidayna.prefs, or give them here\n");
            rc = 10;
        } else {
            printf("before: "); show(dev);
            /* enable off/on resets the adapter's packet buffer (scsiwifi.h) */
            SCSIWifi_enable(dev, 0);
            Delay(25);
            SCSIWifi_enable(dev, 1);
            Delay(25);
            memset(&req, 0, sizeof(req));
            strncpy(req.ssid, s.ssid, sizeof(req.ssid) - 1);
            strncpy(req.key, s.key, sizeof(req.key) - 1);
            printf("joining \"%s\"\n", req.ssid);
            if (!SCSIWifi_joinNetwork(dev, &req))
                printf("the adapter refused the join request\n");
            for (t = 0, rc = 5; t < 40; t++) {         /* up to 40 s */
                Delay(50);
                memset(&net, 0, sizeof(net));
                if (SCSIWifi_getNetwork(dev, &net) && net.rssi && !strcmp(net.ssid, req.ssid)) {
                    printf("joined after %d s: ", t + 1); show(dev);
                    rc = 0;
                    break;
                }
            }
            if (rc) printf("not joined after 40 s\n");
        }
    } else
        show(dev);
    if (dev) SCSIWifi_close(dev);
    CloseLibrary((struct Library *)UtilityBase);
    FreeArgs(rda);
    return rc;
}
