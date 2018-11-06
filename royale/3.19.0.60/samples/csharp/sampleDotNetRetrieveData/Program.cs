/****************************************************************************\
 * Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace sampleDotNetRetrieveData
{
    class Program
    {
        class DataReceiver : RoyaleDotNet.IDepthDataListener
        {
            public void OnNewData (RoyaleDotNet.DepthData data)
            {
                RoyaleDotNet.DepthPoint dp = data.points[ (data.height * data.width) / 2];
                Console.WriteLine ("============================================================\n");
                Console.WriteLine ("Received Frame: " + data.width + "x" + data.height + ", some point: { " + dp.x + ", " + dp.y + ", " + dp.z + " } confidence: " + dp.depthConfidence);
            }
        }

        static void Main (string[] args)
        {
            Console.WriteLine ("============================================================");
            Console.WriteLine ("|                    royale .NET sample                    |");
            Console.WriteLine ("============================================================");

            RoyaleDotNet.CameraStatus status;
            DataReceiver receiver = new DataReceiver();
            RoyaleDotNet.CameraManager camManager = new RoyaleDotNet.CameraManager();
            List<string> connectedCameras = camManager.GetConnectedCameraList();

            if (connectedCameras.Count == 0)
            {
                Console.WriteLine ("No connected cameras found.");
                return;
            }

            RoyaleDotNet.CameraDevice device = camManager.CreateCamera (connectedCameras[0]);

            status = device.Initialize();
            if (RoyaleDotNet.CameraStatus.SUCCESS != status)
            {
                Console.WriteLine ("Failed to initialize camera.");
            }

            status = device.RegisterDepthDataListener (receiver);
            if (RoyaleDotNet.CameraStatus.SUCCESS != status)
            {
                Console.WriteLine ("Failed to register data listener.");
            }

            Console.WriteLine ("Starting to capture for 10 seconds.");

            status = device.StartCapture();
            if (RoyaleDotNet.CameraStatus.SUCCESS != status)
            {
                Console.WriteLine ("Failed to start capture.");
            }
            Thread.Sleep (10000);

            status = device.StopCapture();
            if (RoyaleDotNet.CameraStatus.SUCCESS != status)
            {
                Console.WriteLine ("Failed to stop capture.");
            }

        }
    }
}
