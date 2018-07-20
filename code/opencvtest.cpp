#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/ximgproc/disparity_filter.hpp"
#include <iostream>
#include <string>
#include <math.h>

// HTTP POST 통신을 위한 헤더파일 추가
#include <Windows.h>
#include <winhttp.h>
#pragma comment (lib, "winhttp.lib")

using namespace cv;
using namespace cv::ximgproc;
using namespace std;
Mat cameraMatrix = Mat::eye(3, 3, CV_64FC1);
Mat distCoeffs = Mat::zeros(1, 5, CV_64FC1);
Mat image01, image02;


int main(int, char**)
{

	cameraMatrix = (Mat1d(3, 3) << 8.6284313724313131e+02, 0., 640., 0., 8.6284313724313131e+02, 360., 0., 0., 1.);
	distCoeffs = (Mat1d(1, 5) << -4.2037029317618180e-01, 2.6770730624131461e-01, 0., 0., -1.2516364113555581e-01);

	// HTTP POST 통신을 위한 변수 선언
	DWORD dwSize = 0;   // 응답받은 데이터 사이즈
	DWORD dwDownloaded = 0;   // 응답받은 데이터
	LPSTR pszOutBuffer;      // 응답 버퍼
	BOOL  bResults = FALSE;   // 응답 결과 [True, False]
							  // Http 통신을 위한 변수 [1]session -> [2]connect -> [3]request 순서로 진행
	HINTERNET  hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;

	// Use WinHttpOpen to obtain a session handle. (세션핸들 생성)
	hSession = WinHttpOpen(L"WinHTTP Example/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
		
	// Specify an HTTP server. (푸쉬 알람을 위해 FCM 구글 api 서버로 연결)
	if (hSession)
		hConnect = WinHttpConnect(hSession, L"fcm.googleapis.com",
			INTERNET_DEFAULT_HTTPS_PORT, 0);

	// Create an HTTP request handle. (HTTP reqeust 핸들 생성, POST방식으로 FCM 클라우드 메시징 전송 API로 연결)
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/fcm/send",
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);

	// POST Parameter 변수 ( Header 및 Body 데이터 )
	WCHAR szHeader[] = L"Content-Type: application/json\r\nAuthorization: key=AAAA3lluOhA:APA91bGj7qukQdrnf2oSK34jjBapwsfAHtPaqPaRiSrHV2Fd0CEheUzT-g9C4gLhrFg0fM_xTW9QTqubU0yov004tI_nk_dk-KOSDn-Rm5iPwzfCXQ7VdNDY8Ps9NFWhQULg6N3HicAF\r\n";
	CHAR szData[] = "{\"to\" : \"e-qR7hLSLCQ:APA91bGmrK1kaRLUoJQhalFYq1Lf9IKyKWA6WXnUF9zh3FOYfk3C5_oJX7SxMc8mj1WvXyPH57iC2OJbu6sLOhc9_tuy1xN-3NVHpYI9dmV7FPoiij0Odfjp_FHA1DPknotgY7mse3q3\",\"priority\" : \"high\",\"notification\" : {\"body\" : \"Watch Out!!\",\"title\" : \"Smombie\",\"sound\" : \"default\"},\"data\" : {\"title\" : \"Smombie\",\"message\" : \"Watch Out!!\"}}";
	DWORD dwHeaderLength = lstrlenW(szHeader);   // 헤더 길이
	DWORD dwDataLength = lstrlenA(szData);   // 바디 길이

											 
	// 웹 스트리밍 영상 수신을 위한 비디오 변수
	VideoCapture vcap01;
	VideoCapture vcap02;


	//String videoStreamAddress01 = "http://firegot.mooo.com:8080/?action=stream";
	//String videoStreamAddress02 = "http://firegot.mooo.com:8554/?action=stream";
	String videoStreamAddress01 = "http://192.168.43.221:8080/?action=stream";
	String videoStreamAddress02 = "http://192.168.43.221:8554/?action=stream";
	//String videoStreamAddress01 = "http://172.30.1.5:8080/?action=stream";
	//String videoStreamAddress02 = "http://172.30.1.5:8554/?action=stream";

	//parameters for stereo matching and filtering
	double vis_mult = 5.0;
	int wsize = 13;
	int max_disp = 16 * 10;
	double lambda = 10000.0;
	double sigma = 1.0;

	//Some object instatiation that can be done only once

	Mat image01, image02, g1, g2, raw_disp_via;
	Mat leftDisp, rightDisp, filteredSDisp;
	Mat left_for_matcher, right_for_matcher;
	Mat left_disp, right_disp;
	Mat filtered_disp;
	Rect ROI;
	Mat filtered_disp_vis;


	Ptr<StereoSGBM> left_matcher = StereoSGBM::create(23, 96, 151); // 마지막 수정 151
	//Ptr<StereoBM> left_matcher = StereoBM::create( max_disp, 7);
	Ptr<ximgproc::DisparityWLSFilter> wls_filter = ximgproc::createDisparityWLSFilter(left_matcher);
	Ptr<StereoMatcher> right_matcher = ximgproc::createRightMatcher(left_matcher);

	 left_matcher->setUniquenessRatio(10);
	left_matcher->setDisp12MaxDiff(1000000); 
	left_matcher->setSpeckleWindowSize(0);
	left_matcher->setP1(24 * wsize*wsize);
	left_matcher->setP2(96 * wsize*wsize);
    left_matcher->setPreFilterCap(63);
	left_matcher->setMode(StereoSGBM::MODE_SGBM_3WAY); 
	
	wls_filter = createDisparityWLSFilterGeneric(false);
	wls_filter->setDepthDiscontinuityRadius((int)ceil(0.5*wsize));

	if (!vcap01.open(videoStreamAddress01)) {
		cout << "Error opening video stream or file" << endl;
		return -1;
	}

	if (!vcap02.open(videoStreamAddress02)) {
		cout << "Error opening video stream or file" << endl;
		return -1;
	}

	// Image capture and visualization
   for (;;) {
		if (!vcap01.read(image01)) {
			cout << "No frame01" << endl;
			waitKey();
		}

		if (!vcap02.read(image02)) {
			cout << "No frame02" << endl;
			waitKey();
		}
		
		imshow("Output Window01", image01);
		imshow("Output Window02", image02);

		cvtColor(image01, image01, COLOR_BGR2GRAY);
		cvtColor(image02, image02, COLOR_BGR2GRAY);

		// sgbm->compute(image01, image02, disp);
		left_matcher->compute(image01, image02, leftDisp);
		right_matcher->compute(image02, image01, rightDisp);

		//! [filtering]
		wls_filter->setLambda(8000); //double lambda = 10000.0;
		wls_filter->setSigmaColor(1); // double sigma = 1.0;
		wls_filter->filter(leftDisp, image01, filteredSDisp, rightDisp);
		//! [filtering]

		ximgproc::getDisparityVis(leftDisp, raw_disp_via, 1);
		//imshow("Output Window03", raw_disp_via);
		
		getDisparityVis(filteredSDisp, filtered_disp_vis, 1.0);
		
		//화면 띄워 확인하는 용도
		namedWindow("filtered disparity", WINDOW_AUTOSIZE);
		imshow("filtered disparity", filtered_disp_vis);
		//! [visualization]

	
		double minVal, maxVal;
		Point minLoc, maxLoc;
		minMaxLoc(filteredSDisp, &minVal, &maxVal, &minLoc, &maxLoc);

		cout << "max val : " << maxVal << endl;	// 340 x 280 170 140
		
		// maxValue가 기준치 넘은 경우 FCM 메시지 발송
		if (maxVal < 400) {

			// Google API에 세팅된 Parameter와 함께 HTTP POST 전송
			if (hRequest) {
				bResults = WinHttpSendRequest(hRequest,
					szHeader, dwHeaderLength,
					szData, dwDataLength,
					dwDataLength, 0);
			}

			// End the request.

			if (bResults)
				bResults = WinHttpReceiveResponse(hRequest, NULL);

			// Keep checking for data until there is nothing left.
			if (bResults)
			{
				
				do
				{
					// Check for available data.
					dwSize = 0;
					if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
						printf("Error %u in WinHttpQueryDataAvailable.\n",
							GetLastError());

					// Allocate space for the buffer.
					pszOutBuffer = new char[dwSize + 1];
					if (!pszOutBuffer)
					{
						printf("Out of memory\n");
						dwSize = 0;
					}
					else
					{
						// Read the data.
						ZeroMemory(pszOutBuffer, dwSize + 1);

						if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
							dwSize, &dwDownloaded))
							printf("Error %u in WinHttpReadData.\n", GetLastError());
						else
							// printf("%s", pszOutBuffer);
							printf("위험 ! 푸쉬알림 \n");
						// Free the memory allocated to the buffer.
						delete[] pszOutBuffer;
					}
				} while (dwSize > 0);
				

				Sleep(2000);   // FCM 메시지가 무차별적으로 전송되는것을 방지 하기 위해 3초 Sleep
				if (!vcap01.open(videoStreamAddress01)) {
					cout << "Error opening video stream or file" << endl;
					return -1;
				}

				if (!vcap02.open(videoStreamAddress02)) {
					cout << "Error opening video stream or file" << endl;
					return -1;
				}
			}

			// Report any errors.
			if (!bResults)
				printf("Error %d has occurred.\n", GetLastError());

		}

		if (waitKey(1) >= 0) break;
	}

	// Close any open handles. (열려있는 핸들이 있는 경우 프로그램 종료 전에 닫아줌)
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return 0;


	waitKey(0);
}
