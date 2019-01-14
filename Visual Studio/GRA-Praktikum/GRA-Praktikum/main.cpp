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

#define debug false

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
		* Praktikum 1/2/3 Inhaltliche Veraenderungen
		\****************************************************************************/

		show_gray_picture();
		show_depth_picture();

		// Nur Video aufnehmen, wenn es auch der Parameter sagt
		if (mode == 2) {
			write_video_files();
		}

		// eig. nur wenn Eingabe = 1
		glaettung_grauwerte();
		if (glaettung_frame > 21) {
			auswertung_geglaettete_grauwerte();
		}

		/****************************************************************************\
		* Praktikum 1/2/3 Inhaltliche Veraenderungen
		* ENDE
		\****************************************************************************/


	}

	void setLensParameters(const royale::LensParameters &lensParameters) {
		cameraMatrix = (cv::Mat1d(3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
			0, lensParameters.focalLength.second, lensParameters.principalPoint.second, 0, 0, 1);

		distortionCoefficients = (cv::Mat1d(1, 5) << lensParameters.distortionRadial[0], lensParameters.distortionRadial[1],
			lensParameters.distortionTangential.first, lensParameters.distortionTangential.second, lensParameters.distortionRadial[2]);
	}


	/****************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
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
        if (debug) {
            cv::imshow("Gray (Edit)", grayImage_edit);
        }
	}

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
        if (debug) {
            cv::imshow("Depth (Edit)", zImage_edit);
        }
	}

	void open_video_files(string filename, cv::Size bild_groesse, double fps) {
		file_gray = filename + "_gray.avi";
		file_depth = filename + "_depth.avi";

		// VideoWriter fuer Graubild oeffnen
		vw_gray.open(file_gray, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, false);
		if (!vw_gray.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for GrayScale" << endl;
			exit(1);
		}

		// VideoWriter fuer Tiefenbild oeffnen
		vw_depth.open(file_depth, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, true);
		if (!vw_depth.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for Depth" << endl;
			exit(1);
		}
	}

	void write_video_files() {
		vw_gray.write(grayImage_edit);
		vw_depth.write(zImage_edit);
	}

	void close_video_files() {
		vw_gray.release();
		vw_depth.release();
	}

	string get_file_gray() { return file_gray; }
	string get_file_depth() { return file_depth; }
	void setMode(int nmode) { mode = nmode; }

	// Praktikum 2
	void setFrame(int frame) { glaettung_frame = frame; }
	void set_mit_ueber_20_frames(vector<int> v) { mit_ueber_20_frames = v; };

	void glaettung_grauwerte() {
		if (glaettung_frame < 21) {
			// Fuer die ersten 20 Frames Mittelwert berechnen
			for (int i = 0; i < grayImage_edit.rows; i++) {
				for (int j = 0; j < grayImage_edit.cols; j++) {
					mit_ueber_20_frames[i*grayImage_edit.rows + j] = mit_ueber_20_frames[i*grayImage_edit.rows + j] + grayImage_edit.at<uchar>(i, j);
				}
			}
			cout << "Frame no. " << glaettung_frame << endl;
			glaettung_frame++;
		}
		else {
			// Linienprofil-Bild (Breite * Hoehe => 256 * [Hoehe des Graubilds])
			cv::Mat linienprofil(256, grayImage_edit.rows, CV_8UC3);

			// Irgendein zufaelliger Wert, der zwischen 0-Breite liegt
			uchar x_wert = grayImage_edit.rows / 2;

			// "Anwendung" Mittelwert über 20 Frames => klappt nicht ganz, irgendwas ist da kaputt!
			if (glaettung_frame == 21) {
				mit20_bild = cv::Mat(grayImage_edit.rows, grayImage_edit.cols, CV_8UC1);
				for (int i = 0; i < mit_ueber_20_frames.size(); i++) {
					mit_ueber_20_frames[i] = (int)mit_ueber_20_frames[i] / 20;
				}
				
				for (int i = 0; i < mit20_bild.rows; i++) {
					for (int j = 0; j < mit20_bild.cols; j++) {
						mit20_bild.at<uchar>(i, j) = mit_ueber_20_frames[i*grayImage_edit.rows + j];
					}
				}
				glaettung_frame++;
			}
            if (debug) {
                cv::imshow("Mit20", mit20_bild);
            }

			// Anwendung Medianfilter (Groesse 3x3)
			cv::Mat median_bild = grayImage_edit.clone();
			cv::medianBlur(grayImage_edit, median_bild, 3);
			med_bild = median_bild.clone();
            if (debug) {
                cv::imshow("Med", med_bild);
            }

			// Anwendung Mittelwertfilter
			cv::Mat mittelwert_bild = grayImage_edit.clone();
			cv::blur(grayImage_edit, mittelwert_bild, cv::Size(3, 3));
			mit_bild = mittelwert_bild.clone();
            if (debug) {
                cv::imshow("Mit", mit_bild);
            }

			// Linienprofil fuer Mittelwert (20 Frames) zeichnen
			cv::Scalar m20(255, 0, 0);
			for (uchar i = 0; i < linienprofil.cols - 1; i++) {
				cv::line(linienprofil, cv::Point(i, mit20_bild.at<uchar>(x_wert, i)), cv::Point(i + 1, mit20_bild.at<uchar>(x_wert, i + 1)), m20);
			}

			// Linienprofil fuer Medianfilter zeichnen
			cv::Scalar med(0, 255, 0);
			for (uchar i = 0; i < linienprofil.cols-1; i++) {
				cv::line(linienprofil, cv::Point(i, median_bild.at<uchar>(x_wert, i)), cv::Point(i+1, median_bild.at<uchar>(x_wert, i+1)), med);
			}

			// Linienprofil fuer Mittelwertfilter zeichen
			cv::Scalar mit(0, 0, 255);
			for (uchar i = 0; i < linienprofil.cols - 1; i++) {
				cv::line(linienprofil, cv::Point(i, mittelwert_bild.at<uchar>(x_wert, i)), cv::Point(i + 1, mittelwert_bild.at<uchar>(x_wert, i + 1)), mit);
			}

			cv::imshow("Linienprofil", linienprofil);
		}
	}

	void auswertung_geglaettete_grauwerte() {
		// SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
		// Schwellwertsegmentierung (OTSU)
		// ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
		cv::Mat grau_otsu = med_bild.clone();
		cv::threshold(med_bild, grau_otsu, 0, 255, CV_THRESH_OTSU);
        
        if (debug) {
            cv::imshow("OTSU", grau_otsu);
        }

		// Segmentierte Regionen labeln -> Pixel zu Connected Components zusammenfassen
		cv::Mat label_image, stats, centroid;
		int labels = cv::connectedComponentsWithStats(grau_otsu, label_image, stats, centroid, 8, CV_32S);
		vector<int> tasten_labels;

		for (int i = 1; i < labels; i++) {
            if (debug) {
                cout << "\nComponent " << i << std::endl;
                cout << "CC_STAT_LEFT   = " << stats.at<int>(i, cv::CC_STAT_LEFT) << endl;      // X-Koordinate
                cout << "CC_STAT_TOP    = " << stats.at<int>(i, cv::CC_STAT_TOP) << endl;       // Y-Koordinate
                cout << "CC_STAT_WIDTH  = " << stats.at<int>(i, cv::CC_STAT_WIDTH) << endl;     // Breite
                cout << "CC_STAT_HEIGHT = " << stats.at<int>(i, cv::CC_STAT_HEIGHT) << endl;    // Höhe
                cout << "CC_STAT_AREA   = " << stats.at<int>(i, cv::CC_STAT_AREA) << endl;      // Fläche
            }
            
            // Hier muss irgendwie aussortiert werden, welche Components passen und welche nicht :>
			if (stats.at<int>(i, cv::CC_STAT_AREA) > 500 && stats.at<int>(i, cv::CC_STAT_AREA) < 1500) {
				tasten_labels.push_back(i);
			}
		}
		if (tasten_labels.size() != 8) {
			// Es wurden mehr oder weniger als 8 Tasten erkannt :(
		}

        // Gelabelte Connected Components sortieren
        // Das hier funktioniert (nur) bei "kb_portrait_gray.avi"
        sort(tasten_labels.begin(), tasten_labels.end(), [stats](int a, int b) -> bool {
            return (stats.at<int>(a, CC_STAT_TOP) > stats.at<int>(b, CC_STAT_TOP)) ? true : false;
        });
        
        if (debug) {
            cout << endl;
            for (int i=0; i<tasten_labels.size(); i++) {
                cout << "Sortiertes Top (" << i+1 << "):" << stats.at<int>(tasten_labels[i], CC_STAT_TOP) << endl;
            }
            cout << endl;
        }
        
        // Tasten je nach Wert in Schleife Farbe zuweisen und einfaerben
        Mat colored_frame = grayImage.clone();
        
        for (int i=0; i<tasten_labels.size(); i++) {
            int x = stats.at<int>(tasten_labels[i], CC_STAT_LEFT);
            int y = stats.at<int>(tasten_labels[i], CC_STAT_TOP);
            int x2 = x + stats.at<int>(tasten_labels[i], CC_STAT_WIDTH);
            int y2 = y + stats.at<int>(tasten_labels[i], CC_STAT_HEIGHT);
            int color = (256/(tasten_labels.size() + 2))*(i+1);
            
            rectangle(colored_frame, Point(x, y), Point(x2, y2), Scalar(color, color, color), FILLED);
            
            string letters[] = {
                "A", "B", "C", "D", "E", "F", "G", "H"
            };
            putText(colored_frame, letters[i], Point(20, (8-i)*20), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(128, 128, 128));
            
            imshow("Eingezeichnete CCs", colored_frame);
            waitKey(100);
        }
	}

	/****************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\****************************************************************************/


private:
	cv::Mat zImage                                  // Das Tiefenbild, vor Kontrastspreizung, Skalierung und Einfaerbung
    cv::Mat grayImage;                              // Das Grauwertbild, vor Kontrastspreizung und Skalierung
	cv::Mat cameraMatrix, distortionCoefficients;
	mutex flagMutex;


	/****************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\****************************************************************************/

	// Praktikum 1
	cv::Mat zImage_edit                             // Das Tiefenbild, nach Kontrastspreizung, Skalierung und Einfaerbung
    cv::Mat grayImage_edit;                         // Das Grauwertbild, nach Kontrastspreizung und Skalierung
	cv::VideoWriter vw_gray, vw_depth;              // Die VideoWriter zum abspeichern der Videos!
    string file_depth;                              // Der Dateiname fuer das Tiefenbild
	string file_gray;                               // Der Dateiname fuer das Grauwertbild
	int mode;                                       // Der Modus, in dem das Programm ablaeuft (Wiedergabe, Aufnahme, Auswertung)

	// Praktikum 2
	cv::Mat mit20_bild                              // Das Grauwertbild, das ueber 20 Frames gemittelt ist (wird nicht weiter verwendet)
    vector<int> mit_ueber_20_frames;                // Der Vektor fuer die 20 Frames gemittelt (wird nicht weiter verwendet)
    int glaettung_frame;                            // Gibt an, wie viele Frames durchlaufen wurden, um 20 Frames zu mitteln (wird nicht weiter verwendet)
    cv::Mat med_bild                                // Das Grauwertbild, das ueber den Median ermittelt wurde => WIRD WEITER VERWENDET!
    cv::Mat mit_bild;                               // Das Grauwertbild, das ueber den Mittelwert ermittelt wurde (wird nicht weiter verwendet)

	/****************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\****************************************************************************/


};


int main(int argc, char *argv[]) {
	MyListener listener;


	/****************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\****************************************************************************/

	int param = 0;
	string filename = "test"; // Default-Wert ggf. loeschen!

    // Parameter abfragen & enrsprechend handeln
	if (argc > 1) {
		param = atoi(argv[1]);

		if (param == 3) {
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
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
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

	// Ganz normales Vorgehen nach Code-Geruest!
	// start capture mode
	if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error starting the capturing" << endl;
		return 1;
	}


	/****************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\****************************************************************************/

	listener.setMode(param);
	listener.setFrame(0);
	uint16_t height, width;
    cameraDevice->getMaxSensorWidth(width);
    cameraDevice->getMaxSensorHeight(height);
	vector<int> v;
	for (int i = 0; i < width*height; i++) {
		v.push_back(0); // vlt. geht das irgendwie einfacher, die muessen ja alle einfach nur 0 sein
	}
	listener.set_mit_ueber_20_frames(v);

	if (param == 2) {
		// Zum aufnehmen bereit machen
		uint16_t fps;
		cv::Size bild(width, height);
		cameraDevice->getFrameRate(fps);
		listener.open_video_files(filename, bild, fps);
		cout << "Videoaufnahme gestartet" << endl;
	} else if (param == 1) {
		// Auswertung passiert genau hier
		cout << "Aufruf der Auswertung (kommt spaeter)" << endl;
		return 0;
	}

	cout << "Fenster oeffnen, damit Abbrechen mit Enter-Taste funktioniert" << endl;
	cv::namedWindow("Gray (Edit)");
	cv::namedWindow("Depth (Edit)");

	// "Endlosschleife" fuer Anzeige der Tiefen- / Grauwertbilder usw. sowie Aufnahme der Bilder
	for (;;) { if (cv::waitKey(1) == 13) { break; } }

	if (param == 2) {
		listener.close_video_files();
		cout << "Videoaufnahme gestoppt" << endl;
	}

	cv::destroyAllWindows();
	cout << "Alle bestehenden Fenster schliessen" << endl;

	/****************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\****************************************************************************/


	// stop capture mode
	if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error stopping the capturing" << endl;
		return 1;
	}

	return 0;
}
