#pragma once

#include "http.hpp"
#include "thread.hpp"

#include <vita2d.h>

class ImageFetcher
{
public:
    ImageFetcher(DbItem *item);
    ~ImageFetcher();

    vita2d_texture* get_texture();

private:
    Mutex _mutex;

    std::string _path;
    std::string _url;
    bool _abort{false};
    std::unique_ptr<Http> _http;
    vita2d_texture* _texture;

    Thread _thread;

    void do_request();
};
