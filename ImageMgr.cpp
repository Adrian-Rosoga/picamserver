#include "ImageMgr.h"
#include "util.h"

#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cassert>

#include "producer_consumer.h"

ImageMgr::ImageMgr()
{
    images_.emplace_back(MAX_IMAGE_SIZE);
    images_.emplace_back(MAX_IMAGE_SIZE);
}

void ImageMgr::startImageThread(const std::string& takeImageCmd)
{
    takePhotoThread_ = std::thread(&ImageMgr::takeImageThreadLoop_, this, takeImageCmd);
}

ImageMgr::~ImageMgr()
{
    takePhotoThread_.join();
}

void ImageMgr::takeImageThreadLoop_(const std::string& takeImageCmd)
{
    while (1)
    {   
        {
            std::function<void(int)> take_picture =
              [&](int index) {

                  int nbBytes = exec(takeImageCmd.c_str(), images_[index].getBuffer(), MAX_IMAGE_SIZE);
                  
                  if (nbBytes == -1)
                  {
                      std::cerr << "Error: Taking image failed. Exiting." << std::endl;
                      exit(1);
                  }
                  
                  images_[index].size = nbBytes;
                
                  log(std::this_thread::get_id(), " Photo thread - size = ", nbBytes, ", index = ", index);
              };
            
            produce(take_picture);
        }
    }
}
