#pragma once

#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

const int MAX_IMAGE_SIZE = 3000000; // Guess

struct Image
{
    explicit Image(int maxImageSize)
    {
        image_.reset(new char[maxImageSize]);
        size = 0;
    }
    char* getBuffer() { return image_.get(); }
    const char* getBuffer() const { return image_.get(); }
    int size;
    
private:
    std::unique_ptr<char[]> image_;
};

class ImageMgr
{
public:
    ImageMgr();
    ~ImageMgr();
    void startImageThread(const std::string& takeImageCmd);
    const Image& operator[](int index) const { return images_[index]; }
    
    ImageMgr(const ImageMgr&) = delete;
    ImageMgr& operator=(const ImageMgr&) = delete;
    
private:
    std::vector<Image> images_;
    std::thread takePhotoThread_;
    void takeImageThreadLoop_(const std::string& takeImageCmd);
};
