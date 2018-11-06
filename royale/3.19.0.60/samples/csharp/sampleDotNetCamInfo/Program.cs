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
using System.Threading.Tasks;

namespace sampleDotNetCamInfo
{
    class Program
    {
        static void WriteHeader()
        {
            Console.WriteLine ("+----------------------------------------------------------+");
            Console.WriteLine ("|                    royale .NET sample                    |");
            Console.WriteLine ("+----------------------------------------------------------+");
        }

        static void WriteVersion()
        {
            // There are two methods to get the version number, both will give the same result.
            UInt32 major, minor, patch, build;
            RoyaleDotNet.Royale.GetVersion (out major, out minor, out patch, out build);
            Console.WriteLine ("Royale library version: " + major + "." + minor + "." + patch + "." + build);

            // Reading the same information via the RoyaleDotNet Assembly. Royale's version
            // numbers are major.minor.patch.build, so in System.Version the patch is in
            // version.Build and the build is in version.Revision.
            Version version = typeof (RoyaleDotNet.CameraDevice).Assembly.GetName().Version;
            Console.WriteLine ("RoyaleDotNet version: " + version);

            // It's also possible to read the version control revision that it was built from.
            String scm;
            RoyaleDotNet.CameraStatus status = RoyaleDotNet.Royale.GetVersion (out major, out minor, out patch, out build, out scm);
            if (RoyaleDotNet.CameraStatus.SUCCESS == status)
            {
                // This is not expected to fail, unless something is mismatched with the version numbers,
                // because the only error condition that should make it fail is running out of memory.
                Console.WriteLine ("This library was built from SCM version " + scm);
            }
        }

        static RoyaleDotNet.CameraDevice GetFirstCameraDevice()
        {
            RoyaleDotNet.CameraDevice camDevice;
            RoyaleDotNet.CameraManager camManager = new RoyaleDotNet.CameraManager();
            List<string> connectedCameras = camManager.GetConnectedCameraList();

            if (connectedCameras.Count <= 0)
            {
                Console.WriteLine ("No connected cameras found");
                return null;
            }

            camDevice = camManager.CreateCamera (connectedCameras[0]);

            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.Initialize())
            {
                Console.WriteLine ("Failed to initialize camera");
            }

            return camDevice;
        }

        static void WriteCameraInfo ()
        {
            RoyaleDotNet.CameraDevice camDevice = GetFirstCameraDevice();

            if (camDevice == null)
            {
                return;
            }

            // the values to set
            string camId;
            string camName;
            ushort camMaxSensorWidth;
            ushort camMaxSensorHeight;

            List<string> camUseCases;
            List<KeyValuePair<string, string>> camInfo;



            // set the value camId
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetId (out camId))
            {
                Console.WriteLine ("failed to get ID");
                return;
            }


            // set the value camName
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetCameraName (out camName))
            {
                Console.WriteLine ("failed to get name");
                return;
            }


            // set the value camMaxSensorWidth
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetMaxSensorWidth (out camMaxSensorWidth))
            {
                Console.WriteLine ("failed to get max sensor width");
                return;
            }


            // set the value camMaxSensorHeight
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetMaxSensorHeight (out camMaxSensorHeight))
            {
                Console.WriteLine ("failed to get max sensor height");
                return;
            }


            // set the value camUseCases
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetUseCases (out camUseCases))
            {
                Console.WriteLine ("failed to get available use cases");
                return;
            }


            // set the value camInfo
            if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetCameraInfo (out camInfo))
            {
                Console.WriteLine ("failed to get camera info");
                return;
            }


            // Write the values
            Console.WriteLine ("+----------------------------------+");
            Console.WriteLine ("|       Camera information         |");
            Console.WriteLine ("+----------------------------------+");
            Console.WriteLine ("Id:              " + camId);
            Console.WriteLine ("Type:            " + camName);
            Console.WriteLine ("Width:           " + camMaxSensorWidth);
            Console.WriteLine ("Height:          " + camMaxSensorHeight);
            Console.WriteLine ("Operation modes: " + camUseCases.Count());

            // temp variables
            string listIndent = "    ";
            string noteIndent = "        ";
            uint streamCount;

            // loop over every usecase and request how many streams exists associated with this use case
            for (int i = 0; i < camUseCases.Count(); i++)
            {
                // Write the usecase
                Console.WriteLine (listIndent + camUseCases[i]);

                // set the value streamCount
                if (RoyaleDotNet.CameraStatus.SUCCESS != camDevice.GetNumberOfStreams (out streamCount, camUseCases[i]))
                {
                    Console.WriteLine ("failed to get the number of streams or there are no streams");
                }
                else if (streamCount >= 1)
                {
                    Console.WriteLine (noteIndent + "this operation mode has " + streamCount + " stream(s)");
                }
            }

            // loop over every camera information and print it
            Console.WriteLine ("CameraInfo items: " + camInfo.Count());
            for (int i = 0; i < camInfo.Count(); i++)
            {
                Console.WriteLine (listIndent + camInfo[i]);
            }
        }

        static void Main (string[] args)
        {
            // Write an informal header
            WriteHeader();

            // Write the used library version
            WriteVersion();

            // Write the camera information for the first CameraDevice
            WriteCameraInfo ();
        }
    }
}
