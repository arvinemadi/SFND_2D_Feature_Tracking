/* Code completed by Arvin.Emadi@Gmail.com*/
/* Original uncompleted code from udacity - link of github at the readme*/
/* INCLUDES FOR THIS PROJECT */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"

using namespace std;

/* MAIN PROGRAM */
int main(int argc, const char *argv[])
{

    /* INIT VARIABLES AND DATA STRUCTURES */

    // data location
    string dataPath = "../";

    // camera
    string imgBasePath = dataPath + "images/";
    string imgPrefix = "KITTI/2011_09_26/image_00/data/000000"; // left camera, color
    string imgFileType = ".png";
    int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
    int imgEndIndex = 9;   // last file index to load
    int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)

    // misc
    int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
    vector<DataFrame> dataBuffer; // list of data frames which are held in memory at the same time
    bool bVis = false;            // visualize results
    //Buffer<DataFrame> dataBuffer = new Buffer (dataBufferSize);
    dataBuffer.resize(dataBufferSize);
    int buf_loc = -1;
    int buf_size = 0;

    //defining some vecotrs to store performance for analysis and comparison of various methods
    vector<int> result_num_keypoints_in_ROI;
    vector<int> result_num_matched_keypoints;
    vector<double> result_detection_time;
    vector<double> result_extraction_time;
    
    /* MAIN LOOP OVER ALL IMAGES */
    for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
    {
        /* LOAD IMAGE INTO BUFFER */

        // assemble filenames for current index
        ostringstream imgNumber;
        imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
        string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

        // load image from file and convert to grayscale
        cv::Mat img, imgGray;
        img = cv::imread(imgFullFilename);
        cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

        //// TASK MP.1 -> ring buffer of size dataBufferSize

        // push image into data frame buffer
        DataFrame frame;
        frame.cameraImg = imgGray;
        buf_loc = (buf_loc + 1) % dataBufferSize;
        dataBuffer[buf_loc] = frame;
        
        buf_size++;
        
        cout << "#1 : LOAD IMAGE INTO BUFFER done" << endl;
        
        /* DETECT IMAGE KEYPOINTS */
        // extract 2D keypoints from current image
        vector<cv::KeyPoint> keypoints; // create empty feature list for current image
        string detectorType = "HARRIS";

        //// add the following keypoint detectors in file matching2D.cpp and enable string-based selection based on detectorType
        //// -> HARRIS, FAST, BRISK, ORB, AKAZE, SIFT

        //Aproximate timing so that we dont repeat in everyfunction
        double tt = (double)cv::getTickCount();
        if (detectorType.compare("SHITOMASI") == 0)
        {
            detKeypointsShiTomasi(keypoints, imgGray, false);
        }
        else if (detectorType.compare("HARRIS") == 0)
        {
            detKeypointsHarris(keypoints, imgGray, false);
        }
        else 
        {
            detKeypointsModern(keypoints, imgGray, detectorType,false);
        }
        tt = ((double)cv::getTickCount() - tt) / cv::getTickFrequency();
        cout << "Approximate detection time (including function calls) is : " << 1000 * tt / 1.0 << " msec" << endl;
        result_detection_time.push_back(1000 * tt / 1.0);

        // only keep keypoints on the preceding vehicle
        vector<cv::KeyPoint> filtered_keypoints;
        bool bFocusOnVehicle = true;
        cv::Rect vehicleRect(535, 180, 180, 150);
        if (bFocusOnVehicle)
        {
            for(auto kpt : keypoints)
            {
                if(vehicleRect.contains(kpt.pt))
                    filtered_keypoints.push_back(kpt);
            }
            keypoints = filtered_keypoints;
        }
        cout << "# of keypoint in ROI is " << keypoints.size() << endl;
        result_num_keypoints_in_ROI.push_back(keypoints.size());
        
        // limit number of keypoints (helpful for debugging and learning)
        bool bLimitKpts = false;
        if (bLimitKpts)
        {
            int maxKeypoints = 50;

            if (detectorType.compare("SHITOMASI") == 0)
            { // there is no response info, so keep the first 50 as they are sorted in descending quality order
                keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
            }
            cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
            cout << " NOTE: Keypoints have been limited!" << endl;
        }

        dataBuffer[buf_loc].keypoints = keypoints;
        cout << "#2 : DETECT KEYPOINTS done" << endl;

        /* EXTRACT KEYPOINT DESCRIPTORS */
        //// -> BRIEF, ORB, FREAK, AKAZE, SIFT
        cv::Mat descriptors;
        string descriptorType = "BRIEF"; // BRIEF, ORB, FREAK, AKAZE, SIFT
        //Approximate Extraction time that includes function call - estimated to be 0.03msec additional
        double ttt = 0;
        descKeypoints((dataBuffer[buf_loc]).keypoints, (dataBuffer[buf_loc]).cameraImg, descriptors, descriptorType, ttt);
        cout << "Extraction time is : " << 1000 * ttt / 1.0 << " msec" << endl;
        result_extraction_time.push_back(1000 * ttt / 1.0);
        dataBuffer[buf_loc].descriptors = descriptors;

        cout << "#3 : EXTRACT DESCRIPTORS done" << endl;

        if (buf_size > 1)
        {
            /* MATCH KEYPOINT DESCRIPTORS */
            vector<cv::DMatch> matches;
            string matcherType = "MAT_BF";        // MAT_BF, MAT_FLANN
            string descriptorType = "DES_HOG"; // DES_BINARY, DES_HOG
            string selectorType = "SEL_KNN";       // SEL_NN, SEL_KNN

            //// add FLANN matching in file matching2D.cpp
            //// add KNN match selection and perform descriptor distance ratio filtering with t=0.8 in file matching2D.cpp

            int preIndex = (buf_loc - 1 + dataBufferSize) % dataBufferSize;
            matchDescriptors((dataBuffer[preIndex]).keypoints, (dataBuffer[buf_loc]).keypoints,
                             (dataBuffer[preIndex]).descriptors, (dataBuffer[buf_loc]).descriptors,
                             matches, descriptorType, matcherType, selectorType);
            
            // store matches in current data frame
            (dataBuffer[buf_loc]).kptMatches = matches;

            cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << endl;
            cout << "# of matched keypoints is " << matches.size() << endl;
            result_num_matched_keypoints.push_back(matches.size());
            
            // visualize matches between current and previous image
            bVis = true;
            if (bVis)
            {
                int preIndex = (buf_loc - 1 + dataBufferSize) % dataBufferSize;
                cv::Mat matchImg = ((dataBuffer[buf_loc]).cameraImg).clone();
                cv::drawMatches((dataBuffer[preIndex]).cameraImg, (dataBuffer[preIndex]).keypoints,
                                (dataBuffer[buf_loc]).cameraImg, (dataBuffer[buf_loc]).keypoints,
                                matches, matchImg,
                                cv::Scalar::all(-1), cv::Scalar::all(-1),
                                vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

                string windowName = "Matching keypoints between two camera images";
                cv::namedWindow(windowName, 7);
                cv::imshow(windowName, matchImg);
                cout << "Press key to continue to next image" << endl;
                cv::waitKey(0); // wait for key to be pressed
            }
            bVis = false;
        }

    } // eof loop over all images


    //printing performance results for analysis
    
    cout << "Number of detected keypoints in the front car ROI per image" << endl; 
    for(int n : result_num_keypoints_in_ROI)
    {
        cout << n << endl;
    }

    cout << "Number of matched keypoints per image" << endl; 
    for(int n : result_num_matched_keypoints)
    {
        cout << n << endl;
    }   

    cout << "Detection time per image in msec" << endl; 
    for(auto n : result_detection_time)
    {
        cout << n << endl;
    } 

    cout << "Extraction time per image" << endl; 
    for(auto n : result_extraction_time)
    {
        cout << n << endl;
    } 
    
    return 0;
}
