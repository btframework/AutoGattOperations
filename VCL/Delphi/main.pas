unit main;

interface

uses
  Vcl.Forms, Vcl.Dialogs, Vcl.Controls, Vcl.StdCtrls, wclBluetooth,
  System.Classes, System.Generics.Collections;

type
  TfmMain = class(TForm)
    Manager: TwclBluetoothManager;
    Client: TwclGattClient;
    btStart: TButton;
    btDisconnect: TButton;
    lbLog: TListBox;
    procedure FormCreate(Sender: TObject);
    procedure ClientCharacteristicChanged(Sender: TObject; const Handle: Word;
      const Value: TwclGattCharacteristicValue);
    procedure ClientConnect(Sender: TObject; const Error: Integer);
    procedure ClientDisconnect(Sender: TObject; const Reason: Integer);
    procedure ManagerDiscoveringCompleted(Sender: TObject;
      const Radio: TwclBluetoothRadio; const Error: Integer);
    procedure ManagerDeviceFound(Sender: TObject;
      const Radio: TwclBluetoothRadio; const Address: Int64);
    procedure ManagerDiscoveringStarted(Sender: TObject;
      const Radio: TwclBluetoothRadio);
    procedure ManagerBeforeClose(Sender: TObject);
    procedure ManagerAfterOpen(Sender: TObject);
    procedure btStartClick(Sender: TObject);
    procedure btDisconnectClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
  private
    FDevices: TList<Int64>;
    FSubscribed: TList<TwclGattCharacteristic>;

    procedure Trace(const Msg: string);
    function ShowUuid(const Uuid: TwclGattUuid): string;
    procedure CloseBluetoothManager;
    procedure Disconnect;
  end;

var
  fmMain: TfmMain;

implementation

uses
  SysUtils, wclErrors;

{$R *.dfm}

{ TfmMain }

procedure TfmMain.btDisconnectClick(Sender: TObject);
begin
  Disconnect;
end;

procedure TfmMain.btStartClick(Sender: TObject);
var
  Res: Integer;
begin
  lbLog.Items.Clear;
  Trace('Open Bluetooth Manager');
  Res := Manager.Open;
  if Res <> WCL_E_SUCCESS then
    Trace('Bluetooth Manager open failed: 0x' + IntToHex(Res, 8));
end;

procedure TfmMain.ClientCharacteristicChanged(Sender: TObject;
  const Handle: Word; const Value: TwclGattCharacteristicValue);
var
  s: string;
  b: Byte;
begin
  Trace('Notification received: ');
  s := '';
  for b in Value do
    s := s + IntToHex(b, 2);
  Trace('  ' + s);
end;

procedure TfmMain.ClientConnect(Sender: TObject; const Error: Integer);
var
  Services: TwclGattServices;
  Res: Integer;
  Service: TwclGattService;
  Chars: TwclGattCharacteristics;
  Char: TwclGattCharacteristic;
begin
  if Error <> WCL_E_SUCCESS then begin
    Trace('Connection failed: 0x' + IntToHex(Error, 8));
    CloseBluetoothManager;

  end else begin
    FSubscribed := TList<TwclGattCharacteristic>.Create();

    Trace('Connected with success');

    Trace('Read services');
    Res := Client.ReadServices(goNone, Services);
    if Res <> WCL_E_SUCCESS then
      Trace('Read services failed: 0x' + IntToHex(Res, 8))

    else begin
      for Service in Services do begin
        Trace('Service ' + ShowUuid(Service.Uuid));
        Res := Client.ReadCharacteristics(Service, goNone, Chars);
        if Res <> WCL_E_SUCCESS then
          Trace('  Unable to read characteristics: 0x' + IntToHex(Res, 8))

        else begin
          for Char in Chars do begin
            Trace('  Characteristic ' + ShowUuid(Char.Uuid));
            if Char.IsReadable then
              Trace('    Readable');
            if Char.IsWritable or Char.IsSignedWritable or Char.IsWritableWithoutResponse then
              Trace('    Writable');
            if Char.IsIndicatable or Char.IsNotifiable then begin
              Trace('    Notifiable');
              Trace('    Subscribe');
              Res := Client.Subscribe(Char);
              if Res <> WCL_E_SUCCESS then
                Trace('      Subscribed failed: 0x' + IntToHex(Res, 8))

              else begin
                Res := Client.WriteClientConfiguration(Char, True, goNone, plNone);
                if Res <> WCL_E_SUCCESS then begin
                  Trace('      Write CCD failed: 0x' + IntToHex(Res, 8));
                  Client.Unsubscribe(Char);

                end else begin
                  Trace('      Subscribed');
                  FSubscribed.Add(Char);
                end;
              end;
            end;
          end;
        end;
      end;
    end;
    btDisconnect.Enabled := True;
  end;
end;

procedure TfmMain.ClientDisconnect(Sender: TObject; const Reason: Integer);
begin
  btDisconnect.Enabled := False;

  Trace('Client disconnected by reason: 0x' + IntToHex(Reason, 8));
  CloseBluetoothManager;
end;

procedure TfmMain.CloseBluetoothManager;
begin
  Trace('Closing Bluetooth Manager');
  Manager.Close;
end;

procedure TfmMain.Disconnect;
var
  Char: TwclGattCharacteristic;
begin
  if btDisconnect.Enabled then begin
    Trace('Unsubscribe from all characteristics');

    for Char in FSubscribed do begin
      Client.WriteClientConfiguration(Char, False, goNone, plNone);
      Client.Unsubscribe(Char);
    end;
    FSubscribed.Clear;
    FSubscribed.Free;

    Trace('Disconnecting');
    Client.Disconnect;
  end;
end;

procedure TfmMain.FormCreate(Sender: TObject);
begin
  FDevices := nil;
  FSubscribed := nil;
end;

procedure TfmMain.FormDestroy(Sender: TObject);
begin
  Disconnect;
end;

procedure TfmMain.ManagerAfterOpen(Sender: TObject);
var
  Radio: TwclBluetoothRadio;
  i: Integer;
  Res: Integer;
begin
  btStart.Enabled := False;

  Trace('Bluetooth Manager has been opened');
  Trace('Looking for working radio');

  if Manager.Count = 0 then begin
    Trace('No Bluetooth hardware installed');
    CloseBluetoothManager;

  end else begin
    Radio := nil;
    for i := 0 to Manager.Count - 1 do begin
      if Manager[i].Available then begin
        Radio := Manager[i];
        Break;
      end;
    end;

    if Radio = nil then begin
      Trace('No available Bluetooth Radio was found');
      CloseBluetoothManager;

    end else begin
      Trace('Start discovering for BLE devices on radio ' + Radio.ApiName);
      Res := Radio.Discover(10, dkBle);
      if Res <> WCL_E_SUCCESS then begin
        Trace('Start discovering failed: 0x' + IntToHex(Res, 8));
        CloseBluetoothManager;
      end;
    end;
  end;
end;

procedure TfmMain.ManagerBeforeClose(Sender: TObject);
begin
  Trace('Bluetooth Manager closing');
  btStart.Enabled := True;
end;

procedure TfmMain.ManagerDeviceFound(Sender: TObject;
  const Radio: TwclBluetoothRadio; const Address: Int64);
begin
  Trace('Device found: ' + IntToHex(Address, 12));
  FDevices.Add(Address);
end;

procedure TfmMain.ManagerDiscoveringCompleted(Sender: TObject;
  const Radio: TwclBluetoothRadio; const Error: Integer);
var
  Res: Integer;
begin
  if Error <> WCL_E_SUCCESS then begin
    Trace('Discovering completed with error: 0x' + IntToHex(Error, 8));
    CloseBluetoothManager;

  end else begin
    if FDevices.Count = 0 then begin
      Trace('No one BLE device was found');
      CloseBluetoothManager;

    end else begin
      Trace('Discovering completed with success');
      Trace('Use firs found device to connect: ' + IntToHex(FDevices[0], 12));

      Client.Address := FDevices[0];
      Res := Client.Connect(Radio);
      if Res <> WCL_E_SUCCESS then begin
        Trace('Connect attemp failed; 0x' + IntToHex(Res, 8));
        CloseBluetoothManager;
      end;
    end;
  end;
  FDevices.Free;
  FDevices := nil;
end;

procedure TfmMain.ManagerDiscoveringStarted(Sender: TObject;
  const Radio: TwclBluetoothRadio);
begin
  Trace('Discovering has been started');
  FDevices := TList<Int64>.Create;
end;

function TfmMain.ShowUuid(const Uuid: TwclGattUuid): string;
begin
  if Uuid.IsShortUuid then
    Result := IntToHex(Uuid.ShortUuid, 4)
  else
    Result := GUIDToString(Uuid.LongUuid);
end;

procedure TfmMain.Trace(const Msg: string);
begin
  lbLog.Items.Add(Msg);
end;

end.
