#pragma once
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <ctype.h>

#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>

#include <pcl\io\io.h>
#include <pcl\io\pcd_io.h>
#include <pcl\point_types.h>
#include <pcl\visualization\cloud_viewer.h>
#include <pcl\stereo\disparity_map_converter.h>

using namespace std;
using namespace cv;

class Calibrate {

private:
	VideoCapture camera0;
	VideoCapture camera1;
	Mat camera_mat0;
	Mat camera_mat1;
	vector<string> imageList;
	const int times = 16;
	int time = 0;

private:
	float squareSize = 1.f;
	bool displayCorners = true;
	bool useCalibrated = true;
	bool showRectified = true;
	bool saveTestImg = true;
	Size boardSize = Size(6, 9);
	const int maxScale = 2;


public:
	Calibrate() {}
	~Calibrate() {}


	bool openCamera(void) {
		camera0.open(0);
		camera1.open(1);
		if (!camera0.isOpened()) {
			std::cout << "Failed to open camera\n";
			return false;
		}
		if (!camera1.isOpened()) {
			std::cout << "Failed to open camera\n";
			return false;
		}
		return true;
	}


	static bool readStringList(const string& filename, vector<string>& l) {
		l.clear();
		FileStorage fs(filename, FileStorage::READ);
		if (!fs.isOpened()) {
			return false;
		}
		FileNode n = fs.getFirstTopLevelNode();
		if (n.type() != FileNode::SEQ) {
			return false;
		}
		FileNodeIterator it = n.begin(), it_end = n.end();
		for (; it != it_end; ++it) {
			l.push_back((string)*it);
		}
		return true;
	}


	void saveTestImage(void) {
		if (openCamera()) {
			while (true) {
				camera0 >> camera_mat0;
				camera1 >> camera_mat1;
				imshow("camera 0", camera_mat0);
				imshow("camera 1", camera_mat1);
				char key = (char)waitKey(30);
				if (key == ' ') {
					saveImage(time);
					time++;
				}
				if (key == 27 || time == times) {
					break;
				}
			}
		}
		else {
			cout << "FAILED TO OPEN CAMERAS" << endl;
		}
	}


	void saveImage(int id) {
		imwrite(imageList[id*2+0], camera_mat0);
		imwrite(imageList[id*2+1], camera_mat1);
		cout << "SAVE IMAGES TO FILES NO _" << id << "_" << endl;
	}


	vector<Point3f> Create3DChessboardCorners(Size boardSize, float squareSize) {
		vector<Point3f> corners;
		for (int i = 0; i < boardSize.height; i++) {
			for (int j = 0; j < boardSize.width; j++) {
				corners.push_back(Point3f(float(j*squareSize),
					float(i*squareSize), 0));
			}
		}
		return corners;
	}


	void stereoCal(void) {
		readStringList("imageList.xml", imageList);
		if (saveTestImg) {
			saveTestImage();
		}
		if (imageList.size() % 2 != 0) {
			cout << "Error: the image list contains odd (non-even) number of elements\n";
		}

		vector<vector<Point2f>> imagePoints[2];
		vector<vector<Point3f>> objectPoints;
		Size imageSize;

		int i, j, k, nimages = (int)imageList.size() / 2;

		imagePoints[0].resize(nimages);
		imagePoints[1].resize(nimages);
		vector<string> goodImageList;

		for (i = j = 0; i < nimages; i++) {
			for (k = 0; k < 2; k++) {
				const string filename = imageList[i * 2 + k];
				Mat img = imread(filename, 0);
				if (img.empty())
					break;
				if (imageSize == Size())
					imageSize = img.size();
				else if (img.size() != imageSize) {
					cout << "The image " << filename << " has the size different from the first image size. Skipping the pair\n";
					break;
				}
				bool found = false;
				vector<Point2f>& corners = imagePoints[k][j];
				for (int scale = 1; scale <= maxScale; scale++)
				{
					Mat timg;
					if (scale == 1)
						timg = img;
					else
						resize(img, timg, Size(), scale, scale);
					found = findChessboardCorners(timg, boardSize, corners,
						CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE);
					if (found)
					{
						if (scale > 1)
						{
							Mat cornersMat(corners);
							cornersMat *= 1. / scale;
						}
						break;
					}
				}
				if (displayCorners)
				{
					cout << filename << endl;
					Mat cimg, cimg1;
					cvtColor(img, cimg, COLOR_GRAY2BGR);
					drawChessboardCorners(cimg, boardSize, corners, found);
					double sf = 640. / MAX(img.rows, img.cols);
					resize(cimg, cimg1, Size(), sf, sf);
					imshow("corners", cimg1);
					char c = (char)waitKey(500);
					if (c == 27 || c == 'q' || c == 'Q')
						exit(-1);
				}
				else
					putchar('.');
				if (!found)
					break;
				cornerSubPix(img, corners, Size(11, 11), Size(-1, -1),
					TermCriteria(TermCriteria::COUNT + TermCriteria::EPS,
						30, 0.01));
			}
			if (k == 2)
			{
				goodImageList.push_back(imageList[i * 2]);
				goodImageList.push_back(imageList[i * 2 + 1]);
				j++;
			}
		}
		cout << j << " pairs have been successfully detected.\n";
		nimages = j;
		if (nimages < 2)
		{
			cout << "Error: too little pairs to run the calibration\n";
			return;
		}

		imagePoints[0].resize(nimages);
		imagePoints[1].resize(nimages);
		objectPoints.resize(nimages);

		for (i = 0; i < nimages; i++)
		{
			for (j = 0; j < boardSize.height; j++)
				for (k = 0; k < boardSize.width; k++)
					objectPoints[i].push_back(Point3f(k*squareSize, j*squareSize, 0));
		}

		cout << "Running stereo calibration ...\n";

		Mat cameraMatrix[2], distCoeffs[2];
		cameraMatrix[0] = initCameraMatrix2D(objectPoints, imagePoints[0], imageSize, 0);
		cameraMatrix[1] = initCameraMatrix2D(objectPoints, imagePoints[1], imageSize, 0);
		Mat R, T, E, F;
		// R - output rotation matrix between the 1st and the 2nd camera coordinate systems
		// T - output translation vector between the coordinate systems of the cameras
		// E - output essential matrix
		// F - output fundamental matrix

		double rms = stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1],
			cameraMatrix[0], distCoeffs[0],
			cameraMatrix[1], distCoeffs[1],
			imageSize, R, T, E, F,
			CALIB_FIX_ASPECT_RATIO +
			CALIB_ZERO_TANGENT_DIST +
			CALIB_USE_INTRINSIC_GUESS +
			CALIB_SAME_FOCAL_LENGTH +
			CALIB_RATIONAL_MODEL +
			CALIB_FIX_K3 + CALIB_FIX_K4 + CALIB_FIX_K5,
			TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 100, 1e-5));
		cout << "done with RMS error=" << rms << endl;

		// CALIBRATION QUALITY CHECK
		double err = 0;
		int npoints = 0;
		vector<Vec3f> lines[2];
		for (i = 0; i < nimages; i++)
		{
			int npt = (int)imagePoints[0][i].size();
			Mat imgpt[2];
			for (k = 0; k < 2; k++)
			{
				imgpt[k] = Mat(imagePoints[k][i]);
				undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], Mat(), cameraMatrix[k]);
				computeCorrespondEpilines(imgpt[k], k + 1, F, lines[k]);
			}
			for (j = 0; j < npt; j++)
			{
				double errij = fabs(imagePoints[0][i][j].x*lines[1][j][0] +
					imagePoints[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
					fabs(imagePoints[1][i][j].x*lines[0][j][0] +
						imagePoints[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
				err += errij;
			}
			npoints += npt;
		}
		cout << "average epipolar err = " << err / npoints << endl;

		// Save intrinsic parameters
		FileStorage fs("intrinsics.yml", FileStorage::WRITE);
		if (fs.isOpened())
		{
			fs << "M1" << cameraMatrix[0] << "D1" << distCoeffs[0] <<
				"M2" << cameraMatrix[1] << "D2" << distCoeffs[1];
			fs.release();
		}
		else
			cout << "Error: can not save the intrinsic parameters\n";

		Mat R1, R2, P1, P2, Q;
		Rect validRoi[2];
		// R1 - output 3*3 rectification transform (rotation matrix) for the first camera
		// R2 - output 3*3 rectification transform (rotation matrix) for the second camera
		// P1 - output 3*4 projection matrix in the new (rectified) coordinate systems for the first camera
		// P2 - output 3*4 projection matrix in the new (rectified) coordinate systems for the second camera
		// Q - output 4*4 disparity-to-depth mapping matrix

		stereoRectify(cameraMatrix[0], distCoeffs[0],
			cameraMatrix[1], distCoeffs[1],
			imageSize, R, T, R1, R2, P1, P2, Q,
			CALIB_ZERO_DISPARITY, 1, imageSize, &validRoi[0], &validRoi[1]);

		fs.open("extrinsics.yml", FileStorage::WRITE);
		if (fs.isOpened()) {
			fs << "R" << R << "T" << T << "R1" << R1 << "R2" << R2 << "P1" << P1 << "P2" << P2 << "Q" << Q;
			fs.release();
		}
		else
			cout << "Error: can not save the extrinsic parameters\n";

		bool isVerticalStereo = fabs(P2.at<double>(1, 3)) > fabs(P2.at<double>(0, 3));

		// COMPUTE AND DISPLAY RECTIFICATION
		if (!showRectified)
			return;

		Mat rmap[2][2];
		if (useCalibrated) {
		} else {
			vector<Point2f> allimgpt[2];
			for (k = 0; k < 2; k++) {
				for (i = 0; i < nimages; i++)
					std::copy(imagePoints[k][i].begin(), imagePoints[k][i].end(), back_inserter(allimgpt[k]));
			}
			F = findFundamentalMat(Mat(allimgpt[0]), Mat(allimgpt[1]), FM_8POINT, 0, 0);
			Mat H1, H2;
			stereoRectifyUncalibrated(Mat(allimgpt[0]), Mat(allimgpt[1]), F, imageSize, H1, H2, 3);

			R1 = cameraMatrix[0].inv()*H1*cameraMatrix[0];
			R2 = cameraMatrix[1].inv()*H2*cameraMatrix[1];
			P1 = cameraMatrix[0];
			P2 = cameraMatrix[1];
		}

		//Precompute maps for cv::remap()
		initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_16SC2, rmap[0][0], rmap[0][1]);
		initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_16SC2, rmap[1][0], rmap[1][1]);

		Mat canvas;
		double sf;
		int w, h;
		if (!isVerticalStereo) {
			sf = 600. / MAX(imageSize.width, imageSize.height);
			w = cvRound(imageSize.width*sf);
			h = cvRound(imageSize.height*sf);
			canvas.create(h, w * 2, CV_8UC3);
		}
		else {
			sf = 300. / MAX(imageSize.width, imageSize.height);
			w = cvRound(imageSize.width*sf);
			h = cvRound(imageSize.height*sf);
			canvas.create(h * 2, w, CV_8UC3);
		}

		for (i = 0; i < nimages; i++) {
			for (k = 0; k < 2; k++) {
				Mat img = imread(goodImageList[i * 2 + k], 0), rimg, cimg;
				remap(img, rimg, rmap[k][0], rmap[k][1], INTER_LINEAR);
				cvtColor(rimg, cimg, COLOR_GRAY2BGR);
				Mat canvasPart = !isVerticalStereo ? canvas(Rect(w*k, 0, w, h)) : canvas(Rect(0, h*k, w, h));
				resize(cimg, canvasPart, canvasPart.size(), 0, 0, INTER_AREA);
				if (useCalibrated) {
					Rect vroi(cvRound(validRoi[k].x*sf), cvRound(validRoi[k].y*sf),
						cvRound(validRoi[k].width*sf), cvRound(validRoi[k].height*sf));
					rectangle(canvasPart, vroi, Scalar(0, 0, 255), 3, 8);
				}
			}

			if (!isVerticalStereo)
				for (j = 0; j < canvas.rows; j += 16)
					line(canvas, Point(0, j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);
			else
				for (j = 0; j < canvas.cols; j += 16)
					line(canvas, Point(j, 0), Point(j, canvas.rows), Scalar(0, 255, 0), 1, 8);
			imshow("rectified", canvas);
			char c = (char)waitKey();
			if (c == 27 || c == 'q' || c == 'Q')
				break;
		}
		destroyAllWindows();
	}


};