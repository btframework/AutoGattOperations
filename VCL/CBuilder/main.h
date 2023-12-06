//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
#include "wclBluetooth.hpp"
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
//---------------------------------------------------------------------------
#include <list>
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include "wclBluetooth.hpp"
#include "wclAudio.hpp"
//---------------------------------------------------------------------------
class TfmMain : public TForm
{
__published:	// IDE-managed Components
	TButton *btStart;
	TButton *btDisconnect;
	TListBox *lbLog;
	TwclBluetoothManager *Manager;
	TwclGattClient *Client;
	void __fastcall btDisconnectClick(TObject *Sender);
	void __fastcall btStartClick(TObject *Sender);
	void __fastcall ClientCharacteristicChanged(TObject *Sender, const WORD Handle,
		  const TwclGattCharacteristicValue Value);
	void __fastcall ClientConnect(TObject *Sender, const int Error);
	void __fastcall ClientDisconnect(TObject *Sender, const int Reason);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ManagerAfterOpen(TObject *Sender);
	void __fastcall ManagerBeforeClose(TObject *Sender);
	void __fastcall ManagerDeviceFound(TObject *Sender, TwclBluetoothRadio * const Radio,
          const __int64 Address);
	void __fastcall ManagerDiscoveringCompleted(TObject *Sender, TwclBluetoothRadio * const Radio,
          const int Error);
	void __fastcall ManagerDiscoveringStarted(TObject *Sender, TwclBluetoothRadio * const Radio);


private:	// User declarations
	std::list<__int64>					FDevices;
	std::list<TwclGattCharacteristic>	FSubscribed;

	void __fastcall Trace(const String& Msg);
	String __fastcall ShowUuid(const TwclGattUuid& Uuid);
	void __fastcall CloseBluetoothManager();
	void __fastcall Disconnect();

public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
