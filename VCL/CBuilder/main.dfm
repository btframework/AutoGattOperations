object fmMain: TfmMain
  Left = 0
  Top = 0
  BorderStyle = bsSingle
  Caption = 'Auto Gatt Operations'
  ClientHeight = 453
  ClientWidth = 637
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object btStart: TButton
    Left = 8
    Top = 8
    Width = 75
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = btStartClick
  end
  object btDisconnect: TButton
    Left = 89
    Top = 8
    Width = 75
    Height = 25
    Caption = 'Disconnect'
    Enabled = False
    TabOrder = 1
    OnClick = btDisconnectClick
  end
  object lbLog: TListBox
    Left = 8
    Top = 39
    Width = 621
    Height = 406
    ItemHeight = 13
    TabOrder = 2
  end
  object Manager: TwclBluetoothManager
    AfterOpen = ManagerAfterOpen
    BeforeClose = ManagerBeforeClose
    OnDeviceFound = ManagerDeviceFound
    OnDiscoveringCompleted = ManagerDiscoveringCompleted
    OnDiscoveringStarted = ManagerDiscoveringStarted
    Left = 160
    Top = 152
  end
  object Client: TwclGattClient
    OnCharacteristicChanged = ClientCharacteristicChanged
    OnConnect = ClientConnect
    OnDisconnect = ClientDisconnect
    Left = 240
    Top = 160
  end
end
