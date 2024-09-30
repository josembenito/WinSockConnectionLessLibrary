// getadaptersinfo.cpp : Defines the entry point for the console application.
//


#ifdef Compile
#include <WinSock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib,"IPHLPAPI.lib")
#pragma comment(lib,"Ws2_32.lib")

using namespace std;

void displayAddress(const SOCKET_ADDRESS& Address)
{
    cout << "\n  Length of sockaddr: " << Address.iSockaddrLength;
    if (Address.lpSockaddr->sa_family == AF_INET)
    {
        sockaddr_in* si = (sockaddr_in*)(Address.lpSockaddr);
        char a[INET_ADDRSTRLEN] = {};
        if (inet_ntop(AF_INET, &(si->sin_addr), a, sizeof(a)))
            cout << "\n   IPv4 address: " << a;
    }
    else if (Address.lpSockaddr->sa_family == AF_INET6)
    {
        sockaddr_in6* si = (sockaddr_in6*)(Address.lpSockaddr);
        char a[INET6_ADDRSTRLEN] = {};
        if (inet_ntop(AF_INET6, &(si->sin6_addr), a, sizeof(a)))
            cout << "\n   IPv6 address: " << a;
    }
}

int main()
{
    cout << "\nUsing GetAdaptersAddresses";

    ULONG size = 1024 * 15;
    PIP_ADAPTER_ADDRESSES p = (IP_ADAPTER_ADDRESSES*)HeapAlloc(GetProcessHeap(), 0, size);
    if (!p)
    {
        cout << "\nCannot allocate memory";
        cin.get();
        return -1;
    }

    ULONG ret;
    do
    {
        ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, p, &size);
        if (ret != ERROR_BUFFER_OVERFLOW)
            break;

        PIP_ADAPTER_ADDRESSES newp = (IP_ADAPTER_ADDRESSES*)HeapReAlloc(GetProcessHeap(), 0, p, size);
        if (!newp)
        {
            cout << "\nCannot reallocate memory";
            HeapFree(GetProcessHeap(), 0, p);
            cin.get();
            return -1;
        }

        p = newp;
    } while (true);

    if (ret != NO_ERROR)
    {
        cout << "\nSomething went wrong. Error: " << ret;
        HeapFree(GetProcessHeap(), 0, p);
        cin.get();
        return -1;
    }

    int i = 0;
    for (PIP_ADAPTER_ADDRESSES tp = p; tp != NULL; tp = tp->Next)
    {
        ++i;
        cout << "\nLength of IP Adapter info: " << tp->Length;
        cout << "\n IPv4 IfIndex: " << tp->IfIndex;
        cout << "\n Adapter name: " << tp->AdapterName;
        cout << "\n Unicast addresses:";
        int j = 0;
        for (PIP_ADAPTER_UNICAST_ADDRESS pu = tp->FirstUnicastAddress; pu != NULL; pu = pu->Next)
        {
            ++j;
            displayAddress(pu->Address);
        }
        cout << "\n # of Unicast addresses: " << j;
        cout << "\n Anycast addresses:";
        j = 0;
        for (PIP_ADAPTER_ANYCAST_ADDRESS pa = tp->FirstAnycastAddress; pa != NULL; pa = pa->Next)
        {
            ++j;
            displayAddress(pa->Address);
        }
        cout << "\n # of Anycast addresses: " << j;
        cout << "\n Multicast addresses:";
        j = 0;
        for (PIP_ADAPTER_MULTICAST_ADDRESS pm = tp->FirstMulticastAddress; pm != NULL; pm = pm->Next)
        {
            ++j;
            displayAddress(pm->Address);
        }
        cout << "\n # of Multicast addresses: " << j;
        cout << "\n DNS server addresses:";
        j = 0;
        for (PIP_ADAPTER_DNS_SERVER_ADDRESS pd = tp->FirstDnsServerAddress; pd != NULL; pd = pd->Next)
        {
            ++j;
            displayAddress(pd->Address);
        }
        cout << "\n # of DNS server addresses: " << j;
        cout << "\n Gateway addresses:";
        j = 0;
        for (PIP_ADAPTER_GATEWAY_ADDRESS_LH pg = tp->FirstGatewayAddress; pg != NULL; pg = pg->Next)
        {
            ++j;
            displayAddress(pg->Address);
        }
        cout << "\n # of Gateway addresses: " << j;
        cout << "\n DNS suffix" << tp->DnsSuffix;
        cout << "\n Description" << tp->Description;
        cout << "\n Friendly name" << tp->FriendlyName;
        if (tp->PhysicalAddressLength != 0)
        {
            cout << "\n Physical address: ";
            cout << std::hex << (int)tp->PhysicalAddress[0];
            for (UINT i = 1; i < tp->PhysicalAddressLength; i++)
                cout << "-" << std::hex << (int)tp->PhysicalAddress[i];
        }
        cout << "\n Flags" << tp->Flags;
        cout << "\n MTU" << tp->Mtu;
        cout << "\n IfType" << tp->IfType;
        cout << "\n OperStatus" << tp->OperStatus;
        cout << "\n IPv6 IfIndex :" << tp->Ipv6IfIndex;
        cout << "\n and more...";
    }
    cout << "\n# of IP Adapters: " << i;

    HeapFree(GetProcessHeap(), 0, p);

    cin.get();
    return 0;
}
#endif // Compile