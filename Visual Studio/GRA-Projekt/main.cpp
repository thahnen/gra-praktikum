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

        
/****************************************************************************\
 * ANFANG
 * Praktikum 1/2 Inhaltliche Ver�nderungen
\****************************************************************************/

        
		// Was bei jedem neuen Frame gemacht wird
		show_gray_picture();
		show_depth_picture();

        // Nur Video aufnehmen, wenn es auch der Parameter sagt
		if (mode == 2) {
			write_video_files();
		}
        
        
/****************************************************************************\
 * Praktikum 1/2 Inhaltliche Ver�nderungen
 * ENDE
\****************************************************************************/
        
        
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
    
    
/****************************************************************************\
 * ANFANG
 * Praktikum 1/2 Inhaltliche Ver�nderungen
\****************************************************************************/

    
    // Praktikum 1
	void show_gray_picture() {
		// Kontrastspreizung
		cv::Mat mask;
		cv::compare(grayImage, cv::Scalar(0), mask, cv::CMP_GT);
		cv::Mat gray_CV_8UC1(grayImage.rows, grayImage.cols, CV_8UC1);
		double min, max;
		cv::minMaxLoc(grayImage, &min, &max, 0, 0, (cv::InputArray)mask);

		// Skalierung berechnen & Bild skalieren
		double scale = 255 / (max - min);
		cv::convertScaleAbs(grayImage, gray_CV_8UC1, scale);

		grayImage_edit = gray_CV_8UC1;

		// Anzeigen (kann weg)
		cv::imshow("Gray (edit)", grayImage_edit);
		cv::waitKey(1);
	}

    // Praktikum 1
	void show_depth_picture() {
		// Kontrastspreizung
		cv::Mat mask;
		cv::compare(zImage, cv::Scalar(0), mask, cv::CMP_GT);
		cv::Mat depth_CV_8UC1(zImage.rows, zImage.cols, CV_8UC1);
		double min, max;
		cv::minMaxLoc(zImage, &min, &max, 0, 0, (cv::InputArray)mask);

		// Skalierung berechnen & Bild skalieren
		double scale = 255 / (max - min);
		cv::convertScaleAbs(zImage, depth_CV_8UC1, scale);

		// Einfaerbung mit COLORMAP_RAINBOW
		cv::Mat color_map;
		cv::applyColorMap(depth_CV_8UC1, color_map, cv::COLORMAP_RAINBOW);

		zImage_edit = color_map;

		// Anzeigen (kann weg)
		cv::imshow("Depth (Edit)", zImage_edit);
		cv::waitKey(1);
	}

    // Praktikum 1
	void open_video_files(string filename, cv::Size bild_groesse, double fps) {
		string file_g = filename + "_gray.avi";
		string file_d = filename + "_depth.avi";

		file_gray = file_g;
		file_depth = file_d;

        // VideoWriter f�r Graubild �ffnen
		vw_gray.open(file_gray, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, false);
		if (!vw_gray.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for GrayScale" << endl;
			exit(1);
		}
        
        // VideoWriter f�r Tiefenbild �ffnen
		vw_depth.open(file_depth, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, true);
		if (!vw_depth.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for Depth" << endl;
			exit(1);
		}
	}

    // Praktikum 1
	void write_video_files() {
		vw_gray.write(grayImage_edit);
		vw_depth.write(zImage_edit);
	}

    // Praktikum 1
	void close_video_files() {
		vw_gray.release();
		vw_depth.release();
	}

    // Praktikum 1
	string get_file_gray() { return file_gray; }
	string get_file_depth() { return file_depth; }
    // Praktikum 1
	void setMode(int nmode) { mode = nmode; }
    
    // Praktikum 2
    /*
    void glaettung_grauwerte() {
        if (glaettung_frame < 21) {
            // Fuer die ersten 20 Frames Mittelwert berechnen
            glaettung_grauwerte = (glaettung_grauwerte + grayImage_edit)/2;
            glaettung_frame++;
        } else {
            // Anwendung Medianfilter (Groesse 3x3)
            cv::Mat median_bild = grayImage_edit.clone();
            cv::medianBlur(grayImage_edit, median_bild, cv::Size(3, 3));
            // Anwendung Mittelwertfilter
            cv::Mat mittelwert_bild = grayImage_edit.clone();
            cv::blur(grayImage_edit, mittelwert_bild, cv::Size(3, 3));
            
            // Linienprofil f�r Mittelwert (20 Frames) zeichnen
            
            // Linienprofil f�r Medianfilter zeichnen
            
            // Linienprofil f�r Mittelwertfilter zeichen
        }
    }
     */
    
    // Praktikum 2
    /*
    void auswertung_geglaettete_grauwerte() {
        // SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
        // Schwellwertsegmentierung (OTSU) -> Threshold so richtig?
        // ggf. vor OTSU noch ein Closing durchf�hren?
        cv::Mat grau_otsu = grayImage_edit.clone()
        cv::threshold(grayImage_edit, grau_otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
        
        // Schwellwertsegmentierung (Adaptiv) -> ADAPTIVE_THRESH_MEAN_C oder  ADAPTIVE_THRESH_GAUSSIAN_C ?
        // ggf. vor Adaptiv noch ein Closing durchf�hren?
        cv::Mat grau_adaptiv = grayImage_edit.clone();
        cv::adaptiveThreshold(grayImage_edit, grau_adaptiv, 255, ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 3, 1);
        
        // Segmentierte Regionen labeln:
        // -> Pixel zu Connected Components zusammenfassen (anhand welches Eingangsbildes grau_otsu oder grau_adaptiv ?)
        cv::Mat label_image, stat, centroid;
        int labels = cv::connectedComponentsWithStats(grau_otsu, label_image, stat, centroid, 8, CV_32S);
        
        // -> Kleinste X/Y-Koordinate erhalten + H�he/Breite
        
        // Gelabelte Connected Components sortieren
        
        // -> Richtige ausw�hlen (ca. gleiche Abma�e)
        
        // -> je nach Ausrichtung nach X/Y-Koordinate sortieren
        
        // Tasten je nach Wert in Schleife Farbe zuweisen und einf�rben
        for (;;) {
            //
        }
     */


/****************************************************************************\
 * Praktikum 1/2 Inhaltliche Ver�nderungen
 * ENDE
\****************************************************************************/

    
private:
	cv::Mat zImage, grayImage;
	cv::Mat cameraMatrix, distortionCoefficients;
	mutex flagMutex;

    
/****************************************************************************\
 * ANFANG
 * Praktikum 1/2 Inhaltliche Ver�nderungen
\****************************************************************************/
    

    // Praktikum 1
    cv::Mat zImage_edit, grayImage_edit;
	cv::VideoWriter vw_gray, vw_depth;
	string file_gray;
	string file_depth;
	int mode;
    
    // Praktikum 2
    /*
    cv::Mat glaettung_grauwert;
    int glaettung_frame = 1;
     */
    
    
/****************************************************************************\
 * Praktikum 1/2 Inhaltliche Ver�nderungen
 * ENDE
\****************************************************************************/
    
    
};


int main(int argc, char *argv[]) {
	MyListener listener;
    
    
/****************************************************************************\
 * ANFANG
 * Praktikum 1/2 Inhaltliche Ver�nderungen
\****************************************************************************/
    
	int param = 0;
	string filename = "test"; // Default-Wert ggf. l�schen!

	// Parameter abfragen & enrsprechend handeln
	if (argc > 1) {
        param = atoi(argv[1]);
        
        if (param == 3) {
            // Video abspielen und das wars dann
			cout << "Bitte Datei-Namen des fertigen Videos eingeben" << endl;
			cin >> filename;

			cv::VideoCapture cap(filename);
			if (!cap.isOpened()) {
				cerr << "Kann Video nicht wiedergeben" << endl;
				return 1;
			}

			cv::Mat frame;
			for (;;) {
				cap >> frame;
				if (frame.empty()) {
					cerr << "Frame ist leer" << endl;
					break;
				}
				cv::imshow("Video (Tiefen)", frame);
				cv::waitKey(20);
			}
			return 0;
        } else if (param == 2) {
            cout << "Bitte Datei-Praefix des aufnehmenden Videos eingeben" << endl;
            cin >> filename;
        }
	}

    
/****************************************************************************\
 * Praktikum 1/2 Inhaltliche Ver�nderungen
 * ENDE
\****************************************************************************/


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
		} else {
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

	// Ganz normales Vorgehen nach Code-Geruest!
	// start capture mode
	if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error starting the capturing" << endl;
		return 1;
	}


/****************************************************************************\
 * ANFANG
 * Praktikum 1/2 Inhaltliche Ver�nderungen
\****************************************************************************/
    
    
	if (param == 2) {
        // Zum aufnehmen bereit machen
		listener.setMode(param);
		uint16_t height, width, fps;
		cameraDevice->getMaxSensorWidth(width);
		cameraDevice->getMaxSensorHeight(height);
		cv::Size bild(width, height);
		cameraDevice->getFrameRate(fps);
		listener.open_video_files(filename, bild, fps);
		cout << "Videoaufnahme gestartet" << endl;
	} else if (param == 1) {
        // Auswertung passiert genau hier
		cout << "Aufruf der Auswertung (kommt spaeter)" << endl;
		return 0;
	}

    cout << "Fenster �ffnen, damit Abbrechen mit Enter-Taste funktioniert" << endl;
	cv::namedWindow("Gray (Edit)");
	cv::namedWindow("Depth (Edit)");

	// "Endlosschleife" fuer Anzeige der Tiefen- / Grauwertbilder usw. sowie Aufnahme der Bilder
	for (size_t i = 0; i < 10000000000; i++) {
		if (cv::waitKey(1) == 13) {
			break;
		}
	}

	if (param == 2) {
		listener.close_video_files();
		cout << "Videoaufnahme gestoppt" << endl;
	}

	cv::destroyAllWindows();
    cout << "Alle bestehenden Fenster schliessen" << endl;
    
    
/****************************************************************************\
 * Praktikum 1/2 Inhaltliche Ver�nderungen
 * ENDE
\****************************************************************************/
    

	// stop capture mode
	if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error stopping the capturing" << endl;
		return 1;
	}

	return 0;
}
