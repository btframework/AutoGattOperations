////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//   Wireless Communication Library 7                                         //
//                                                                            //
//   Copyright (C) 2006-2023 Mike Petrichenko                                 //
//                           Soft Service Company                             //
//                           All Rights Reserved                              //
//                                                                            //
//   http://www.btframework.com                                               //
//                                                                            //
//   support@btframework.com                                                  //
//   shop@btframework.com                                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace wclBluetooth
{

/* Microsoft control functions declarations. */

/// <summary> Creates a Microsoft Bluetooth Radio object. </summary>
/// <param name="Manager"> The <see cref="CwclBluetoothManager" /> object that
///   owns the Radio. </param>
/// <returns> The function returns a Radio object that represents the Microsoft
///   Bluetooth driver. </returns>
/// <remarks> An application must never call this function. Instead it must use
///   high-level Bluetooth functions and classes. This function is for
///   internal use only. </remarks>
/// <seealso cref="CwclBluetoothRadio" />
/// <seealso cref="CwclBluetoothManager" />
CwclBluetoothRadio* wclLoadMicrosoft(CwclBluetoothManager* const Manager);

}