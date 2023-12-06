using System;
using System.Collections.Generic;
using System.Windows.Forms;

using wclCommon;
using wclBluetooth;

namespace AutoGattOperations
{
    public partial class fmMain : Form
    {
        private wclBluetoothManager FManager;
        private wclGattClient FClient;
        private List<Int64> FDevices;
        private List<wclGattCharacteristic> FSubscribed;

        private void Trace(String Msg)
        {
            lbLog.Items.Add(Msg);
        }

        private String ShowUuid(wclGattUuid Uuid)
        {
            if (Uuid.IsShortUuid)
                return Uuid.ShortUuid.ToString("X4");
            return Uuid.LongUuid.ToString();
        }

        private void CloseBluetoothManager()
        {
            Trace("Closing Bluetooth Manager");
            FManager.Close();
        }

        private void Disconnect()
        {
            if (btDisconnect.Enabled)
            {
                Trace("Unsubscribe from all characteristics");

                foreach (wclGattCharacteristic Char in FSubscribed)
                {
                    FClient.WriteClientConfiguration(Char, false, wclGattOperationFlag.goNone, wclGattProtectionLevel.plNone);
                    FClient.Unsubscribe(Char);
                }
                FSubscribed.Clear();
                FSubscribed = null;

                Trace("Disconnecting");
                FClient.Disconnect();
            }
        }

        public fmMain()
        {
            InitializeComponent();
        }

        private void fmMainLoad(object sender, EventArgs e)
        {
            FDevices = null;
            FSubscribed = null;

            FManager = new wclBluetoothManager();
            FManager.AfterOpen += new EventHandler(ManagerAfterOpen);
            FManager.BeforeClose += new EventHandler(ManagerBeforeClose);
            FManager.OnDiscoveringStarted += new wclBluetoothEvent(ManagerDiscoveringStarted);
            FManager.OnDeviceFound += new wclBluetoothDeviceEvent(ManagerDeviceFound);
            FManager.OnDiscoveringCompleted += new wclBluetoothResultEvent(ManagerDiscoveringCompleted);

            FClient = new wclGattClient();
            FClient.OnDisconnect += new wclCommunication.wclClientConnectionDisconnectEvent(ClientDisconnect);
            FClient.OnConnect += new wclCommunication.wclClientConnectionConnectEvent(ClientConnect);
            FClient.OnCharacteristicChanged += new wclGattCharacteristicChangedEvent(ClientCharacteristicChanged);
        }

        void ClientCharacteristicChanged(Object Sender, UInt16 Handle, Byte[] Value)
        {
            Trace("Notification received: ");
            String s = "";
            foreach (Byte b in Value)
                s = s + b.ToString("X2");
            Trace("  " + s);
        }

        void ClientConnect(Object Sender, Int32 Error)
        {
            if (Error != wclErrors.WCL_E_SUCCESS)
            {
                Trace("Connection failed: 0x" + Error.ToString("X8"));
                CloseBluetoothManager();
            }
            else
            {
                FSubscribed = new List<wclGattCharacteristic>();

                Trace("Connected with success");

                Trace("Read services");
                wclGattService[] Services;
                Int32 Res = FClient.ReadServices(wclGattOperationFlag.goNone, out Services);
                if (Res != wclErrors.WCL_E_SUCCESS)
                    Trace("Read services failed: 0x" + Res.ToString("X8"));
                else
                {
                    foreach (wclGattService Service in Services)
                    {
                        Trace("Service " + ShowUuid(Service.Uuid));
                        wclGattCharacteristic[] Chars;
                        Res = FClient.ReadCharacteristics(Service, wclGattOperationFlag.goNone, out Chars);
                        if (Res != wclErrors.WCL_E_SUCCESS)
                            Trace("  Unable to read characteristics: 0x" + Res.ToString("X8"));
                        else
                        {
                            foreach (wclGattCharacteristic Char in Chars)
                            {
                                Trace("  Characteristic " + ShowUuid(Char.Uuid));
                                if (Char.IsReadable)
                                    Trace("    Readable");
                                if (Char.IsWritable || Char.IsSignedWritable || Char.IsWritableWithoutResponse)
                                    Trace("    Writable");
                                if (Char.IsIndicatable || Char.IsNotifiable)
                                {
                                    Trace("    Notifiable");
                                    Trace("    Subscribe");
                                    Res = FClient.Subscribe(Char);
                                    if (Res != wclErrors.WCL_E_SUCCESS)
                                        Trace("      Subscribed failed: 0x" + Res.ToString("X8"));
                                    else
                                    {
                                        Res = FClient.WriteClientConfiguration(Char, true, wclGattOperationFlag.goNone, wclGattProtectionLevel.plNone);
                                        if (Res != wclErrors.WCL_E_SUCCESS)
                                        {
                                            Trace("      Write CCD failed: 0x" + Res.ToString("X8"));
                                            FClient.Unsubscribe(Char);
                                        }
                                        else
                                        {
                                            Trace("      Subscribed");
                                            FSubscribed.Add(Char);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                btDisconnect.Enabled = true;
            }
        }

        void ClientDisconnect(Object Sender, Int32 Reason)
        {
            btDisconnect.Enabled = false;

            Trace("Client disconnected by reason: 0x" + Reason.ToString("X8"));
            CloseBluetoothManager();
        }

        void ManagerDiscoveringCompleted(Object Sender, wclBluetoothRadio Radio, Int32 Error)
        {
            if (Error != wclErrors.WCL_E_SUCCESS)
            {
                Trace("Discovering completed with error: 0x" + Error.ToString("X8"));
                CloseBluetoothManager();
            }
            else
            {
                if (FDevices.Count == 0)
                {
                    Trace("No one BLE device was found");
                    CloseBluetoothManager();
                }
                else
                {
                    Trace("Discovering completed with success");
                    Trace("Use firs found device to connect: " + FDevices[0].ToString("X12"));

                    FClient.Address = FDevices[0];
                    Int32 Res = FClient.Connect(Radio);
                    if (Res != wclErrors.WCL_E_SUCCESS)
                    {
                        Trace("Connect attemp failed; 0x" + Res.ToString("X8"));
                        CloseBluetoothManager();
                    }
                }
            }
            FDevices = null;
        }

        void ManagerDeviceFound(Object Sender, wclBluetoothRadio Radio, Int64 Address)
        {
            Trace("Device found: " + Address.ToString("X12"));
            FDevices.Add(Address);
        }

        void ManagerDiscoveringStarted(Object Sender, wclBluetoothRadio Radio)
        {
            Trace("Discovering has been started");
            FDevices = new List<Int64>();
        }

        void ManagerBeforeClose(Object sender, EventArgs e)
        {
            Trace("Bluetooth Manager closing");
            btStart.Enabled = true;
        }

        void ManagerAfterOpen(Object sender, EventArgs e)
        {
            btStart.Enabled = false;

            Trace("Bluetooth Manager has been opened");
            Trace("Looking for working radio");

            if (FManager.Count == 0)
            {
                Trace("No Bluetooth hardware installed");
                CloseBluetoothManager();
            }
            else
            {
                wclBluetoothRadio Radio = null;
                for (Int32 i = 0; i < FManager.Count; i++)
                {
                    if (FManager[i].Available)
                    {
                        Radio = FManager[i];
                        break;
                    }
                }

                if (Radio == null)
                {
                    Trace("No available Bluetooth Radio was found");
                    CloseBluetoothManager();
                }
                else
                {
                    Trace("Start discovering for BLE devices on radio " + Radio.ApiName);
                    Int32 Res = Radio.Discover(10, wclBluetoothDiscoverKind.dkBle);
                    if (Res != wclErrors.WCL_E_SUCCESS)
                    {
                        Trace("Start discovering failed: 0x" + Res.ToString("X8"));
                        CloseBluetoothManager();
                    }
                }
            }
        }

        private void btStartClick(Object sender, EventArgs e)
        {
            lbLog.Items.Clear();
            Trace("Open Bluetooth Manager");
            Int32 Res = FManager.Open();
            if (Res != wclErrors.WCL_E_SUCCESS)
                Trace("Bluetooth Manager open failed: 0x" + Res.ToString("X8"));
        }

        private void btDisconnectClick(Object sender, EventArgs e)
        {
            Disconnect();
        }

        private void fmMainClosed(Object sender, FormClosedEventArgs e)
        {
            Disconnect();
        }
    }
}