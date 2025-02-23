#include <windows.h>
#include <stdio.h>
#include <lmaccess.h>
#include <dsgetdc.h>
#include "beacon.h"
#include "basicreconbofdefs.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "ADSIid.lib")
#pragma comment(lib, "ActiveDS.Lib")

PDOMAIN_CONTROLLER_INFOW pDCI;

BOOL BasicRecon(void)
{
	// All the functions I used to retrieve information from Active Directory can be found here: https://learn.microsoft.com/en-us/windows/win32/api/dsgetdc/  and https://learn.microsoft.com/en-us/windows/win32/api/lmaccess/
	// Here is the example code provided by Microsoft for enumerating domain controllers: https://learn.microsoft.com/en-us/windows/win32/ad/enumerating-domain-controllers

	DWORD dwRet = NETAPI32$DsGetDcNameW // Retrieves the name of a domain controller in a specified domain. (All NULLs will be the current computers domain).
	(NULL,
		NULL,
		NULL,
		NULL,
		0,			// You can select flags that specify characteristics of the domain controller if needed.
		&pDCI);		// Contains data about the DC selected. Must be freed with NetApiBufferFree at the end.

	if (dwRet == ERROR_SUCCESS)
	{		
		// Print out info obtained from DsGetDcNameA
		BeaconPrintfW(CALLBACK_OUTPUT, L"[+] Domain Name: \n\t%s", pDCI->DomainName);
		BeaconPrintfW(CALLBACK_OUTPUT, L"[+] DnsForestName:\n\t%s", pDCI->DnsForestName);
		BeaconPrintfW(CALLBACK_OUTPUT, L"[+] Domain Controller Name:\n\t%s", pDCI->DomainControllerName);
		BeaconPrintfW(CALLBACK_OUTPUT, L"[+] Domain Controller Address:\n\t%s", pDCI->DomainControllerAddress);

		// We can retrieve some additional information with User Modal functions: https://learn.microsoft.com/en-us/windows/win32/netmgmt/user-modal-functions
		// Specifically NetUserModalsGet is useful for our use case, but all of the functions are very powerful. This may be helpful: https://learn.microsoft.com/en-us/windows/win32/api/lmaccess/nf-lmaccess-netusergetlocalgroups and this: https://learn.microsoft.com/en-us/windows/win32/api/lmjoin/nf-lmjoin-netjoindomain
		USER_MODALS_INFO_0* buf0 = NULL;
		NETAPI32$NetUserModalsGet(pDCI->DomainControllerName, 0, (LPBYTE*)&buf0);
		BeaconPrintf(CALLBACK_OUTPUT, "[+] Password Policy:");
		BeaconPrintf(CALLBACK_OUTPUT, "\tMinimum Password Length: %d", buf0->usrmod0_min_passwd_len);
		BeaconPrintf(CALLBACK_OUTPUT, "\tPassword History Length: %d", buf0->usrmod0_password_hist_len);
		BeaconPrintf(CALLBACK_OUTPUT, "\tMaximum Password Age: %d", buf0->usrmod0_max_passwd_age / 86400);
		BeaconPrintf(CALLBACK_OUTPUT, "\tMinimum Password Age: %d", buf0->usrmod0_min_passwd_age / 86400);

		USER_MODALS_INFO_3* buf3 = NULL;
		NETAPI32$NetUserModalsGet(pDCI->DomainControllerName, 3, (LPBYTE*)&buf3);
		BeaconPrintf(CALLBACK_OUTPUT, "\tLockout Duration: %d", buf3->usrmod3_lockout_duration);
		BeaconPrintf(CALLBACK_OUTPUT, "\tLockout Observation Window: %d", buf3->usrmod3_lockout_observation_window);
		BeaconPrintf(CALLBACK_OUTPUT, "\tLockout Threshold: %d", buf3->usrmod3_lockout_threshold);


		// This next part will just print out all of the DCs, no other information about them.
		HANDLE hGetDCcontext;
		dwRet = NETAPI32$DsGetDcOpenW	// Opens a new domain controller enumeration option when given the DNS name of the domain
		(pDCI->DomainName,		// THe DNS name of the domain to open a new domain controller enumeration for
			DS_NOTIFY_AFTER_SITE_RECORDS, // The DsGetDcNext function will return the ERROR_FILEMARK_DETECTED value after all of the site-specific domain controllers are retrieved.
			NULL,
			NULL,
			NULL,
			0,
			&hGetDCcontext);		// Receives the domain controller enumeration context handle. This handle is used with the DsGetDcNext function to identify the domain controller enumeration operation. This handle needs to be passed to DsGetDcClose to close the domain controller enumeration operation.

		if (dwRet == ERROR_SUCCESS)
		{
			BeaconPrintf(CALLBACK_OUTPUT, "[+] Printing out additionally discovered Domain Controller(s) names:");
			while (TRUE)
			{
				LPWSTR lpsDnsHostName;
				dwRet = NETAPI32$DsGetDcNextW	// Retrieves the next domain controller in a domain controller enumeration operation.
				(hGetDCcontext,		// We pass in the domain controller enumeration context handle provided by the DsGetDcOpen function
					NULL,		// Receives the number of elements in the SockAddresses array. If this parameter is NULL, socket addresses are not retrieved.
					NULL,		// Array of SOCKET_ADDRESS structures that receives the socket address data for the domain controller. SockAddressCount receives the number of elements in this array. Ignored if SockAddressCount is NULL. MUST BE FREED WITH LocalFree.
					&lpsDnsHostName);		// Receives the DNS name of the domain controller. This parameter receives NULL if no host name is known. The caller must free this memory when it is no longer required by calling NetApiBufferFree.

				if (dwRet == ERROR_SUCCESS)
				{
					BeaconPrintfW(CALLBACK_OUTPUT, L"\t%s", lpsDnsHostName);

					// Free the allocated string.
					NETAPI32$NetApiBufferFree(lpsDnsHostName);

					// Free the socket address array. (If we didn't pass NULL into DsGetDcNextA)
					//LocalFree(rgSocketAddresses);
				}
				if (dwRet == ERROR_NO_MORE_ITEMS)
				{
					BeaconPrintf(CALLBACK_ERROR, "No more items!");
					break;
				}
				if (dwRet = ERROR_FILEMARK_DETECTED)
				{
					BeaconPrintf(CALLBACK_ERROR, "End of site-specific domain controllers!");
					continue;
				}
				else
				{
					// Some error occurred
					BeaconPrintf(CALLBACK_ERROR, "An error occurred!");
					break;
				}
			}

			// Close handle to the domain controller enumeration context now that we are finished looping
			NETAPI32$DsGetDcCloseW(hGetDCcontext);
		}

		// Free the pDCI buffer obtained from DsGetDcNameA
		NETAPI32$NetApiBufferFree(pDCI);
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to retrieve information about the DC!");
		return -1;
	}
	return TRUE;
}

void BeaconPrintfW(int type, const wchar_t* fmt, ...)
{
	wchar_t wideStr[MAX_PATH];
	char asciiStr[MAX_PATH];
	va_list args;
    	va_start(args, fmt);
	MSVCRT$_vsnwprintf_s(wideStr, MAX_PATH, _TRUNCATE, fmt, args);
	KERNEL32$WideCharToMultiByte(CP_ACP, 0, wideStr, -1, asciiStr, MAX_PATH, NULL, NULL);
	BeaconPrintf(type, asciiStr);
	va_end(args);
}
