#include <windows.h>
#include <activeds.h>

#define DS_NOTIFY_AFTER_SITE_RECORDS 0x02 // Obtained from DsGetDC.h
typedef struct _SOCKET_ADDRESS {
	LPSOCKADDR lpSockaddr;
	INT        iSockaddrLength;
} SOCKET_ADDRESS, * PSOCKET_ADDRESS, * LPSOCKET_ADDRESS;

DECLSPEC_IMPORT DWORD WINAPI NETAPI32$DsGetDcNameW(LPCWSTR ComputerName, LPCWSTR DomainName, GUID *DomainGuid, LPCWSTR SiteName, ULONG Flags, PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);
DECLSPEC_IMPORT DWORD WINAPI NETAPI32$NetUserModalsGet(LPCWSTR servername, DWORD level, LPBYTE  *bufptr);
DECLSPEC_IMPORT DWORD WINAPI NETAPI32$DsGetDcOpenW(LPCWSTR DnsName, ULONG OptionFlags, LPCWSTR SiteName, GUID *DomainGuid, LPCWSTR DnsForestName, ULONG DcFlags, PHANDLE RetGetDcContext);
DECLSPEC_IMPORT DWORD WINAPI NETAPI32$DsGetDcNextW(HANDLE GetDcContextHandle, PULONG SockAddressCount, LPSOCKET_ADDRESS *SockAddresses, LPWSTR *DnsHostName);
DECLSPEC_IMPORT DWORD WINAPI NETAPI32$NetApiBufferFree(LPVOID Buffer);
DECLSPEC_IMPORT DWORD WINAPI NETAPI32$DsGetDcCloseW(HANDLE GetDcContextHandle);
WINBASEAPI int __cdecl MSVCRT$swprintf_s(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...);
WINBASEAPI int MSVCRT$_vsnwprintf_s(wchar_t *dest, size_t destsz, size_t count, const wchar_t *format, va_list argptr);
WINBASEAPI int WINAPI KERNEL32$WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar);
DECLSPEC_IMPORT void   BeaconPrintfW(int type, const wchar_t* fmt, ...);
