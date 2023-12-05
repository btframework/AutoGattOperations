Public Class fmMain
    Private WithEvents FManager As wclBluetoothManager
    Private WithEvents FClient As wclGattClient
    Private FDevices As List(Of Int64)
    Private FSubscribed As List(Of wclGattCharacteristic)

    Private Sub Trace(Msg As String)
        lbLog.Items.Add(Msg)
    End Sub

    Private Function ShowUuid(Uuid As wclGattUuid) As String
        If Uuid.IsShortUuid Then Return Uuid.ShortUuid.ToString("X4")
        Return Uuid.LongUuid.ToString()
    End Function

    Private Sub CloseBluetoothManager()
        Trace("Closing Bluetooth Manager")
        FManager.Close()
    End Sub

    Private Sub Disconnect()
        If btDisconnect.Enabled Then
            Trace("Unsubscribe from all characteristics")

            For Each [Char] As wclGattCharacteristic In FSubscribed
                FClient.WriteClientConfiguration([Char], False, wclGattOperationFlag.goNone, wclGattProtectionLevel.plNone)
                FClient.Unsubscribe([Char])
            Next
            FSubscribed.Clear()
            FSubscribed = Nothing

            Trace("Disconnecting")
            FClient.Disconnect()
        End If
    End Sub

    Private Sub fmMainLoad(sender As Object, e As EventArgs) Handles MyBase.Load
        FDevices = Nothing

        FManager = New wclBluetoothManager()
        FClient = New wclGattClient()
    End Sub

    Private Sub ClientCharacteristicChanged(Sender As Object, Handle As UShort, Value() As Byte) Handles FClient.OnCharacteristicChanged
        Trace("Notification received: ")
        Dim s As String = ""
        For Each b As Byte In Value
            s = s + b.ToString("X2")
            Trace("  " + s)
        Next
    End Sub

    Private Sub ClientConnect(Sender As Object, [Error] As Integer) Handles FClient.OnConnect
        If [Error] <> wclErrors.WCL_E_SUCCESS Then
            Trace("Connection failed: 0x" + [Error].ToString("X8"))
            CloseBluetoothManager()
        Else
            FSubscribed = New List(Of wclGattCharacteristic)()

            Trace("Connected with success")

            Trace("Read services")
            Dim Services As wclGattService() = Nothing
            Dim Res As Int32 = FClient.ReadServices(wclGattOperationFlag.goNone, Services)
            If Res <> wclErrors.WCL_E_SUCCESS Then
                Trace("Read services failed: 0x" + Res.ToString("X8"))
            Else
                For Each Service As wclGattService In Services
                    Trace("Service " + ShowUuid(Service.Uuid))
                    Dim Chars As wclGattCharacteristic() = Nothing
                    Res = FClient.ReadCharacteristics(Service, wclGattOperationFlag.goNone, Chars)
                    If Res <> wclErrors.WCL_E_SUCCESS Then
                        Trace("  Unable to read characteristics: 0x" + Res.ToString("X8"))
                    Else
                        For Each [Char] As wclGattCharacteristic In Chars
                            Trace("  Characteristic " + ShowUuid([Char].Uuid))
                            If [Char].IsReadable Then Trace("    Readable")
                            If [Char].IsWritable OrElse [Char].IsSignedWritable OrElse [Char].IsWritableWithoutResponse Then Trace("    Writable")
                            If [Char].IsIndicatable OrElse [Char].IsNotifiable Then
                                Trace("    Notifiable")
                                Trace("    Subscribe")
                                Res = FClient.Subscribe([Char])
                                If Res <> wclErrors.WCL_E_SUCCESS Then
                                    Trace("      Subscribed failed: 0x" + Res.ToString("X8"))
                                Else
                                    Res = FClient.WriteClientConfiguration([Char], True, wclGattOperationFlag.goNone, wclGattProtectionLevel.plNone)
                                    If Res <> wclErrors.WCL_E_SUCCESS Then
                                        Trace("      Write CCD failed: 0x" + Res.ToString("X8"))
                                        FClient.Unsubscribe([Char])
                                    Else
                                        Trace("      Subscribed")
                                        FSubscribed.Add([Char])
                                    End If
                                End If
                            End If
                        Next
                    End If
                Next
            End If
            btDisconnect.Enabled = True
        End If
    End Sub

    Private Sub ClientDisconnect(Sender As Object, Reason As Integer) Handles FClient.OnDisconnect
        btDisconnect.Enabled = False

        Trace("Client disconnected by reason: 0x" + Reason.ToString("X8"))
        CloseBluetoothManager()
    End Sub

    Private Sub ManagerDiscoveringCompleted(Sender As Object, Radio As wclBluetoothRadio, [Error] As Integer) Handles FManager.OnDiscoveringCompleted
        If [Error] <> wclErrors.WCL_E_SUCCESS Then
            Trace("Discovering completed with error: 0x" + [Error].ToString("X8"))
            CloseBluetoothManager()
        Else
            If FDevices.Count = 0 Then
                Trace("No one BLE device was found")
                CloseBluetoothManager()
            Else
                Trace("Discovering completed with success")
                Trace("Use firs found device to connect: " + FDevices(0).ToString("X12"))

                FClient.Address = FDevices(0)
                Dim Res As Int32 = FClient.Connect(Radio)
                If Res <> wclErrors.WCL_E_SUCCESS Then
                    Trace("Connect attemp failed; 0x" + Res.ToString("X8"))
                    CloseBluetoothManager()
                End If
            End If
        End If
        FDevices = Nothing
    End Sub

    Private Sub ManagerDeviceFound(Sender As Object, Radio As wclBluetoothRadio, Address As Long) Handles FManager.OnDeviceFound
        Trace("Device found: " + Address.ToString("X12"))
        FDevices.Add(Address)
    End Sub

    Private Sub ManagerDiscoveringStarted(Sender As Object, Radio As wclBluetoothRadio) Handles FManager.OnDiscoveringStarted
        Trace("Discovering has been started")
        FDevices = New List(Of Int64)()
    End Sub

    Private Sub ManagerBeforeClose(sender As Object, e As EventArgs) Handles FManager.BeforeClose
        Trace("Bluetooth Manager closing")
        btStart.Enabled = True
    End Sub

    Private Sub ManagerAfterOpen(sender As Object, e As EventArgs) Handles FManager.AfterOpen
        btStart.Enabled = False

        Trace("Bluetooth Manager has been opened")
        Trace("Looking for working radio")

        If FManager.Count = 0 Then
            Trace("No Bluetooth hardware installed")
            CloseBluetoothManager()
        Else
            Dim Radio As wclBluetoothRadio = Nothing
            For i As Int32 = 0 To FManager.Count - 1
                If FManager(i).Available Then
                    Radio = FManager(i)
                    Exit For
                End If
            Next

            If Radio Is Nothing Then
                Trace("No available Bluetooth Radio was found")
                CloseBluetoothManager()
            Else
                Trace("Start discovering for BLE devices on radio " + Radio.ApiName)
                Dim Res As Int32 = Radio.Discover(10, wclBluetoothDiscoverKind.dkBle)
                If Res <> wclErrors.WCL_E_SUCCESS Then
                    Trace("Start discovering failed: 0x" + Res.ToString("X8"))
                    CloseBluetoothManager()
                End If
            End If
        End If
    End Sub

    Private Sub btStartClick(sender As Object, e As EventArgs) Handles btStart.Click
        lbLog.Items.Clear()
        Trace("Open Bluetooth Manager")
        Dim Res As Int32 = FManager.Open()
        If Res <> wclErrors.WCL_E_SUCCESS Then Trace("Bluetooth Manager open failed: 0x" + Res.ToString("X8"))
    End Sub

    Private Sub btDisconnectClick(sender As Object, e As EventArgs) Handles btDisconnect.Click
        Disconnect()
    End Sub

    Private Sub fmMainClosed(sender As Object, e As FormClosedEventArgs) Handles MyBase.FormClosed
        Disconnect()
    End Sub
End Class
