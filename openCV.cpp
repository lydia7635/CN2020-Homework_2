#include "opencv2/opencv.hpp"

#include <iostream>

using namespace std;
using namespace cv;

int main(int argc, char** argv){
  
    // server

    Mat imgServer,imgClient;
    VideoCapture cap("./tmp.mpg");
    
    // get the resolution of the video
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    cout  << width << ", " << height << endl;
    
    //allocate container to load frames 
    
    imgServer = Mat::zeros(540,960, CV_8UC3);    
    imgClient = Mat::zeros(540, 960, CV_8UC3);
 
 
     // ensure the memory is continuous (for efficiency issue.)
    if(!imgServer.isContinuous()){
         imgServer = imgServer.clone();
    }

    if(!imgClient.isContinuous()){
         imgClient = imgClient.clone();
    }

    while(1){
        //get a frame from the video to the container on server.
        cap >> imgServer;
        
        // get the size of a frame in bytes 
        int imgSize = imgServer.total() * imgServer.elemSize();
        
        // allocate a buffer to load the frame (there would be 2 buffers in the world of the Internet)
        uchar buffer[imgSize];
        
        // copy a frame to the buffer
        memcpy(buffer,imgServer.data, imgSize);
        
        // copy a fream from buffer to the container on client
        uchar *iptr = imgClient.data;
        memcpy(iptr,buffer,imgSize);
      
        imshow("Video", imgClient);
      //Press ESC on keyboard to exit
      // notice: this part is necessary due to openCV's design.
      // waitKey means a delay to get the next frame.
        char c = (char)waitKey(33.3333);
        if(c==27)
                break;
        }
      ////////////////////////////////////////////////////
	cap.release();
	destroyAllWindows();
	return 0;
}
