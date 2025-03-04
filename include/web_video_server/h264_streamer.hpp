#ifndef H264_STREAMERS_H_
#define H264_STREAMERS_H_

#include <image_transport/image_transport.hpp>
#include "web_video_server/libav_streamer.hpp"
#include "async_web_server_cpp/http_request.hpp"
#include "async_web_server_cpp/http_connection.hpp"

namespace web_video_server
{

class H264Streamer : public LibavStreamer
{
public:
  H264Streamer(
    const async_web_server_cpp::HttpRequest & request,
    async_web_server_cpp::HttpConnectionPtr connection,
    rclcpp::Node::SharedPtr node);
  ~H264Streamer();

protected:
  virtual void initializeEncoder();
  std::string preset_;
};

class H264StreamerType : public LibavStreamerType
{
public:
  H264StreamerType();
  std::shared_ptr<ImageStreamer> create_streamer(
    const async_web_server_cpp::HttpRequest & request,
    async_web_server_cpp::HttpConnectionPtr connection,
    rclcpp::Node::SharedPtr node);
};

}

#endif
