#ifndef NETTOOLS_H
#define NETTOOLS_H

// main

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

// output

#define NETWORK_ACTION_PING 1
#define NETWORK_ACTION_TRACEROUTE 2
#define NETWORK_ACTION_WHOIS 3

VOID PerformNetworkAction(
    __in HWND hWnd,
    __in ULONG Action,
    __in PPH_IP_ADDRESS Address
    );

#endif
