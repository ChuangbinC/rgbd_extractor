/*
 * @Author: Chuangbin Chen
 * @Date: 2019-11-06 20:52:59
 * @LastEditTime: 2019-11-07 01:43:02
 * @LastEditors: Do not edit
 * @Description: 
 */
#include <ros/ros.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <ros/package.h>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/thread/thread.hpp>

#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

#define KEYCODE_SPACE 0x20
#define KEYCODE_Q 0x71

class ImageExtractor
{
public:
    ImageExtractor();
    void KeyLoop();

    void imageCallback(const cv_bridge::CvImage::ConstPtr &imgMsg);
    void depthCallback(const cv_bridge::CvImage::ConstPtr &depthMsg);
    void printInformation();
private:
    ros::NodeHandle nh_;
    ros::Subscriber imgSub, depthSub;
    std::string image_save_path;
    string rgbFileName ;
    string depthFileName;
    int rgbNum=0;
    int depthNum=0;

};

ImageExtractor::ImageExtractor()
{
    imgSub = nh_.subscribe("/camera/rgb/image_raw", 1, &ImageExtractor::imageCallback,this);
    depthSub = nh_.subscribe("/camera/depth/image_raw",1, &ImageExtractor::depthCallback,this);
    // 下面的 / 是要的，根据 rosparam list 查到的
    nh_.getParam("/image_save_path", image_save_path);
    ROS_INFO("%s\n", image_save_path.c_str());
}

int kfd = 0;
struct termios cooked, raw;
bool save_rgb_image = false, save_depth_image = false;


int main(int argc, char **argv)
{
    ros::init(argc, argv, "rgbd_image_extractor");
    ImageExtractor image_extractor;
    // 添加一个多程序可以执行订阅程序，否则keeyloop会堵塞主程序
    boost::thread t = boost::thread(boost::bind(&ImageExtractor::KeyLoop, &image_extractor));
    // 订阅信息的程序需要添加 ros::spin(); 不然没办法订阅
    // spin后面的程序是不会执行的，所以添加在它后面的程序是没意义的
    ros::spin();
    
    return (0);
}

void ImageExtractor::KeyLoop()
{
    char c;

    // get the console in raw mode
    tcgetattr(kfd, &cooked);
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &= ~(ICANON | ECHO);
    // Setting a new line, then end of file
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);

    puts("Reading from keyboard");
    puts("---------------------------");
    puts("Use arrow keys to move the turtle. 'q' to quit.");

    for (;;)
    {
        // get the next event from the keyboard
        if (read(kfd, &c, 1) < 0)
        {
            perror("read():");
            exit(-1);
        }

        ROS_DEBUG("value: 0x%02X\n", c);

        switch (c)
        {
        case KEYCODE_SPACE:
            save_rgb_image = true;
            save_depth_image = true;
            break;

        case KEYCODE_Q:
            return;
        }
        
    }
    return;
}

void ImageExtractor::imageCallback(const cv_bridge::CvImage::ConstPtr &imgMsg)
{
    if(save_rgb_image)
    {
        Mat inImage;
        inImage = imgMsg->image;

        vector<int> compressionQuality;
        compressionQuality.push_back(CV_IMWRITE_PNG_COMPRESSION);
        stringstream ss;
        ss << rgbNum;
        // rgbFileName = image_save_path + ss.str() + ".png";
        rgbFileName = "/home/ccb/Code/image/rgb_" + ss.str() + ".png";

        imwrite(rgbFileName, inImage, compressionQuality);
        cout << "write "
             << "  rgb_" << ss.str() << " suscessfully!" << endl;
        rgbNum += 1;

        save_rgb_image = false;
    }
}

void ImageExtractor::depthCallback(const cv_bridge::CvImage::ConstPtr &depthMsg)
{
    if (save_depth_image)
    {
        Mat depthImage(depthMsg->image.cols, depthMsg->image.rows, CV_16UC1);
        depthImage = depthMsg->image;
        vector<int> compressionQuality;
        compressionQuality.push_back(CV_IMWRITE_PNG_COMPRESSION);
        stringstream ss;
        ss << depthNum;
        // depthFileName = image_save_path + ss.str() + ".png";
        depthFileName = "/home/ccb/Code/image/depth_" + ss.str() + ".png";

        imwrite(depthFileName, depthImage, compressionQuality);
        cout << "write "
             << "depth_" << ss.str() << " suscessfully!" << endl;
        depthNum += 1;
        save_depth_image = false;
    }
}

void ImageExtractor::printInformation()
{
    ROS_INFO("%s\n", image_save_path.c_str());
}