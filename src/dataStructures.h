#ifndef dataStructures_h
#define dataStructures_h

#include <vector>
#include <opencv2/core.hpp>


struct DataFrame { // represents the available sensor information at the same time instance
    
    cv::Mat cameraImg; // camera image
    
    std::vector<cv::KeyPoint> keypoints; // 2D keypoints within camera image
    cv::Mat descriptors; // keypoint descriptors
    std::vector<cv::DMatch> kptMatches; // keypoint matches between previous and current frame
};


template<typename T>
struct Buffer {
    public:
    int size;
    int loc;
    std::vector<T> buffer;
    Buffer(int size_) : size(size_) 
    {
        this->buffer.resize(size_);
        loc = 0;
    }
    T get(int index) 
    {
        return buffer[(loc - index + size) % size];
    }
    void push(T newdata) 
    {
        int index = loc % size;
        buffer[index] = newdata;
        loc = (loc + 1) % size;
    }
};
 

#endif /* dataStructures_h */
