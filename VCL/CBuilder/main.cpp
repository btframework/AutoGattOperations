//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "wclBluetooth"
#pragma resource "*.dfm"
TfmMain *fmMain;
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::btDisconnectClick(TObject *Sender)
{
	Disconnect();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::btStartClick(TObject *Sender)
{
	lbLog->Items->Clear();
	Trace("Open Bluetooth Manager");
	int Res = Manager->Open();
	if (Res != WCL_E_SUCCESS)
		Trace("Bluetooth Manager open failed: 0x" + IntToHex(Res, 8));
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ClientCharacteristicChanged(TObject *Sender, const WORD Handle,
          const TwclGattCharacteristicValue Value)
{
	Trace("Notification received: ");
	String s;
	if (Value.Length > 0)
	{
		for (int i = 0; i < Value.Length; i++)
			s += IntToHex(Value[i], 2);
	}
	Trace("  " + s);
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ClientConnect(TObject *Sender, const int Error)
{
	if (Error != WCL_E_SUCCESS)
	{
		Trace("Connection failed: 0x" + IntToHex(Error, 8));
		CloseBluetoothManager();
	}
	else
	{
		FSubscribed.clear();
		Trace("Connected with success");

		Trace("Read services");
		TwclGattServices Services;
		int Res = Client->ReadServices(goNone, Services);
		if (Res != WCL_E_SUCCESS)
			Trace("Read services failed: 0x" + IntToHex(Res, 8));
		else
		{
			for (int i = 0; i < Services.Length; i++)
			{
				Trace("Service " + ShowUuid(Services[i].Uuid));
				TwclGattCharacteristics Chars;
				Res = Client->ReadCharacteristics(Services[i], goNone, Chars);
				if (Res != WCL_E_SUCCESS)
					Trace("  Unable to read characteristics: 0x" + IntToHex(Res, 8));
				else
				{
					for (int i = 0; i < Chars.Length; i++)
					{
						Trace("  Characteristic " + ShowUuid(Chars[i].Uuid));
						if (Chars[i].IsReadable)
							Trace("    Readable");
						if (Chars[i].IsWritable || Chars[i].IsSignedWritable || Chars[i].IsWritableWithoutResponse)
							Trace("    Writable");
						if (Chars[i].IsIndicatable || Chars[i].IsNotifiable)
						{
							Trace("    Notifiable");
							Trace("    Subscribe");
							Res = Client->Subscribe(Chars[i]);
							if (Res != WCL_E_SUCCESS)
								Trace("      Subscribed failed: 0x" + IntToHex(Res, 8));
							else
							{
								Res = Client->WriteClientConfiguration(Chars[i], true, goNone, plNone);
								if (Res != WCL_E_SUCCESS)
								{
									Trace("      Write CCD failed: 0x" + IntToHex(Res, 8));
									Client->Unsubscribe(Chars[i]);
								}
								else
								{
									Trace("      Subscribed");
									FSubscribed.push_back(Chars[i]);
								}
							}
						}
					}
				}
			}
		}
		btDisconnect->Enabled = true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ClientDisconnect(TObject *Sender, const int Reason)
{
	btDisconnect->Enabled = false;

	Trace("Client disconnected by reason: 0x" + IntToHex(Reason, 8));
	CloseBluetoothManager();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::CloseBluetoothManager()
{
	Trace("Closing Bluetooth Manager");
	Manager->Close();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::Disconnect()
{
	if (btDisconnect->Enabled)
	{
		Trace("Unsubscribe from all characteristics");

		for (std::list<TwclGattCharacteristic>::iterator Char = FSubscribed.begin(); Char != FSubscribed.end(); Char++)
		{
			Client->WriteClientConfiguration(*Char, false, goNone, plNone);
			Client->Unsubscribe(*Char);
		}
		FSubscribed.clear();

		Trace("Disconnecting");
		Client->Disconnect();
	}
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	Disconnect();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ManagerAfterOpen(TObject *Sender)
{
	btStart->Enabled = false;

	Trace("Bluetooth Manager has been opened");
	Trace("Looking for working radio");

	if (Manager->Count == 0)
	{
		Trace("No Bluetooth hardware installed");
		CloseBluetoothManager();
	}
	else
	{
		TwclBluetoothRadio* Radio = NULL;
		for (int i = 0; i < Manager->Count; i++)
		{
			if (Manager->Radios[i]->Available)
			{
				Radio = Manager->Radios[i];
				break;
			}
		}

		if (Radio == NULL)
		{
			Trace("No available Bluetooth Radio was found");
			CloseBluetoothManager();
		}
		else
		{
			Trace("Start discovering for BLE devices on radio " + Radio->ApiName);
			int Res = Radio->Discover(10, dkBle);
			if (Res != WCL_E_SUCCESS)
			{
				Trace("Start discovering failed: 0x" + IntToHex(Res, 8));
				CloseBluetoothManager();
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ManagerBeforeClose(TObject *Sender)
{
	Trace("Bluetooth Manager closing");
	btStart->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ManagerDeviceFound(TObject *Sender, TwclBluetoothRadio * const Radio,
          const __int64 Address)
{
	Trace("Device found: " + IntToHex(Address, 12));
	FDevices.push_back(Address);
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ManagerDiscoveringCompleted(TObject *Sender, TwclBluetoothRadio * const Radio,
          const int Error)
{
	if (Error != WCL_E_SUCCESS)
	{
		Trace("Discovering completed with error: 0x" + IntToHex(Error, 8));
		CloseBluetoothManager();
	}
	else
	{
		if (FDevices.size() == 0)
		{
			Trace("No one BLE device was found");
			CloseBluetoothManager();
		}
		else
		{
			Trace("Discovering completed with success");
			Trace("Use firs found device to connect: " + IntToHex(*FDevices.begin(), 12));

			Client->Address = *FDevices.begin();
			int Res = Client->Connect(Radio);
			if (Res != WCL_E_SUCCESS)
			{
				Trace("Connect attemp failed; 0x" + IntToHex(Res, 8));
				CloseBluetoothManager();
			}
		}
	}
	FDevices.clear();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::ManagerDiscoveringStarted(TObject *Sender, TwclBluetoothRadio * const Radio)
{
	Trace("Discovering has been started");
	FDevices.clear();
}
//---------------------------------------------------------------------------

String __fastcall TfmMain::ShowUuid(const TwclGattUuid& Uuid)
{
	if (Uuid.IsShortUuid)
		return IntToHex(Uuid.ShortUuid, 4);
	return GUIDToString(Uuid.LongUuid);
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::Trace(const String& Msg)
{
	lbLog->Items->Add(Msg);
}
//---------------------------------------------------------------------------

