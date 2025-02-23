#include <windows.h>
#include <activeds.h>
#include <iads.h>
#include <combaseapi.h>
#include <objbase.h>
#include <sddl.h>

extern "C" {
#include "beacon.h"
#include "querybofdefs.h"

HRESULT Query(IDirectorySearch* pContainerToSearch, wchar_t* ldapQuery, wchar_t* ldapFilter);
BOOL showResult = FALSE;

int go(IN char* Args, IN ULONG Length)
{
	// Parse Arguments
	datap parser;
	BeaconDataParse(&parser, Args, Length);
	char* ldapQueryA = BeaconDataExtract(&parser, NULL);
	char* ldapFilterA = BeaconDataExtract(&parser, NULL);
	int argCount = BeaconDataLength(&parser);
	formatp buffer;
	BeaconFormatAlloc(&buffer, 1024);
	BeaconFormatPrintf(&buffer, "LDAP Query:\t%s\n", ldapQueryA);
	if (MSVCRT$_stricmp(ldapFilterA, "\0") == 0)
	{
		BeaconFormatPrintf(&buffer, "No filter specified! Printing all data!\n");
		showResult = TRUE;
	}
	else
	{
		BeaconFormatPrintf(&buffer, "Specified Filter: %s\n", ldapFilterA);
	}
	BeaconPrintf(CALLBACK_OUTPUT, "%s", BeaconFormatToString(&buffer, NULL));
	BeaconFormatFree(&buffer);
	
	wchar_t ldapQuery[MAX_PATH];
	wchar_t ldapFilter[MAX_PATH];
	toWideChar(ldapQueryA, ldapQuery, MAX_PATH);
	toWideChar(ldapFilterA, ldapFilter, MAX_PATH);
	
	// Handle the command line arguments.
	int maxAlloc = MAX_PATH * 2;

	// Initialize COM
	// According to https://learn.microsoft.com/en-us/windows/win32/adsi/programming-language-support, Clients that do not support Automation have access to all ADSI interfaces, including both pure COM interfaces with the naming convention IDirectoryXXX and Automation COM interfaces with the naming convention IADsXXX. Because clients predominantly request information from directory services, the ADSI flexible query model through OLE DB and IDirectorySearch is effective.
	HRESULT hr = OLE32$CoInitialize(NULL);
	if (hr == S_OK)
	{
		// Get rootDSE and the current user's domain container DN.
		IADs* pObject = NULL;
		
		// An IID (Interface Identifier) is simply a GUID that identifies a specific interface in COM.
		// As a normal executable I don't have to explicitly set these values. I can just put IID_IADs and IID_IDirectorySearch for ADsOpenObject directly and it would get resolved.
		// According to chatgpt (lol) the reason why it looks like below instead of like {FD8256D0-FD15-11CE-ABC4-02608C9E7553} is because "in C++ as an IID or GUID, the structure is represented in a special format where the first part (32 bits) is a DWORD (unsigned 32-bit integer), followed by two WORD values (16-bit unsigned integers), and the last part is an array of BYTE values (8 bytes)". This totals up to 128 bits when is the length of a GUID.
		IID IID_IADs = { 0xFD8256D0, 0xFD15, 0x11CE, { 0xAB, 0xC4, 0x02, 0x60, 0x8C, 0x9E, 0x75, 0x53 } };
    		IID IID_IDirectorySearch = { 0x109BA8EC, 0x92F0, 0x11D0, { 0xA7, 0x90, 0x00, 0xC0, 0x4F, 0xD8, 0xD5, 0xA8 } };		
		
		//HMODULE hModule = LoadLibraryA("Activeds.dll"); // This gets loaded as soon as you call ADsOpenObject. Elastic flags this as suspicious LDAP Image Load
		//_ADsOpenObject ADsOpenObject = (_ADsOpenObject)
		//GetProcAddress(hModule, "ADsOpenObject");
		
		HRESULT hr = ACTIVEDS$ADsOpenObject(L"LDAP://rootDSE", NULL, NULL, 0x1, IID_IADs, (void**)&pObject); // Bind to the rootDSE to obtain the domain's Distinguished Name (DN). IID_IADs is the interface identifier for the COM interface we want.
		if (SUCCEEDED(hr)) // If we were able to bind to the rootDSE, lets obtain the defaultNamingContext
		{
			VARIANT var;
			hr = pObject->Get((BSTR)L"defaultNamingContext", &var); // Use the IADs rootDSE to obtain the default naming context.
			if (SUCCEEDED(hr)) // If we were able to obtain the defaultNamingContext, we should have the domain DN
			{
				// Build path to the domain container.
				wchar_t szPath[MAX_PATH];
				MSVCRT$wcscpy_s(szPath, MAX_PATH, L"LDAP://");
				MSVCRT$wcscat_s(szPath, MAX_PATH, var.bstrVal); // Put the entire LDAP://domain DN into szPath
				
				IDirectorySearch* pContainerToSearch = NULL; // This will be our IDirectorySearch COM interface which is used for querying directories.
				hr = ACTIVEDS$ADsOpenObject(szPath, NULL, NULL, 0x1, IID_IDirectorySearch, (void**)&pContainerToSearch); // Bind to the domain's DN. IID_IDirectorySearch is the interface identifier for the COM interface we want. IDirectorySearch interface https://learn.microsoft.com/en-us/windows/win32/api/iads/nn-iads-idirectorysearch is a pure COM interface for querying directories.
				if (SUCCEEDED(hr)) // If we were able to bind to the domain using its DN, then we can proceed with querying the domain
				{
					hr = Query(pContainerToSearch, ldapQuery, ldapFilter);
					if (SUCCEEDED(hr)) // This if, else if, else statement is pulled straight from the microsoft example code
					{
						if (S_FALSE == hr)
						{
							BeaconPrintf(CALLBACK_ERROR, "[!] No remaining user objects could be found.");
						}
							
					}
					else if (0x8007203e == hr)
						BeaconPrintf(CALLBACK_ERROR, "[!] Could not execute query. An invalid filter was specified.");
					else
						BeaconPrintf(CALLBACK_ERROR, "[!] Query failed to run. HRESULT: %x", hr);
				}
				else
				{
					BeaconPrintf(CALLBACK_ERROR, "[!] Could not execute query. Could not bind to the container.");
				}
				if (pContainerToSearch)
					pContainerToSearch->Release();
			}
			OLEAUT32$VariantClear(&var); // Clear the variant variable before the memory region is freed
		}
		else
		{
			BeaconPrintf(CALLBACK_ERROR, "[!] Could not execute query. Could not bind to LDAP://rootDSE.");
		}
		if (pObject)
			pObject->Release();
	}
	// Uninitialize COM
	OLE32$CoUninitialize();
	return 0;
}

HRESULT Query(IDirectorySearch* pContainerToSearch, wchar_t* ldapQuery, wchar_t* ldapFilter)
{
	// Create a ldap query
	wchar_t pszSearchFilter[MAX_PATH * 2]; // We want a region reserved here
	MSVCRT$wcscpy_s(pszSearchFilter, MAX_PATH * 2, ldapQuery); // L"(&(objectCategory=person)(servicePrincipalName=*))"

	// Specify subtree search
	ADS_SEARCHPREF_INFO SearchPrefs;
	SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
	SearchPrefs.vValue.Integer = ADS_SCOPE_SUBTREE; // Search all levels recursively
	DWORD dwNumPrefs = 1;

	// COL for iterations
	ADS_SEARCH_COLUMN col;
	HRESULT hr;

	// Set the search preferences for the IDirectorySearch COM interface
	hr = pContainerToSearch->SetSearchPreference(&SearchPrefs, dwNumPrefs);
	if (FAILED(hr))
	{
		return hr;
	}

	FILETIME filetime;
	SYSTEMTIME systemtime;
	DATE date;
	VARIANT varDate;
	LARGE_INTEGER liValue;
	LPOLESTR* pszPropertyList = NULL;

	// Handle used for searching
	ADS_SEARCH_HANDLE hSearch = NULL;
	wchar_t wideStr[MAX_PATH];
	char asciiStr[MAX_PATH];
	// Return all properties.
	hr = pContainerToSearch->ExecuteSearch(pszSearchFilter, NULL, -1L, &hSearch); // Execute a search using the IDirectorySearch COM interface
	if (SUCCEEDED(hr))
	{
		LPWSTR pszColumn = NULL;
		ADS_SEARCH_COLUMN col;
		// The way it works when retrieving the data is you get the first row of data, then you go through the columns of the first row. For the columns you first get the name, and using the name you can get the column.
		// After going through all the columns, you can get the next row and loop through it. After you loop through all the rows, close the search handle.
		hr = pContainerToSearch->GetFirstRow(hSearch); // Get the first row of the data. If it succeeded then we can call IDirectorySearch::GetNextRow() to retrieve the next row of data
		if (SUCCEEDED(hr))
		{
			while (hr != S_ADS_NOMORE_ROWS) // If the returned value of type HRESULT from GetNextRow is equal to S_ADS_NOMORE_ROWS, then stop.
			{
				// Loop through the array of passed column names, print the data for each column
				// Here specifically we are getting the column name and saving it into LPWSTR pszColumn
				while (pContainerToSearch->GetNextColumnName(hSearch, &pszColumn) != S_ADS_NOMORE_COLUMNS) // This will keep looping through all the columns in the row until the returned value of type HRESULT is equal to S_ADS_NOMORE_COLUMNS
				{
					if (showResult == TRUE || MSVCRT$wcsstr(ldapFilter, pszColumn) != NULL) // if (ldapFilter == NULL || ldapFilter[0] == L'\0' || wcsstr(ldapFilter, pszColumn) != NULL)
					{
						// We pass the column name here to retrieve the ADS_SEARCH_COLUMN structure that contains the actual data of the column from the current row of the search result.
						// The ADS_SEARCH_COLUMN column structure contains: pszAttrName, dwADsType, pADsValues, dwNumValues, and hReserved members
						hr = pContainerToSearch->GetColumn(hSearch, pszColumn, &col);
						if (SUCCEEDED(hr))
						{						
							// Print the column/attribute name (pszColumn & pszAttrName are the same value)
							BeaconPrintfW(CALLBACK_OUTPUT, L"Column Name: %s", col.pszAttrName); // pszAttrName is a unicode string of the attribute name

							// Loop through all values in the column and print them
							for (DWORD x = 0; x < col.dwNumValues; x++) // dwNumValues is the size of the pADsValues array
							{
								switch (col.dwADsType) // col.dwADsType is the the value from the ADSTYPEENUM enumeration that indicates how the attribute values are interpreted. Basically depending on what type of value the attribute is, we have to print our stuff differently. Note that not absolutely every possible type is listed below as a case, but most are.
								{ // https://learn.microsoft.com/en-us/windows/win32/api/iads/ne-iads-adstypeenum
								case ADSTYPE_DN_STRING:
								case ADSTYPE_CASE_EXACT_STRING:
								case ADSTYPE_CASE_IGNORE_STRING:
								case ADSTYPE_PRINTABLE_STRING:
								case ADSTYPE_NUMERIC_STRING:
								case ADSTYPE_TYPEDNAME:
								case ADSTYPE_FAXNUMBER:
								case ADSTYPE_PATH:
								case ADSTYPE_OBJECT_CLASS:
									BeaconPrintfW(CALLBACK_OUTPUT, L"\t%s", col.pADsValues[x].CaseIgnoreString); // col.pADsValues is an array of ADSVALUE structure containing the value of the attribute. We use [x] because we are looping through the array till wwe have the max size of the array
									break;
								case ADSTYPE_BOOLEAN:
								case ADSTYPE_INTEGER:
									BeaconPrintfW(CALLBACK_OUTPUT, L"\t%d", col.pADsValues[x].Integer);
									break;
								case ADSTYPE_OCTET_STRING:
									BeaconPrintfW(CALLBACK_OUTPUT, L"\t%s", col.pADsValues[x].OctetString);
									break;
								case ADSTYPE_UTC_TIME:
									systemtime = col.pADsValues[x].UTCTime;
									if (OLEAUT32$SystemTimeToVariantTime(&systemtime, &date) != 0)
									{
										// Pack in variant.vt
										varDate.vt = VT_DATE;
										varDate.date = date;
										OLEAUT32$VariantChangeType(&varDate, &varDate, VARIANT_NOVALUEPROP, VT_BSTR);
										BeaconPrintfW(CALLBACK_OUTPUT, L"\t%s", varDate.bstrVal);
										OLEAUT32$VariantClear(&varDate);
									}
									break;
								case ADSTYPE_LARGE_INTEGER:
									liValue = col.pADsValues[x].LargeInteger;
									filetime.dwLowDateTime = liValue.LowPart;
									filetime.dwHighDateTime = liValue.HighPart;
									if (KERNEL32$FileTimeToLocalFileTime(&filetime, &filetime) != 0)
									{
										if (KERNEL32$FileTimeToSystemTime(&filetime, &systemtime) != 0)
										{
											if (OLEAUT32$SystemTimeToVariantTime(&systemtime, &date) != 0)
											{
												// Pack in variant.vt
												varDate.vt = VT_DATE;
												varDate.date = date;
												OLEAUT32$VariantChangeType(&varDate, &varDate, VARIANT_NOVALUEPROP, VT_BSTR);
												BeaconPrintfW(CALLBACK_OUTPUT, L"\t%s", varDate.bstrVal);
												OLEAUT32$VariantClear(&varDate);
											}
										}
									}
									break;
								default:
									BeaconPrintfW(CALLBACK_OUTPUT, L"\tUnknown type %d.", col.dwADsType);
								
								}
							}

							// Free the column resources after looping through all the columns for the row
							pContainerToSearch->FreeColumn(&col);
						}
					}
				}

				// Get the next row and continue the loop again. This will keep going until we hit the S_ADS_NOMORE_ROWS
				hr = pContainerToSearch->GetNextRow(hSearch);
			}
		}

		// Close the search handle to clean up
		pContainerToSearch->CloseSearchHandle(hSearch);
	}
	if (SUCCEEDED(hr))
		hr = S_FALSE;

	BeaconPrintf(CALLBACK_OUTPUT, "--------------------------------------------------------------------");

	return hr;
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
}
