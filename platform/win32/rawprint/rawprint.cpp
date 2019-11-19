// rawprint.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <comutil.h> //comsuppw.lib or comsuppwd.lib
//#include <StdIO.h>
#include <string>
#include <atlstr.h>
#include <atlconv.h>
// **********************************************************************
// PrintError - uses printf() to display error code information
// 
// Params:
//   dwError       - the error code, usually from GetLastError()
//   lpString      - some caller-defined text to print with the error info
// 
// Returns: void
// 
void PrintError(DWORD dwError, LPCTSTR lpString)
{
#define MAX_MSG_BUF_SIZE 512
	TCHAR 	*msgBuf;
	DWORD	cMsgLen;

	cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, dwError,
		MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR)&msgBuf,
		MAX_MSG_BUF_SIZE, NULL);
	//printf(_T("%s Error [%d]:: %s\n"), lpString, dwError, msgBuf);
	std::cout << lpString << dwError << msgBuf << std::endl;
	LocalFree(msgBuf);
#undef MAX_MSG_BUF_SIZE
}
// end PrintError
// **********************************************************************

// **********************************************************************
// ReadFileWithAlloc - allocates memory for and reads contents of a file
// 
// Params:
//   szFileName   - NULL terminated string specifying file name
//   pdwSize      - address of variable to receive file bytes size
//   ppBytes      - address of pointer which will be allocated and contain file bytes
// 
// Returns: TRUE for success, FALSE for failure.
//
// Notes: Caller is responsible for freeing the memory using GlobalFree()
// 
BOOL ReadFileWithAlloc(LPTSTR szFileName, LPDWORD pdwSize, LPBYTE *ppBytes)
{
	HANDLE		hFile;
	DWORD		dwBytes;
	BOOL		bSuccess = FALSE;

	// Validate pointer parameters
	if ((pdwSize == NULL) || (ppBytes == NULL))
		return FALSE;
	// Open the file for reading
	hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PrintError(GetLastError(), TEXT("CreateFile()"));
		return FALSE;
	}
	// How big is the file?
	*pdwSize = GetFileSize(hFile, NULL);
	if (*pdwSize == (DWORD)-1)
		PrintError(GetLastError(), TEXT("GetFileSize()"));
	else
	{
		// Allocate the memory
		*ppBytes = (LPBYTE)GlobalAlloc(GPTR, *pdwSize);
		if (*ppBytes == NULL)
			PrintError(GetLastError(), TEXT("Failed to allocate memory\n"));
		else
		{
			// Read the file into the newly allocated memory
			bSuccess = ReadFile(hFile, *ppBytes, *pdwSize, &dwBytes, NULL);
			if (!bSuccess)
				PrintError(GetLastError(), TEXT("ReadFile()"));
		}
	}
	// Clean up
	CloseHandle(hFile);
	return bSuccess;
}
// End ReadFileWithAlloc
// **********************************************************************

// **********************************************************************
// RawDataToPrinter - sends binary data directly to a printer
// 
// Params:
//   szPrinterName - NULL terminated string specifying printer name
//   lpData        - Pointer to raw data bytes
//   dwCount       - Length of lpData in bytes
// 
// Returns: TRUE for success, FALSE for failure.
// 
BOOL RawDataToPrinter(LPTSTR szPrinterName, LPBYTE lpData, DWORD dwCount)
{
	HANDLE     hPrinter;
	DOC_INFO_1 DocInfo;
	DWORD      dwJob;
	DWORD      dwBytesWritten;

	// Need a handle to the printer.
	if (!OpenPrinter(szPrinterName, &hPrinter, NULL))
	{
		PrintError(GetLastError(), TEXT("OpenPrinter"));
		return FALSE;
	}

	// Fill in the structure with info about this "document."
	DocInfo.pDocName = const_cast<TCHAR*>(TEXT("My Document"));
	DocInfo.pOutputFile = NULL;
	DocInfo.pDatatype = const_cast<TCHAR*>(TEXT("RAW"));
	// Inform the spooler the document is beginning.
	if ((dwJob = StartDocPrinter(hPrinter, 1, (LPBYTE)&DocInfo)) == 0)
	{
		PrintError(GetLastError(), TEXT("StartDocPrinter"));
		ClosePrinter(hPrinter);
		return FALSE;
	}
	// Start a page.
	if (!StartPagePrinter(hPrinter))
	{
		PrintError(GetLastError(), TEXT("StartPagePrinter"));
		EndDocPrinter(hPrinter);
		ClosePrinter(hPrinter);
		return FALSE;
	}
	// Send the data to the printer.
	if (!WritePrinter(hPrinter, lpData, dwCount, &dwBytesWritten))
	{
		PrintError(GetLastError(), TEXT("WritePrinter"));
		EndPagePrinter(hPrinter);
		EndDocPrinter(hPrinter);
		ClosePrinter(hPrinter);
		return FALSE;
	}
	// End the page.
	if (!EndPagePrinter(hPrinter))
	{
		PrintError(GetLastError(), TEXT("EndPagePrinter"));
		EndDocPrinter(hPrinter);
		ClosePrinter(hPrinter);
		return FALSE;
	}
	// Inform the spooler that the document is ending.
	if (!EndDocPrinter(hPrinter))
	{
		PrintError(GetLastError(), TEXT("EndDocPrinter"));
		ClosePrinter(hPrinter);
		return FALSE;
	}
	// Tidy up the printer handle.
	ClosePrinter(hPrinter);
	// Check to see if correct number of bytes were written.
	if (dwBytesWritten != dwCount)
	{
		std::cout << "Wrote " << dwBytesWritten << " bytes instead of requested" << dwCount << " bytes." << std::endl;
		
		return FALSE;
	}
	return TRUE;
}
// End RawDataToPrinter
// **********************************************************************

char *w2c(char *pcstr, const wchar_t *pwstr, size_t len)
{
	int nlength = wcslen(pwstr);
	//获取转换后的长度
	int nbytes = WideCharToMultiByte(0, 0, pwstr, nlength, NULL, 0, NULL, NULL);
	if (nbytes > len)   nbytes = len;
	// 通过以上得到的结果，转换unicode 字符为ascii 字符
	WideCharToMultiByte(0, 0, pwstr, nlength, pcstr, nbytes, NULL, NULL);
	return pcstr;
}
void printers() {
	TCHAR  szOut[1024] = { 0 };
	
	//TEXT("USB004")
	std::string resolutions;
	DWORD result = DeviceCapabilities(TEXT("NantianPR-OLI"), nullptr, DC_ENUMRESOLUTIONS, szOut, nullptr);
	
	int count = 0;
	for (int i = 0; i < 1024; i++) {
		if (szOut[i] == '\0') {
			count++;
			if (count > result * 2) {
				break;
			}
			if ((count % 2) == 0) {
				resolutions.append(";");
			}
			else {
				resolutions.append("x");
			}
			
		}
		else {
			uint64_t* bytes = (uint64_t*)(&szOut[i]);
			long r = *bytes & 0xfffffff;
			resolutions.append(std::to_string(r));
		}
		
	}

	std::cout << resolutions << std::endl;
	std::string cprinter = "NantianPR-OLI";
	HANDLE hprinter;
	BOOL res = OpenPrinter(_bstr_t(cprinter.c_str()), &hprinter, nullptr);
	
	// get printer driver information
	DWORD needed = 0;
	GetPrinter(hprinter, 2, nullptr, 0, &needed);
	std::shared_ptr<PRINTER_INFO_2> prt_info((PRINTER_INFO_2 *) new BYTE[needed]);
	if (prt_info)
		res = GetPrinter(hprinter, 2, (LPBYTE)prt_info.get(), needed, &needed);
	if (!res || !prt_info || needed <= sizeof(PRINTER_INFO_2)) {
		return;
	}


	LONG devsize =
		DocumentProperties(nullptr, hprinter, _bstr_t(cprinter.c_str()), nullptr, /* Asking for size, so */
			nullptr,                                /* not used. */
			0);                                     /* Zero returns buffer size. */
	if (devsize < sizeof(DEVMODE)) {
		// If failure, inform the user, cleanup and return failure.
		return ;
	}
	std::shared_ptr<DEVMODE> devMode((LPDEVMODE)(new BYTE[devsize]));
	if (!devMode) {
		return ;
	}
	// Get the default DevMode for the printer and modify it for your needs.
	LONG returnCode = DocumentProperties(nullptr, hprinter, _bstr_t(cprinter.c_str()),
		devMode.get(),        /* The address of the buffer to fill. */
		nullptr,        /* Not using the input buffer. */
		DM_OUT_BUFFER); /* Have the output buffer filled. */
	if (IDOK != returnCode) {
		// If failure, inform the user, cleanup and return failure.
		return ;
	}
}
// **********************************************************************
// main - entry point for this console application
// 
// Params:
//   argc        - count of command line arguments
//   argv        - array of NULL terminated command line arguments
//
// Returns: 0 for success, non-zero for failure.
// 
// Command line: c:\>RawPrint PrinterName FileName
//               sends raw data file to printer using spooler APIs
//               written nov 1999 jmh
//
int main(int argc, char* argv[])
{
	//printers();
	//return 0;

	LPBYTE	pBytes = NULL;
	DWORD	dwSize = 0;

	if (argc != 3) {
		std::cout << "Syntax: %s <PrinterName> <FileName>" << std::endl;
		return 0;
	}
		
	std::cout << "Attempting to send file [" << argv[2] << "] to printer[" << argv[1] <<"]." << std::endl;

	if (!ReadFileWithAlloc(_bstr_t(argv[2]), &dwSize, &pBytes))
	{
		std::cout << "Failed to allocate memory for and read file" << argv[2] << std::endl;
		return 0;
	}
	

	if (!RawDataToPrinter(_bstr_t(argv[1]), pBytes, dwSize))
		std::cout << "Failed to send data to printer." << std::endl;
	else
		std::cout << "Data sent to printer." << std::endl;

	GlobalFree((HGLOBAL)pBytes);
	return 0;
}
// end main

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
