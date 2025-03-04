#ifndef PNG_STREAMERS_H_
#define PNG_STREAMERS_H_

#include <image_transport/image_transport.hpp>
#include <opencv2/imgcodecs.hpp>
#include "web_video_server/image_streamer.hpp"
#include "async_web_server_cpp/http_request.hpp"
#include "async_web_server_cpp/http_connection.hpp"
#include "web_video_server/multipart_stream.hpp"

namespace web_video_server
{

class PngStreamer : public ImageTransportImageStreamer
{
public:
  PngStreamer(
    const async_web_server_cpp::HttpRequest & request,
    async_web_server_cpp::HttpConnectionPtr connection,
    rclcpp::Node::SharedPtr node);
  ~PngStreamer();

protected:
  virtual void sendImage(const cv::Mat &, const rclcpp::Time & time);

private:
  MultipartStream stream_;
  int quality_;
};

class PngStreamerType : public ImageStreamerType
{
public:
  std::shared_ptr<ImageStreamer> create_streamer(
    const async_web_server_cpp::HttpRequest & request,
    async_web_server_cpp::HttpConnectionPtr connection,
    rclcpp::Node::SharedPtr node);
  std::string create_viewer(const async_web_server_cpp::HttpRequest & request);
};

class PngSnapshotStreamer : public ImageTransportImageStreamer
{
public:
  PngSnapshotStreamer(
    const async_web_server_cpp::HttpRequest & request,
    async_web_server_cpp::HttpConnectionPtr connection,
    rclcpp::Node::SharedPtr node);
  ~PngSnapshotStreamer();

protected:
  virtual void sendImage(const cv::Mat &, const rclcpp::Time & time);

private:
  int quality_;
};

}

#endif
