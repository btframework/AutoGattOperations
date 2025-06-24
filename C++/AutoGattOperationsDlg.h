
// AutoGattOperationsDlg.h : header file
//

#pragma once

#include <list>

#include "wclBluetooth.h"
#include "afxwin.h"

using namespace std;
using namespace wclCommon;
using namespace wclBluetooth;

// CAutoGattOperationsDlg dialog
class CAutoGattOperationsDlg : public CDialogEx
{
// Construction
public:
	CAutoGattOperationsDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTOGATTOPERATIONS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	CButton btStart;
	CButton btDisconnect;
	CListBox lbLog;

private:
	CwclBluetoothManager		FManager;
	CwclGattClient				FClient;
	list<__int64>				FDevices;
	list<wclGattCharacteristic>	FSubscribed;

	void Trace(CString Msg);
	CString ShowUuid(wclGattUuid Uuid);
	void CloseBluetoothManager();
	void Disconnect();

	void ClientCharacteristicChanged(void* Sender, const unsigned short Handle,
		const unsigned char* Value, const unsigned long Length);
	void ClientConnect(void* Sender, const int Error);
	void ClientDisconnect(void* Sender, const int Reason);

	void ManagerDiscoveringCompleted(void* Sender, CwclBluetoothRadio* const Radio,
		const int Error);
	void ManagerDeviceFound(void* Sender, CwclBluetoothRadio* const Radio, __int64 Address);
	void ManagerDiscoveringStarted(void* Sender, CwclBluetoothRadio* const Radio);
	void ManagerBeforeClose(void* Sender);
	void ManagerAfterOpen(void* Sender);
public:
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnDestroy();
};
