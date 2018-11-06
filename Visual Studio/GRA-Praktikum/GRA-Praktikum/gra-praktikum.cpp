/****************************************************************************\
* Vorlage fuer das Praktikum "Graphische Datenverarbeitung" WS 2018/19
* FB 03 der Hochschule Niedderrhein
* Regina Pohle-Froehlich
*
* Der Code basiert auf den c++-Beispielen der Bibliothek royale
\****************************************************************************/


#include <royale.hpp>
#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>


using namespace std;


class MyListener : public royale::IDepthDataListener {
public:
	void onNewData(const royale::DepthData *data) {
		// this callback function will be called for every new depth frame

		lock_guard<mutex> lock(flagMutex);
		zImage.create(cv::Size(data->width, data->height), CV_32FC1);
		grayImage.create(cv::Size(data->width, data->height), CV_32FC1);
		zImage = 0;
		grayImage = 0;
		int k = 0;
		for (int y = 0; y < zImage.rows; y++) {
			for (int x = 0; x < zImage.cols; x++) {
				auto curPoint = data->points.at(k);
				if (curPoint.depthConfidence > 0) {
					// if the point is valid
					zImage.at<float>(y, x) = curPoint.z;
					grayImage.at<float>(y, x) = curPoint.grayValue;
				}
				k++;
			}
		}

		cv::Mat temp = zImage.clone();
		undistort(temp, zImage, cameraMatrix, distortionCoefficients);
		temp = grayImage.clone();
		undistort(temp, grayImage, cameraMatrix, distortionCoefficients);

		// Was bei jedem neuen Fram gemacht wird
		show_gray_picture();
		show_depth_picture();
	}

	void setLensParameters(const royale::LensParameters &lensParameters) {
		// Construct the camera matrix
		// (fx   0    cx)
		// (0    fy   cy)
		// (0    0    1 )
		cameraMatrix = (cv::Mat1d(3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
			0, lensParameters.focalLength.second, lensParameters.principalPoint.second,
			0, 0, 1);

		// Construct the distortion coefficients
		// k1 k2 p1 p2 k3
		distortionCoefficients = (cv::Mat1d(1, 5) << lensParameters.distortionRadial[0],
			lensParameters.distortionRadial[1],
			lensParameters.distortionTangential.first,
			lensParameters.distortionTangential.second,
			lensParameters.distortionRadial[2]);
	}

	// Im Praktikum hinzugefügt
	void show_gray_picture() {
		// Kontrastspreizung
		cv::Mat mask;
		cv::compare(grayImage, cv::Scalar(0), mask, cv::CMP_GT);
		cv::Mat gray_CV_8UC1(grayImage.rows, grayImage.cols, CV_8UC1);
		double min, max;
		cv::minMaxLoc(grayImage, &min, &max, 0, 0, (cv::InputArray)mask);
		
		// Skalierung berechnen & Bild skalieren
		double scale = 255/(max-min);
		cv::convertScaleAbs(grayImage, gray_CV_8UC1, scale);

		// Anzeigen (kann weg)
		cv::imshow("Gray", grayImage);
		cv::waitKey(1);
		cv::imshow("Gray (edit)", gray_CV_8UC1);
		cv::waitKey(1);
	}

	void show_depth_picture() {
		// Kontrastspreizung
		cv::Mat mask;
		cv::compare(zImage, cv::Scalar(0), mask, cv::CMP_GT);
		cv::Mat depth_CV_8UC1(zImage.rows, zImage.cols, CV_8UC1);
		double min, max;
		cv::minMaxLoc(zImage, &min, &max, 0, 0, (cv::InputArray)mask);
		
		// Skalierung berechnen & Bild skalieren
		double scale = 255/(max-min);
		cv::convertScaleAbs(zImage, depth_CV_8UC1, scale);

		// Einfärbung mit COLORMAP_RAINBOW
		cv::Mat color_map;
		cv::applyColorMap(depth_CV_8UC1, color_map, cv::COLORMAP_RAINBOW);

		// Anzeigen (kann weg)
		cv::imshow("Depth", zImage);
		cv::waitKey(1);
		cv::imshow("Depth (Edit)", color_map);
		cv::waitKey(1);
	}

	void open_video_files(string filename, double fps, cv::Size frame_size) {
		// uint16_t fps; cameraDevice->getFramerate(fps) -> fps=fps
		// cv::Size bild(zImage.rows, zImage.cols) -> frame_size=bild
		string file_gray = filename + "_gray.avi";
		string file_depth = filename + "_depth.avi";

		vw_gray.open(file_gray, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, frame_size, false);
		vw_depth.open(file_depth, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, frame_size, true);
	}

	void write_video_files() {
		//
	}

	void close_video_files() {
		//
	}

private:
	cv::Mat zImage, grayImage;
	cv::Mat cameraMatrix, distortionCoefficients;
	mutex flagMutex;

	cv::VideoWriter vw_gray, vw_depth;
};


int main(int argc, char *argv[]) {
	MyListener listener;

	// this represents the main camera device object
	unique_ptr<royale::ICameraDevice> cameraDevice;

	// the camera manager will query for a connected camera
	{
		royale::CameraManager manager;

		// try to open the first connected camera
		royale::Vector<royale::String> camlist(manager.getConnectedCameraList());
		std::cout << "Detected " << camlist.size() << " camera(s)." << endl;

		if (!camlist.empty()) {
			cameraDevice = manager.createCamera(camlist[0]);
		}
		else {
			cerr << "No suitable camera device detected." << endl
				<< "Please make sure that a supported camera is plugged in, all drivers are "
				<< "installed, and you have proper USB permission" << endl;
			return 1;
		}

		camlist.clear();
	}

	// the camera device is now available and CameraManager can be deallocated here

	if (cameraDevice == nullptr) {
		// no cameraDevice available
		if (argc > 1) {
			cerr << "Could not open " << argv[1] << endl;
			return 1;
		}
		else {
			cerr << "Cannot create the camera device" << endl;
			return 1;
		}
	}

	// call the initialize method before working with the camera device
	auto status = cameraDevice->initialize();
	if (status != royale::CameraStatus::SUCCESS) {
		cerr << "Cannot initialize the camera device, error string : " << getErrorString(status) << endl;
		return 1;
	}

	// retrieve the lens parameters from Royale
	royale::LensParameters lensParameters;
	status = cameraDevice->getLensParameters(lensParameters);
	if (status != royale::CameraStatus::SUCCESS) {
		cerr << "Can't read out the lens parameters" << endl;
		return 1;
	}

	listener.setLensParameters(lensParameters);

	// register a data listener
	if (cameraDevice->registerDataListener(&listener) != royale::CameraStatus::SUCCESS) {
		cerr << "Error registering data listener" << endl;
		return 1;
	}

	// Belichtung muss auf Automatisch gesetzt werden
	cameraDevice->setExposureMode(royale::ExposureMode::AUTOMATIC);
	
	// start capture mode
	if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error starting the capturing" << endl;
		return 1;
	}


	/*
		Hier alles aus dem Praktikum
	*/
	// Übergabeparameter überprüfen
	// Parameter:
	//		argv[0] -> Programm
	//		argv[1] -> Kamera ODER 1/2/3
	int param = 0;
	if (argc > 1) {
		if (argc == 2) {
			// Programm + Kamera ODER Programm + 1/2/3
			if (argv[1] == "1" || argv[1] == "2" || argv[1] == "3") {
				param = (int)argv[1];
			}
		} else {
			// Programm + Kamera + 1/2/3 [+ ...]
			if (argv[2] == "1" || argv[2] == "2" || argv[2] == "3") {
				param = (int)argv[2];
			}
		}
	}

	if (param != 0) {
		switch (param) {
		case 1:
			std::cout << "Aufruf der Auswertung" << endl;
		case 2:
			// 1. Präfix des Videodateinames einlesen (oder Parameter)
			// 2. Max. Bildgröße und max. Framerate abbfragen -> an MyListener abgeben
			// 3. ggf. Belichtungsmodus setzen
			// 4. Video abspeichern (gewisse Anzahl an Sekunden?)
			std::cout << "Bitte Präfix des Videodateinamens eingeben:" << endl;
		case 3:
			// Wie bei 2 nur noch mit abspielen!
		default:
			break;
		}
	}

	// Die Endlosschleife
	for (;;) {
	}


	// stop capture mode
	if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error stopping the capturing" << endl;
		return 1;
	}

	return 0;
}
