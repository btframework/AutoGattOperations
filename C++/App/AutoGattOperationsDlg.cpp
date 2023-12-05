
// AutoGattOperationsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AutoGattOperations.h"
#include "AutoGattOperationsDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString IntToHex(const int i)
{
	CString s;
	s.Format(_T("%.8X"), i);
	return s;
}

CString IntToHex(const unsigned short i)
{
	CString s;
	s.Format(_T("%.4X"), i);
	return s;
}

CString IntToHex(const unsigned char i)
{
	CString s;
	s.Format(_T("%.2X"), i);
	return s;
}

CString IntToHex(const __int64 i)
{
	CString s;
	s.Format(_T("%.4X%.8X"), static_cast<INT32>((i >> 32) & 0x00000FFFF),
		static_cast<INT32>(i) & 0xFFFFFFFF);
	return s;
}

CString GUIDToString(const GUID& i)
{
	LPOLESTR Guid;
	StringFromCLSID(i, &Guid);
	CString r(Guid);
	CoTaskMemFree(Guid);
	return r;
}


// CAutoGattOperationsDlg dialog



CAutoGattOperationsDlg::CAutoGattOperationsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_AUTOGATTOPERATIONS_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAutoGattOperationsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_START, btStart);
	DDX_Control(pDX, IDC_BUTTON_DISCONNECT, btDisconnect);
	DDX_Control(pDX, IDC_LIST_LOG, lbLog);
}

BEGIN_MESSAGE_MAP(CAutoGattOperationsDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CAutoGattOperationsDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CAutoGattOperationsDlg::OnBnClickedButtonDisconnect)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CAutoGattOperationsDlg message handlers

BOOL CAutoGattOperationsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	__hook(&CwclBluetoothManager::AfterOpen, &FManager, &CAutoGattOperationsDlg::ManagerAfterOpen);
	__hook(&CwclBluetoothManager::BeforeClose, &FManager, &CAutoGattOperationsDlg::ManagerBeforeClose);
	__hook(&CwclBluetoothManager::OnDiscoveringStarted, &FManager, &CAutoGattOperationsDlg::ManagerDiscoveringStarted);
	__hook(&CwclBluetoothManager::OnDeviceFound, &FManager, &CAutoGattOperationsDlg::ManagerDeviceFound);
	__hook(&CwclBluetoothManager::OnDiscoveringCompleted, &FManager, &CAutoGattOperationsDlg::ManagerDiscoveringCompleted);

	__hook(&CwclGattClient::OnDisconnect, &FClient, &CAutoGattOperationsDlg::ClientDisconnect);
	__hook(&CwclGattClient::OnConnect, &FClient, &CAutoGattOperationsDlg::ClientConnect);
	__hook(&CwclGattClient::OnCharacteristicChanged, &FClient, &CAutoGattOperationsDlg::ClientCharacteristicChanged);

	btDisconnect.EnableWindow(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAutoGattOperationsDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAutoGattOperationsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CAutoGattOperationsDlg::Trace(CString Msg)
{
	lbLog.AddString(Msg);
}

CString CAutoGattOperationsDlg::ShowUuid(wclGattUuid Uuid)
{
	if (Uuid.IsShortUuid)
		return IntToHex(Uuid.ShortUuid);
	return GUIDToString(Uuid.LongUuid);
}

void CAutoGattOperationsDlg::CloseBluetoothManager()
{
	Trace(_T("Closing Bluetooth Manager"));
	FManager.Close();
}

void CAutoGattOperationsDlg::Disconnect()
{
	if (btDisconnect.IsWindowEnabled())
	{
		Trace(_T("Unsubscribe from all characteristics"));

		for (list<wclGattCharacteristic>::iterator Char = FSubscribed.begin(); Char != FSubscribed.end(); Char++)
		{
			FClient.WriteClientConfiguration(*Char, false, goNone, plNone);
			FClient.Unsubscribe(*Char);
		}
		FSubscribed.clear();

		Trace(_T("Disconnecting"));
		FClient.Disconnect();
	}
}

void CAutoGattOperationsDlg::ClientCharacteristicChanged(void* Sender, const unsigned short Handle,
	const unsigned char* Value, const unsigned long Length)
{
	Trace(_T("Notification received: "));
	CString s;
	for (unsigned long i = 0; i < Length; i++)
		s = s + IntToHex(Value[i]);
	Trace(_T("  ") + s);
}

void CAutoGattOperationsDlg::ClientConnect(void* Sender, const int Error)
{
	if (Error != WCL_E_SUCCESS)
	{
		Trace(_T("Connection failed: 0x") + IntToHex(Error));
		CloseBluetoothManager();
	}
	else
	{
		FSubscribed.clear();

		Trace(_T("Connected with success"));

		Trace(_T("Read services"));
		wclGattServices Services;
		int Res = FClient.ReadServices(goNone, Services);
		if (Res != WCL_E_SUCCESS)
			Trace(_T("Read services failed: 0x") + IntToHex(Res));
		else
		{
			for (wclGattServices::iterator Service = Services.begin(); Service != Services.end(); Service++)
			{
				Trace(_T("Service ") + ShowUuid((*Service).Uuid));
				wclGattCharacteristics Chars;
				Res = FClient.ReadCharacteristics(*Service, goNone, Chars);
				if (Res != WCL_E_SUCCESS)
					Trace(_T("  Unable to read characteristics: 0x") + IntToHex(Res));
				else
				{
					for (wclGattCharacteristics::iterator Char = Chars.begin(); Char != Chars.end(); Char++)
					{
						Trace(_T("  Characteristic ") + ShowUuid((*Char).Uuid));
						if ((*Char).IsReadable)
							Trace(_T("    Readable"));
						if ((*Char).IsWritable || (*Char).IsSignedWritable || (*Char).IsWritableWithoutResponse)
							Trace(_T("    Writable"));
						if ((*Char).IsIndicatable || (*Char).IsNotifiable)
						{
							Trace(_T("    Notifiable"));
							Trace(_T("    Subscribe"));
							Res = FClient.Subscribe(*Char);
							if (Res != WCL_E_SUCCESS)
								Trace(_T("      Subscribed failed: 0x") + IntToHex(Res));
							else
							{
								Res = FClient.WriteClientConfiguration(*Char, true, goNone, plNone);
								if (Res != WCL_E_SUCCESS)
								{
									Trace(_T("      Write CCD failed: 0x") + IntToHex(Res));
									FClient.Unsubscribe(*Char);
								}
								else
								{
									Trace(_T("      Subscribed"));
									FSubscribed.push_back(*Char);
								}
							}
						}
					}
				}
			}
		}
		btDisconnect.EnableWindow(TRUE);
	}
}

void CAutoGattOperationsDlg::ClientDisconnect(void* Sender, const int Reason)
{
	btDisconnect.EnableWindow(FALSE);

	Trace(_T("Client disconnected by reason: 0x") + IntToHex(Reason));
	CloseBluetoothManager();
}

void CAutoGattOperationsDlg::ManagerDiscoveringCompleted(void* Sender, CwclBluetoothRadio* const Radio,
	const int Error)
{
	if (Error != WCL_E_SUCCESS)
	{
		Trace(_T("Discovering completed with error: 0x") + IntToHex(Error));
		CloseBluetoothManager();
	}
	else
	{
		if (FDevices.size() == 0)
		{
			Trace(_T("No one BLE device was found"));
			CloseBluetoothManager();
		}
		else
		{
			Trace(_T("Discovering completed with success"));
			Trace(_T("Use firs found device to connect: ") + IntToHex(*FDevices.begin()));

			FClient.Address = *FDevices.begin();
			int Res = FClient.Connect(Radio);
			if (Res != WCL_E_SUCCESS)
			{
				Trace(_T("Connect attemp failed; 0x") + IntToHex(Res));
				CloseBluetoothManager();
			}
		}
	}
	FDevices.clear();
}

void CAutoGattOperationsDlg::ManagerDeviceFound(void* Sender, CwclBluetoothRadio* const Radio,
	__int64 Address)
{
	Trace(_T("Device found: ") + IntToHex(Address));
	FDevices.push_back(Address);
}

void CAutoGattOperationsDlg::ManagerDiscoveringStarted(void* Sender, CwclBluetoothRadio* const Radio)
{
	Trace(_T("Discovering has been started"));
	FDevices.clear();
}

void CAutoGattOperationsDlg::ManagerBeforeClose(void* Sender)
{
	Trace(_T("Bluetooth Manager closing"));
	btStart.EnableWindow(TRUE);
}

void CAutoGattOperationsDlg::ManagerAfterOpen(void* Sender)
{
	btStart.EnableWindow(FALSE);

	Trace(_T("Bluetooth Manager has been opened"));
	Trace(_T("Looking for working radio"));

	if (FManager.Count == 0)
	{
		Trace(_T("No Bluetooth hardware installed"));
		CloseBluetoothManager();
	}
	else
	{
		CwclBluetoothRadio* Radio = NULL;
		for (size_t i = 0; i < FManager.Count; i++)
		{
			if (FManager.Radios[i]->Available)
			{
				Radio = FManager.Radios[i];
				break;
			}
		}

		if (Radio == NULL)
		{
			Trace(_T("No available Bluetooth Radio was found"));
			CloseBluetoothManager();
		}
		else
		{
			Trace(_T("Start discovering for BLE devices on radio ") + CString(Radio->ApiName.c_str()));
			int Res = Radio->Discover(10, dkBle);
			if (Res != WCL_E_SUCCESS)
			{
				Trace(_T("Start discovering failed: 0x") + IntToHex(Res));
				CloseBluetoothManager();
			}
		}
	}
}

void CAutoGattOperationsDlg::OnBnClickedButtonStart()
{
	lbLog.ResetContent();
	Trace(_T("Open Bluetooth Manager"));
	int Res = FManager.Open();
	if (Res != WCL_E_SUCCESS)
		Trace(_T("Bluetooth Manager open failed: 0x") + IntToHex(Res));
}

void CAutoGattOperationsDlg::OnBnClickedButtonDisconnect()
{
	Disconnect();
}

void CAutoGattOperationsDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	Disconnect();

	__unhook(&FClient);
	__unhook(&FManager);
}
