// LiveImageGrabber.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "HttpGrabber.h"
#include "compat.h"
#include "log.h"
#include "wingetopt.h"

//required for opencv
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//definitions for the host and the url
std::string host = "";
unsigned short port = 80;
std::string url = "axis-cgi/jpg/image.cgi?resolution=640x480";//single jpeg image
//std::string url = "axis-cgi/mjpg/video.cgi?resolution=640x480";//motion-jpeg stream

//number of image to grab
int numImages = 10;

//timeout to wait for in milliseconds answer (0 means to block forever)
unsigned int timeOut = 1000;

//log level: LOG_ERR=0, LOG_INFO=1, LOG_DEBUG=2
TheLogLevel levelToLog = LOG_ERR;

//whether to save the images
bool saveImages = true;

//read command line options (-h option is mandatory)
int getCommandLineOptions(int argc, char* * argv)
{
	int c;
	while ((c = getopt(argc, argv, ":h:p:u:n:t:v:s:")) != EOF) {
		switch (c) {
		case 'h':
			printf("Argument h, host: %s \n", optarg);
			host = optarg;
			break;
		case 'p':
			printf("Argument p, port: %s \n", optarg);
			port = atoi(optarg);
			break;
		case 'u':
			printf("Argument u, uniform resource locator (url): %s \n", optarg);
			url = optarg;
			break;
		case 'n':
			printf("Argument n, number of images: %s \n", optarg);
			numImages = atoi(optarg);
			break;
		case 't':
			printf("Argument t, timeout in milliseconds: %s \n", optarg);
			timeOut = atoi(optarg);
			break;
		case 'v':
			printf("Argument v, log level: %s \n", optarg);
			levelToLog = (TheLogLevel) atoi(optarg);
			break;
		case 's':
			printf("Argument s, save: %s \n", optarg);
			saveImages = atoi(optarg) != 0;
			break;
		default:
			break;
		}
	}
	if (host == "") {
		return -1;
	}
	optind = 1;// reset arg counter
	return 0;
}

//print usage
void usage()
{
	std::cout << "usage:" << std::endl;
	std::cout << "LiveImageGrabber -h host [-p port -u url -n num -l login -w password -t timeout -v loglevel -s save]:" << std::endl;
	std::cout << "host: name or ip address of host (mandatory):" << std::endl;
	std::cout << "port: host port (default: 80)" << std::endl;
	std::cout << "url: uniform resource locator on host  (default: axis-cgi/jpg/image.cgi?resolution=640x480)" << std::endl;
	std::cout << "num: number of images to grab (default: 10)" << std::endl;
	std::cout << "timeout: maximum time in milliseconds to wait for data (default: 1000)" << std::endl;
	std::cout << "loglevel: loglevel (0=error, 1=info, 2=debug) (default: 0)" << std::endl;
	std::cout << "save: save images or not (!=0 -> save) (default: 1)" << std::endl;
	std::cout << std::endl;
	std::cout << "you must specify the host: -h:" << std::endl;
	std::cout << "will exit" << std::endl;

#if defined (_WIN32)
	system("pause");
#endif	
}

int main(int argc, char* argv[])
{	
	int count;	
	bool go = true;
	char* msgBuf = NULL;
	unsigned int msgSize = 0;

	if(getCommandLineOptions(argc, argv) < 0) {
		usage();
		return(-1);
	}

	//start network (win only)
	START_NETWORKING;

	//create the grabber class
	HttpGrabber* grabber = new HttpGrabber(host.c_str(), url.c_str(), port);

	//set log level for output	
	grabber->setLogLevel(levelToLog);

	count = 0;
	while(go) {
		while(grabber->Run(msgBuf, msgSize) == HttpGrabber::SUCCESS) {
			char fileName[20];
			snprintf(fileName, 20, "image%02d.jpg", count);
			if (saveImages) {
				std::ofstream o(fileName, std::ios::out | std::ios::binary);
				o.write(msgBuf, msgSize);
				o.close();
			}
			else {
				log_info("skipping to save file %s", fileName);
			}
                        
                        std::vector<unsigned char> jpgbytes(msgBuf, msgBuf+msgSize);
                        cv::Mat img = cv::imdecode(jpgbytes, CV_LOAD_IMAGE_COLOR);

                        cv::imshow("disp", img);

                        char c = (char) cv::waitKey(1);
                        if (c == 27 || c == 'q' || c == 'Q')
                            break;

                        
			delete[] msgBuf;
			msgBuf = NULL;
			msgSize = 0;

			if (numImages <= (++count)) {//maximum number of image reached -> break					
				go = false;
				break;
			}
		}
		
		//disconnect
		grabber->Disconnect();
	}
	//delete grabber instance
	delete grabber;

	//stop network (win only)
	STOP_NETWORKING;
    return 0;
}

