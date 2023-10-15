//* --------
//  Saturday
//  --------
//
//  Continue in the OnTest() function.  get program
//  to correctly run the commandline and start winamp
//  with the specified MP3.
//
//  after that, all you need to do is modify the StartAlarm()
//  and StopAlarm() functions to replace the generic WAVE-looping
//  with Winamp mp3 playing functionality
//
//  ------
//  Sunday
//  ------
// 
//  keep working on the test function.  possible access violation
//  coming from OnTest() / OnApply();  experiment by removing the
//  HKEY entry and it seems to work fine until the TEST button is
//  pushed during a successful WinExec() call
//
//  if WinExec isn't successful, it doesn't appear to have an
//  access violation
//
//  --------
//  Sunday 2
//  --------
//
//  Access violation fixed.  Was being caused by UpdateDebug() displaying
//  a string larger than what the static text control (IDC_DEBUG) could
//  handle when certain long file/pathnames of mp3's were being displayed
//
//  continue working the OnTest() function.  seems to work properly except
//  for multiple icons appearing in the system tray.  after process is
//  started, find a way to steal the focus from winamp and give it back
//  to the alarm
//
//  ----------
//  for Monday
//  ----------
//
//  bugs/issues/to-do list:
//
//  1) Multiple icons appear in systray
//  2) alarm doesn't know when winamp is shut down manually -- messes up bool alarm/snooze 
//     status vars
//  3) disable options button when alarm goes active so user can't test alarm
//  4) add a status bar to give updates on what alarm is currently doing
//  5) allow user to use a custom snooze_time in the options dialog
//  6) find out how to steal focus from winamp and put it back on the alarm, so snooze/stop can be used
//     from the keyboard without turning on the monitor
//
// ----------------- */




/* --
// INCLUDES
// -- */

#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include "resource.h"

/* --
// NON RESOURCE DEFINITIONS
// -- */

#define IDC_SPIN_HOUR				9000
#define IDC_SPIN_MINUTE			9001
#define IDC_CLOCK_TIMER			9002

/* --
// GLOBAL VARIABLES
// -- */

HWND g_hDlgMain = NULL, g_hDlgOptions = NULL;

HANDLE g_hWinampProcess = NULL;

char g_szStatusText[255];

char g_szWinampPath[MAX_PATH];
char g_szMP3Path[MAX_PATH];

int g_iCurHour, g_iCurMinute, g_iCurSecond;
char g_szCurTOD[3];

int g_iSnoozeHour = 0, g_iSnoozeMinute = 0, g_iSnoozeSecond = 0, g_iSnoozeLength = 0;
char g_szSnoozeTOD[3];

int g_iAlarmHour, g_iAlarmMinute;
char g_szAlarmTOD[3];

BOOL g_bAlarmActive = FALSE;
BOOL g_bSnoozeActive = FALSE;

/* --
// FUNCTION DECLARATIONS
// -- */

BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK OptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main functions

VOID AttachDlgItems(HWND hwnd);
VOID SetDefaults();
BOOL ntos(int iNum, char szStr[], BOOL bPrependZero);
VOID OnTODChange(UINT idFrom);
VOID OnHourChange(int iDelta);
VOID OnMinuteChange(int iDelta);
VOID UpdateDebug();
VOID GetTime();
VOID DisplayTime();
VOID OnClockTimer();
VOID StartAlarm();
VOID StopAlarm();
VOID SaveDefaults();
VOID FirstRun();
VOID OnSnooze();
VOID OnOptions();
VOID OnInitDialog(HWND hwnd);
VOID SetIcon();

// Options functions

VOID ApplyOptions(HWND hwnd);
VOID OnBrowseWinamp(HWND hwnd);
VOID OnBrowseMP3(HWND hwnd);
VOID OnTestMP3(HWND hwnd);
VOID OnOptionsInitDialog(HWND hwnd);

/* --
// FN: WinMain(...) :: Self-explanatory
// -- */

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
		LPSTR lpCmdLine, int nCmdShow)
{

	// Declarations

	MSG Msg;
	INITCOMMONCONTROLSEX icc;
	HFONT hfTime;

	// Initialize common controls

	icc.dwICC			= ICC_UPDOWN_CLASS;
	icc.dwSize		= sizeof(INITCOMMONCONTROLSEX);

	InitCommonControlsEx(&icc);

	// Create the main dialog

	g_hDlgMain = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc);

	if (!g_hDlgMain)
	{
		MessageBox(NULL, "Unable to create main dialog!", "Error!",
			MB_ICONERROR | MB_OK);
		return (0);
	}

	// Set the dialog icon

	SetIcon();
	
	// Attach dialog items (spin controls, etc);

	AttachDlgItems(g_hDlgMain);

	// Set default alarm values
	
	SetDefaults();

	// Set the default clock font

	hfTime = CreateFont(8, 8, 0, 0, 500, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, "Fixedsys"
	);
	
	SendDlgItemMessage(g_hDlgMain, IDC_CURTIME, WM_SETFONT, (WPARAM)hfTime, 
		(LPARAM)MAKELPARAM(FALSE, 0)
	);

	// Initialize the clock

	GetTime();
	DisplayTime();

	// Begin clock timer

	SetTimer(g_hDlgMain, IDC_CLOCK_TIMER, 500, NULL);

	// Main message loop

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		if (!IsDialogMessage(g_hDlgMain, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	// Exit

	return Msg.wParam;
}

/* --
// PROC: Main dialog message handler
// -- */

BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			OnInitDialog(hwnd);	
		return TRUE;

		case WM_CLOSE:
			SaveDefaults();
			PostQuitMessage(0);
		break;

		case WM_TIMER:
		{
			switch (wParam)
			{
				case IDC_CLOCK_TIMER:
					OnClockTimer();
				break;
			}
		}
		break;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_AM:
					OnTODChange(LOWORD(wParam));
				break;

				case IDC_RADIO_PM:
					OnTODChange(LOWORD(wParam));			
				break;

				case IDC_STOPALARM:
					StopAlarm();
				break;

				case IDC_SNOOZE:
					OnSnooze();
				break;

				case IDC_OPTIONS:
					OnOptions();
				break;
			}
		}
		break;

		case WM_NOTIFY:
		{
			NMHDR *pNMHDR = (NMHDR *)lParam;
			switch (pNMHDR->idFrom)
			{
				case IDC_SPIN_HOUR:
				{
					NMUPDOWN *pNMUD = (NMUPDOWN *)lParam;
					OnHourChange(pNMUD->iDelta);
				}
				break;

				case IDC_SPIN_MINUTE:
				{
					NMUPDOWN *pNMUD = (NMUPDOWN *)lParam;
					OnMinuteChange(pNMUD->iDelta);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
			DestroyWindow(hwnd);
		break;

		default:
			return FALSE;
	}
	return TRUE;
}

/* --
// FN: OnInitDialog() :: ...
// -- */

VOID OnInitDialog(HWND hwnd)
{

	// Deactivate buttons that can't be
	// used in alarm's current state

	EnableWindow(GetDlgItem(hwnd, IDC_STOPALARM), FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_SNOOZE), FALSE);

}

/* --
// FN: SetIcon() :: Sets icon to main dialog
// -- */

VOID SetIcon()
{

	// Declarations

	HICON hIcon, hIconSm;

	// Load the icons

	hIcon		= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
	hIconSm	= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));

	// Set the icons

	SendMessage(g_hDlgMain, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hIcon);
	SendMessage(g_hDlgMain, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIconSm);

}

/* --
// FN: AttachDlgItems() :: Attaches various dialog items
// -- */

VOID AttachDlgItems(HWND hwnd)
{

	// Declarations

	HWND hSpinHour, hSpinMinute;
	HWND hEditHour, hEditMinute;

	// Get required window handles

	hEditHour		= GetDlgItem(hwnd, IDC_EDIT_HOUR);
	hEditMinute	= GetDlgItem(hwnd, IDC_EDIT_MINUTE);

	// Create the hour's spin control

	hSpinHour = CreateUpDownControl(
		WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
		0, 0, 2, 2, hwnd, IDC_SPIN_HOUR, GetModuleHandle(NULL), hEditHour,
		12, 1, 1
	);

	if (!hSpinHour)
	{
		MessageBox(hwnd, "Unable to create IDC_SPIN_HOUR", "Warning!",
			MB_ICONWARNING | MB_OK);
	}

	// Create the minute's spin control

	hSpinMinute = CreateUpDownControl(
		WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
		0, 0, 2, 2, hwnd, IDC_SPIN_MINUTE, GetModuleHandle(NULL), hEditMinute,
		59, 0, 1
	);

	if (!hSpinMinute)
	{
		MessageBox(hwnd, "Unable to create IDC_SPIN_MINUTE", "Warning!",
			MB_ICONWARNING | MB_OK);
	}

}

/* --
// FN: SetDefaults() :: Sets the default values
// -- */

VOID SetDefaults()
{

	// Declarations

	HKEY hkDefaults;
	char szHour[3], szMinute[3], szTOD[3], szSnoozeLength[3];
	char szWinampPath[MAX_PATH], szMP3Path[MAX_PATH];
	LPDWORD dwHour, dwMinute, dwTOD, dwSnoozeLength, dwWinPath, dwMP3Path;
	LPDWORD lpDisp;
	
	// Open a handle to the reg key
	// where defaults are stored

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\WinAlarm", 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkDefaults, (LPDWORD)&lpDisp
	);

	// If the registry key was created as opposed to
	// opened, this is the first time the prorgam is being
	// run; do FirstRun()

	if (lpDisp == (LPDWORD)REG_CREATED_NEW_KEY)
	{
		RegCloseKey(hkDefaults);
		FirstRun();
		return;
	}

	// Get stored registry data into buffers

	dwHour = (LPDWORD)sizeof(szHour);
	RegQueryValueEx(hkDefaults, "szHour", NULL, NULL, szHour, (LPDWORD)&dwHour); 

	dwMinute = (LPDWORD)sizeof(szMinute);
	RegQueryValueEx(hkDefaults, "szMinute", NULL, NULL, szMinute, (LPDWORD)&dwMinute);
	
	dwTOD = (LPDWORD)sizeof(szTOD);
	RegQueryValueEx(hkDefaults, "szTOD", NULL, NULL, szTOD, (LPDWORD)&dwTOD);
	
	dwSnoozeLength = (LPDWORD)sizeof(szSnoozeLength);
	RegQueryValueEx(hkDefaults, "szSnoozeLength", NULL, NULL, szSnoozeLength,
		(LPDWORD)&dwSnoozeLength
	);

	dwWinPath = (LPDWORD)sizeof(szWinampPath);
	RegQueryValueEx(hkDefaults, "szWinampPath", NULL, NULL, szWinampPath, 
		(LPDWORD)&dwWinPath
	);

	dwMP3Path = (LPDWORD)sizeof(szMP3Path);
	RegQueryValueEx(hkDefaults, "szMP3Path", NULL, NULL, szMP3Path, (LPDWORD)&dwMP3Path);

	RegCloseKey(hkDefaults);

	// Make retrieved registry values global

	g_iAlarmHour		= atoi(szHour);
	g_iAlarmMinute	= atoi(szMinute);
	g_iSnoozeLength = atoi(szSnoozeLength);
	strcpy(g_szAlarmTOD, szTOD);
	strcpy(g_szWinampPath, szWinampPath);
	strcpy(g_szMP3Path, szMP3Path);

	// Update alarm display with currently
	// set values

	SetDlgItemText(g_hDlgMain, IDC_EDIT_HOUR, szHour);
	SetDlgItemText(g_hDlgMain, IDC_EDIT_MINUTE, szMinute);

	if (strcmp(g_szAlarmTOD, "AM") == 0)
		SendDlgItemMessage(g_hDlgMain, IDC_RADIO_AM, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
	else if (strcmp(g_szAlarmTOD, "PM") == 0)
		SendDlgItemMessage(g_hDlgMain, IDC_RADIO_PM, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

}

/* --
// FN: ntos() :: Converts an int to a string
// -- */

BOOL ntos(int iNum, char szStr[], BOOL bPrependZero)
{

	// If number is not within a valid
	// time range, don't handle it

	if (iNum < 0 || iNum > 59)
		return FALSE;

	// If prepended zero isn't required,
	// do a straight conversion

	if (!bPrependZero)
	{
		sprintf(szStr, "%i", iNum);
		return TRUE;
	}

	// If prepended zero is required,
	// add it only if the number is less
	// than 10

	if (bPrependZero)
	{
		if (iNum < 10)
			sprintf(szStr, "%02d", iNum);
		else
			sprintf(szStr, "%i", iNum);
	}

	return TRUE;

}

/* --
// FN: OnTODChange() :: Activated when user changes TOD alarm is set for
// -- */

VOID OnTODChange(UINT idFrom)
{

	// If TOD was changed to AM,
	// set global TOD variable to AM

	if (idFrom == IDC_RADIO_AM)
		strcpy(g_szAlarmTOD, "AM");

	// If it was PM, change it to PM

	else if (idFrom == IDC_RADIO_PM)
		strcpy(g_szAlarmTOD, "PM");

	// DEBUG DEBUG DEBUG DEBUG DEBUG

	UpdateDebug();
}


/* --
// FN: OnHourChange() :: Activated when user changes hour alarm is set for
// -- */

VOID OnHourChange(int iDelta)
{

	// Declarations

	char szCurHour[3];
	int iCurHour;

	// Get the current hour value
	// and convert to int

	GetDlgItemText(g_hDlgMain, IDC_EDIT_HOUR, szCurHour, sizeof(szCurHour));
	iCurHour = atoi(szCurHour);

	// Change hour up/down depending on
	// delta (direction spin was pushed)

	if (iDelta > 0)
		iCurHour++;
	else if (iDelta < 0)
		iCurHour--;

	// If proposed new hour goes outside of
	// the valid hour range (1-12), do nothing

	if (iCurHour < 1 || iCurHour > 12)
		return;

	// Assign new alarm hour to the global
	// variable and update the dialog display
	// with the new value

	g_iAlarmHour = iCurHour;
	ntos(g_iAlarmHour, szCurHour, FALSE);
	SetDlgItemText(g_hDlgMain, IDC_EDIT_HOUR, szCurHour);

	// DEBUG DEBUG DEBUG DEBUG DEBUG

	UpdateDebug();

}


/* --
// FN: OnMinuteChange() :: Activated when user changes minute alarm is set for
// -- */

VOID OnMinuteChange(int iDelta)
{

	// Declarations

	char szCurMinute[3];
	int iCurMinute;

	// Get current minute value and
	// convert it to an integer

	GetDlgItemText(g_hDlgMain, IDC_EDIT_MINUTE, szCurMinute, sizeof(szCurMinute));
	iCurMinute = atoi(szCurMinute);

	// Change minute value depending on
	// delta (direction spin was pushed)

	if (iDelta > 0)
		iCurMinute++;
	else if (iDelta < 0)
		iCurMinute--;

	// If proposed new minute leaves the
	// range of a valid minute (0-59),
	// do nothing

	if (iCurMinute > 59 || iCurMinute < 0)
		return;

	// Assign new minute to global variable
	// and update dialog display with it

	g_iAlarmMinute = iCurMinute;
	ntos(g_iAlarmMinute, szCurMinute, TRUE);
	SetDlgItemText(g_hDlgMain, IDC_EDIT_MINUTE, szCurMinute);

	// DEBUG DEBUG DEBUG DEBUG DEBUG

	UpdateDebug();

}

/* --
// FN: DEBUG DEBUG DEBUG DEBUG DEBUG
// -- */

VOID UpdateDebug()
{

	// Declarations

	char szAlarmStatus[6];
	char szSnoozeStatus[6];
	char szString[255];
	char szString2[255];

	// Determine alarm status

	if (g_bAlarmActive)
		strcpy(szAlarmStatus, "TRUE");
	else if (!g_bAlarmActive)
		strcpy(szAlarmStatus, "FALSE");

	// Determine snooze status

	if (g_bSnoozeActive)
		strcpy(szSnoozeStatus, "TRUE");
	else if (!g_bSnoozeActive)
		strcpy(szSnoozeStatus, "FALSE");

	// Format and display the debug string

	sprintf(szString,
		"Cur: H: %i | M: %i | S: %i | T: %s \n Alarm: H: %i | M: %i | T: %s \n Snooze: H: %i | M: %i | S: %i | L: %i | T: %s \n AlarmActive: %s | SnoozeActive: %s",
		 g_iCurHour, g_iCurMinute, g_iCurSecond, g_szCurTOD, g_iAlarmHour, g_iAlarmMinute, g_szAlarmTOD,
		 g_iSnoozeHour, g_iSnoozeMinute, g_iSnoozeSecond, g_iSnoozeLength, g_szSnoozeTOD,
		 szAlarmStatus, szSnoozeStatus
	);

	sprintf(szString2, "Winamp: %s \nMP3: %s", g_szWinampPath, g_szMP3Path);
	
	SetDlgItemText(g_hDlgMain, IDC_DEBUG, szString);
	SetDlgItemText(g_hDlgMain, IDC_DEBUG2, szString2);

}

/* --
// FN: GetTime() :: Assigns current time to global variables
// -- */

VOID GetTime()
{

	// Declarations

	int iHour, iMinute, iSecond;
	SYSTEMTIME sysTime;

	// Get current time

	GetLocalTime(&sysTime);
	
	iHour		= ((int)sysTime.wHour);
	iMinute	= ((int)sysTime.wMinute);
	iSecond = ((int)sysTime.wSecond);

	// Determine current TOD

	if (iHour >= 12)
		strcpy(g_szCurTOD, "PM");
	else if (iHour < 12)
		strcpy(g_szCurTOD, "AM");

	// Determine the time (in 12 hours)

	if (iHour >= 13)
		g_iCurHour = (iHour - 12);
	else if (iHour == 0)
		g_iCurHour = 12;
	else
		g_iCurHour = iHour;

	// Assign current min and sec to global

	g_iCurMinute = iMinute;
	g_iCurSecond = iSecond;

	// DEBUG DEBUG DEBUG DEBUG DEBUG

	UpdateDebug();

}

/* --
// FN: DisplayTime() :: Updates time display with current time
// -- */

VOID DisplayTime()
{

	// Declarations

	char szMinute[3];
	char szTimeString[12];

	// Convert minute to 0-padded string 
	// (hour doesn't require padding)

	ntos(g_iCurMinute, szMinute, TRUE);

	// Format the timestring

	sprintf(szTimeString, "%i:%s %s",
		g_iCurHour, szMinute, g_szCurTOD
	);

	// Update dialog time display with timestring

	SetDlgItemText(g_hDlgMain, IDC_CURTIME, szTimeString);

}

/* --
// FN: OnClockTimer() :: Called when timer expires
// -- */

VOID OnClockTimer()
{

	// Get the current time and
	// display it

	GetTime();
	DisplayTime();

	// If the alarm is not already active
	// and it's time to activate

	if ( (!g_bAlarmActive) && (g_iCurHour == g_iAlarmHour) && (g_iCurMinute == g_iAlarmMinute) 
		&& (g_iCurSecond == 0) && (strcmp(g_szCurTOD, g_szAlarmTOD)==0) )
	{
		StartAlarm();
	}

	// If the alarm is not already active
	// and it's time for the snooze alarm
	// and the snooze feature is active

	if ( (!g_bAlarmActive) && (g_bSnoozeActive) && (g_iSnoozeHour == g_iCurHour) 
		&& (g_iSnoozeMinute == g_iCurMinute) && (g_iSnoozeSecond == g_iCurSecond) 
		&& (strcmp(g_szSnoozeTOD, g_szCurTOD) == 0) )
	{
		StartAlarm();
	}

}

/* --
// FN: StartAlarm() :: Begins playback of alarm loop
// -- */

VOID StartAlarm()
{

	// Declarations

	char szShortPath[MAX_PATH];
	char szShortMP3[MAX_PATH];
	char szCmdLine[MAX_PATH*2];

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Configure structs

	ZeroMemory( &si, sizeof(si) );
	ZeroMemory( &pi, sizeof(pi) );
	si.cb	= sizeof(si);

	// Activate buttons that can now be used

	EnableWindow(GetDlgItem(g_hDlgMain, IDC_STOPALARM), TRUE);
	EnableWindow(GetDlgItem(g_hDlgMain, IDC_SNOOZE), TRUE);
	
	// Set alarm status to active

	g_bAlarmActive = TRUE;

	// Convert Winamp and MP3 path to short

	GetShortPathName(g_szWinampPath, szShortPath, (DWORD)sizeof(szShortPath));
	GetShortPathName(g_szMP3Path, szShortMP3, (DWORD)sizeof(szShortMP3));

	// Format the commandline and start winamp
	// as long as the winamp process isn't currently
	// running

	sprintf(szCmdLine, "%s %s", szShortPath, szShortMP3);
	CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	g_hWinampProcess = pi.hProcess;

	// Set the alarm window to the foreground
	// and apply focus to snooze button

	SetActiveWindow(g_hDlgMain);
	SetFocus(GetDlgItem(g_hDlgMain, IDC_SNOOZE));

	// DEBUG DEBUG DEBUG

	UpdateDebug();

}

/* --
// FN: StopAlarm() :: Deactivates the alarm and stops looping
// -- */

VOID StopAlarm()
{

	
	// Set alarm status to inactive

	g_bAlarmActive	= FALSE;
	g_bSnoozeActive	= FALSE;

	// Disable buttons that can't be used
	// in application's current state

	EnableWindow(GetDlgItem(g_hDlgMain, IDC_STOPALARM), FALSE);
	EnableWindow(GetDlgItem(g_hDlgMain, IDC_SNOOZE), FALSE);

	// Reset snooze values

	g_iSnoozeHour			= 0;
	g_iSnoozeMinute		= 0;
	g_iSnoozeSecond		= 0;

	// Shutdown WinAmp

	TerminateProcess(g_hWinampProcess, 0);

	// DEBUG DEBUG DEBUG

	UpdateDebug();

}

/* --
// FN: SaveDefaults() :: Saves last-used alarm values in the registry
// -- */

VOID SaveDefaults()
{

	// Declarations

	HKEY hkDefaults;
	char szAlarmHour[3], szAlarmMinute[3], szAlarmTOD[3], szSnoozeLength[3];
	char szWinampPath[MAX_PATH], szMP3Path[MAX_PATH];

	// Store current values in buffers

	GetDlgItemText(g_hDlgMain, IDC_EDIT_HOUR, szAlarmHour, sizeof(szAlarmHour));
	GetDlgItemText(g_hDlgMain, IDC_EDIT_MINUTE, szAlarmMinute, sizeof(szAlarmMinute));
	ntos(g_iSnoozeLength, szSnoozeLength, FALSE);
	strcpy(szAlarmTOD, g_szAlarmTOD);
	strcpy(szWinampPath, g_szWinampPath);
	strcpy(szMP3Path, g_szMP3Path);

	// Open (or Create) a registry key to
	// store last-used values in

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\WinAlarm", 0, "",
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkDefaults, NULL
	);

	// Write the hour, minute and TOD to the
	// registry (also write the Winamp path and
	// mp3 filename)

	RegSetValueEx(hkDefaults, "szHour", 0, REG_SZ, szAlarmHour, sizeof(szAlarmHour));
	RegSetValueEx(hkDefaults, "szMinute", 0, REG_SZ, szAlarmMinute, sizeof(szAlarmMinute));
	RegSetValueEx(hkDefaults, "szTOD", 0, REG_SZ, szAlarmTOD, sizeof(szAlarmTOD));
	RegSetValueEx(hkDefaults, "szSnoozeLength", 0, REG_SZ, szSnoozeLength, sizeof(szSnoozeLength));
	RegSetValueEx(hkDefaults, "szWinampPath", 0, REG_SZ, szWinampPath, sizeof(szWinampPath));
	RegSetValueEx(hkDefaults, "szMP3Path", 0, REG_SZ, szMP3Path, sizeof(szMP3Path));

	// Close the registry key

	RegCloseKey(hkDefaults);

}

/* --
// FN: FirstRun() :: Called when prog. is run for the first time
// -- */

VOID FirstRun()
{
	
	// When program is run for the first time,
	// there are no default values in the registry
	// set default values

	int DEFAULT_HOUR = 6;
	int DEFAULT_MINUTE = 30;
	char DEFAULT_TOD[] = "AM";
	int DEFAULT_SNOOZE = 10;

	char szHour[3], szMinute[3];

	char szWinampPath[MAX_PATH] = "Put your WINAMP.EXE here";
	char szMP3Path[MAX_PATH] = "Put your .MP3 file here";
	
	// Make default values global

	g_iAlarmHour		= DEFAULT_HOUR;
	g_iAlarmMinute	= DEFAULT_MINUTE;
	g_iSnoozeLength = DEFAULT_SNOOZE;
	strcpy(g_szAlarmTOD, DEFAULT_TOD);
	strcpy(g_szWinampPath, szWinampPath);
	strcpy(g_szMP3Path, szMP3Path);

	// Convert default int values to strings

	ntos(g_iAlarmHour, szHour, FALSE);
	ntos(g_iAlarmMinute, szMinute, TRUE);

	// Update dialog display

	SetDlgItemText(g_hDlgMain, IDC_EDIT_HOUR, szHour);
	SetDlgItemText(g_hDlgMain, IDC_EDIT_MINUTE, szMinute);

	if (strcmp(g_szAlarmTOD, "AM") == 0)
		SendDlgItemMessage(g_hDlgMain, IDC_RADIO_AM, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
	else if (strcmp(g_szAlarmTOD, "PM") == 0)
		SendDlgItemMessage(g_hDlgMain, IDC_RADIO_PM, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

	// DEBUG DEBUG DEBUG DEBUG

	MessageBox(g_hDlgMain,
		"Before the program will work, you need to configure it in the options menu.",
		"First Run",
		MB_ICONINFORMATION | MB_OK
	);

	//UpdateDebug();

}

/* --
// FN: OnSnooze() :: Called when user pushes snooze button
// -- */

VOID OnSnooze()
{

	// Declarations

	int iHour, iMinute, iSecond;
	char szTOD[3];
	
	// Stop the alarm if it's currently
	// playing

	StopAlarm();

	// Enable/disable buttons that can/can;t
	// be used in application's current state

	EnableWindow(GetDlgItem(g_hDlgMain, IDC_SNOOZE), FALSE);
	EnableWindow(GetDlgItem(g_hDlgMain, IDC_STOPALARM), TRUE);

	// Set snooze to active

	g_bSnoozeActive = TRUE;

	// Set hour & minute g_iSnoozeLength
	// minutes from now
	
	if (g_iCurMinute + g_iSnoozeLength < 59)
	{
		iHour		= g_iCurHour;
		iMinute	= g_iCurMinute + g_iSnoozeLength;
	}

	else if (g_iCurMinute + g_iSnoozeLength > 59)
	{
		iHour		= g_iCurHour + 1;
		iMinute	= g_iCurMinute + g_iSnoozeLength - 60;
	}

	// Make sure incremented hour
	// remains a 12 hour clock

	if (iHour > 12)
		iHour = iHour-12;


	// Set second

	iSecond		= g_iCurSecond;

	// Set snooze TOD

	if (g_iCurHour == 11 && iHour == 12)
	{
		if (strcmp(g_szCurTOD, "AM") == 0)
			strcpy(szTOD, "PM");
		else if (strcmp(g_szCurTOD, "PM") == 0)
			strcpy(szTOD, "AM");
	}
	else
		strcpy(szTOD, g_szCurTOD);

	// Set snooze time to global snooze vars

	g_iSnoozeHour		= iHour;
	g_iSnoozeMinute	= iMinute;
	g_iSnoozeSecond	= iSecond;
	strcpy(g_szSnoozeTOD, szTOD);

	// DEBUG DEBUG DEBUG DEBUG

	UpdateDebug();

}

/* --
// PROC: OptionsProc() :: Handles messages for the options dialog
// -- */

BOOL CALLBACK OptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			OnOptionsInitDialog(hwnd);
		return TRUE;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_OPTIONS_APPLY:
					ApplyOptions(hwnd);
				break;

				case IDC_OPTIONS_CANCEL:
					EndDialog(hwnd, IDC_OPTIONS_CANCEL);
				break;

				case IDC_OPTIONS_TEST:
					OnTestMP3(hwnd);
				break;

				case IDC_OPTIONS_BROWSEMP3:
					OnBrowseMP3(hwnd);
				break;

				case IDC_OPTIONS_BROWSEWINAMP:
					OnBrowseWinamp(hwnd);
				break;
			}
		}
		break;

		case WM_CLOSE:
			EndDialog(hwnd, WM_CLOSE);
		break;

		default:
			return FALSE;
	}

	return TRUE;
}

/* --
// FN: ApplyOptions() :: Applies options set in the options dialog
// -- */

VOID ApplyOptions(HWND hwnd)
{

	// Declarations

	char szWinampPath[MAX_PATH];
	char szMP3Path[MAX_PATH];
	int iSnoozeLength;

	HKEY hkDefaults;

	// Store data in buffers

	GetDlgItemText(hwnd, IDC_OPTIONS_EDIT_WINAMPPATH, szWinampPath, sizeof(szWinampPath));
	GetDlgItemText(hwnd, IDC_OPTIONS_EDIT_MP3PATH, szMP3Path, sizeof(szMP3Path));
	iSnoozeLength = GetDlgItemInt(hwnd, IDC_OPTIONS_EDIT_SNOOZE, NULL, FALSE);

	// Make sure length of snooze does not exceed
	// 59 minutes

	if (iSnoozeLength > 59)
	{
		MessageBox(g_hDlgMain, "You may not set the snooze longer than 59 minutes.",
			"Warning!", MB_ICONINFORMATION | MB_OK);
		SetFocus(GetDlgItem(hwnd, IDC_OPTIONS_EDIT_SNOOZE));
		return;
	}

	// Make new values global (SHORT)

	strcpy(g_szWinampPath, szWinampPath);
	strcpy(g_szMP3Path, szMP3Path);
	g_iSnoozeLength = iSnoozeLength;

	// Write data to registry

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\WinAlarm", 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkDefaults, NULL
	);

	//RegSetValueEx(hkDefaults, "szWinampPath", 0, REG_SZ, g_szWinampPath, sizeof(g_szMP3Path));
	//RegSetValueEx(hkDefaults, "szMP3Path", 0, REG_SZ, g_szMP3Path, sizeof(g_szMP3Path));

	RegCloseKey(hkDefaults);

	// Close the dialog

	EndDialog(hwnd, IDC_OPTIONS_APPLY);

}

/* --
// FN: OnOptions() :: Activated when the options button is clicked
// -- */

VOID OnOptions()
{

	// Create and display the
	// options dialog

	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OPTIONS),
		g_hDlgMain, OptionsProc
	);

}

/* --
// FN: OnBrowseMP3() :: Called when user clicks Browse for .MP3 button
// -- */

VOID OnBrowseMP3(HWND hwnd)
{

	// Declarations

	OPENFILENAME ofn;
	char szFilename[MAX_PATH] = "";

	// Initialize the OFN struct

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize					= sizeof(ofn);
	ofn.Flags								= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOLONGNAMES;
	ofn.hwndOwner						= hwnd;
	ofn.lpstrDefExt					= "mp3";
	ofn.lpstrFile						= szFilename;
	ofn.lpstrFilter					= "MPEG Audio Layer 3 (.mp3)\0*.mp3\0";
	ofn.nMaxFile						= MAX_PATH;

	// Get and display the filename

	if (GetOpenFileName(&ofn))
		SetDlgItemText(hwnd, IDC_OPTIONS_EDIT_MP3PATH, szFilename);

}

/* --
// FN: OnBrowseWinamp() :: Called when user clicks Browse for WINAMP.EXE button
// -- */

VOID OnBrowseWinamp(HWND hwnd)
{

	// Declarations

	OPENFILENAME ofn;
	char szFilename[MAX_PATH] = "";

	// Initialize and configure the
	// OFN structure

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.Flags								= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOLONGNAMES;
	ofn.hwndOwner						= hwnd;
	ofn.lpstrDefExt					= "exe";
	ofn.lpstrFile						= szFilename;
	ofn.lpstrFilter					= "Executables (.exe)\0*.exe";
	ofn.lStructSize					= sizeof(ofn);
	ofn.nMaxFile						= MAX_PATH;

	// Get and display the selected file

	if (GetOpenFileName(&ofn))
		SetDlgItemText(hwnd, IDC_OPTIONS_EDIT_WINAMPPATH, szFilename);
}

/* --
// FN: OnTestMP3() :: Called when user hits 'TEST MP3' button
// -- */

VOID OnTestMP3(HWND hwnd)
{

	// Declarations

	char szShortPath[MAX_PATH];
	char szShortMP3[MAX_PATH];
	char szLongPath[MAX_PATH];
	char szLongMP3[MAX_PATH];
	char szCmdLine[MAX_PATH*2];

	char szButtonLabel[12];

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// Set up the structures

	ZeroMemory( &si, sizeof(si) );
	ZeroMemory( &pi, sizeof(pi) );
	si.cb	= sizeof(si);
	
	// Get the label from the test button

	GetDlgItemText(hwnd, IDC_OPTIONS_TEST, szButtonLabel, sizeof(szButtonLabel));

	// If test is currently in progress (denoted
	// by the button being labbed "Stop"), switch
	// button back to default state and stop the test

	if (strcmp(szButtonLabel, "&Stop >>") == 0)
	{
		SetDlgItemText(hwnd, IDC_OPTIONS_TEST, "&Test >>");
		TerminateProcess(g_hWinampProcess, 0);
		g_hWinampProcess = NULL;
		return;
	}

	// If test is not in progress, switch button
	// label to STOP and begin the test

	else if (strcmp(szButtonLabel, "&Test >>") == 0)
		SetDlgItemText(hwnd, IDC_OPTIONS_TEST, "&Stop >>");


	// Buffer Path/MP3 values, convert them
	// to short filename and format the command line

	GetDlgItemText(hwnd, IDC_OPTIONS_EDIT_WINAMPPATH, szLongPath, MAX_PATH);
	GetDlgItemText(hwnd, IDC_OPTIONS_EDIT_MP3PATH, szLongMP3, MAX_PATH);

	GetShortPathName(szLongPath, szShortPath, (DWORD)MAX_PATH);
	GetShortPathName(szLongMP3, szShortMP3, (DWORD)MAX_PATH);

	sprintf(szCmdLine, "%s %s", szShortPath, szShortMP3);

	// Run winamp with specified MP3 &
	// assign new process ID to global var

	if (!CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL,
		NULL, &si, &pi))
	{
		MessageBox(g_hDlgMain, "Unable to start winamp.exe!  Make sure you have it properly set in the options menu.",
			"Error!", MB_ICONERROR | MB_OK);
		return;	
	}

	g_hWinampProcess = pi.hProcess;

}

/* --
// FN: OnOptionsInitDialog() :: Called when options dialog is initialized
// -- */

VOID OnOptionsInitDialog(HWND hwnd)
{

	// Declarations

	RECT rcMainWindow;

	// Get coords of main dialog

	GetWindowRect(g_hDlgMain, &rcMainWindow);

	// Set maximum value for snooze

	SendDlgItemMessage(hwnd, IDC_OPTIONS_EDIT_SNOOZE, EM_SETLIMITTEXT, (WPARAM)2, (LPARAM)0);
	
	// Set the global winamp/mp3 values
	// into the dialog

	SetDlgItemText(hwnd, IDC_OPTIONS_EDIT_WINAMPPATH, g_szWinampPath);
	SetDlgItemText(hwnd, IDC_OPTIONS_EDIT_MP3PATH, g_szMP3Path);
	SetDlgItemInt(hwnd, IDC_OPTIONS_EDIT_SNOOZE, g_iSnoozeLength, TRUE);

	// Set the position of the options
	// dialog to attach itself to the
	// bottom of the main dialog

	SetWindowPos(hwnd, HWND_TOP, rcMainWindow.left, rcMainWindow.top, 0, 0,
		SWP_NOSIZE | SWP_SHOWWINDOW
	);

}