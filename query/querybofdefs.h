#include <windows.h>

#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)
//#define MSVCRT$wcscpy_s wcscpy_s

WINOLEAPI OLE32$CoInitialize (LPVOID pvReserved);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT int __cdecl MSVCRT$_stricmp(const char *string1,const char *string2);
WINBASEAPI int __cdecl MSVCRT$_wcsicmp(const wchar_t *_Str1, const wchar_t *_Str2);
WINBASEAPI wchar_t *__cdecl MSVCRT$wcsstr(const wchar_t *_Str,const wchar_t *_SubStr);
WINBASEAPI errno_t __cdecl MSVCRT$wcscpy_s(wchar_t *_Dst, rsize_t _DstSize, const wchar_t *_Src);
WINBASEAPI wchar_t *__cdecl MSVCRT$wcscat_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource);
WINBASEAPI int __cdecl MSVCRT$swprintf_s(wchar_t *buffer, size_t sizeOfBuffer, const wchar_t *format, ...);
WINBASEAPI int MSVCRT$_vsnwprintf_s(wchar_t *dest, size_t destsz, size_t count, const wchar_t *format, va_list argptr); // https://www.educative.io/answers/what-is-vsnwprintfs-in-c
WINBASEAPI int WINAPI KERNEL32$WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar);
DECLSPEC_IMPORT void   BeaconPrintfW(int type, const wchar_t* fmt, ...);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);
DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$VariantChangeType(VARIANTARG *pvargDest, const VARIANTARG *pvarSrc, USHORT wFlags, VARTYPE vt);

DECLSPEC_IMPORT INT WINAPI OLEAUT32$SystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, DOUBLE *pvtime);
WINBASEAPI int WINAPI KERNEL32$FileTimeToLocalFileTime(CONST FILETIME *lpFileTime, LPFILETIME lpLocalFileTime);
WINBASEAPI int WINAPI KERNEL32$FileTimeToSystemTime(CONST FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);

DECLSPEC_IMPORT HRESULT WINAPI ACTIVEDS$ADsOpenObject(LPCWSTR lpszPathName, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwReserved, REFIID riid, void** ppObject);

/*typedef HRESULT (WINAPI *_ADsOpenObject)(
	LPCWSTR lpszPathName, 
	LPCWSTR lpszUserName, 
	LPCWSTR lpszPassword, 
	DWORD dwReserved, 
	REFIID riid, 
	void **ppObject
	);
*/
