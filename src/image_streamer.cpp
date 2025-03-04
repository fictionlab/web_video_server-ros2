#include "web_video_server/image_streamer.hpp"
#include <cv_bridge/cv_bridge.hpp>
#include <iostream>

namespace web_video_server
{

ImageStreamer::ImageStreamer(
  const async_web_server_cpp::HttpRequest & request,
  async_web_server_cpp::HttpConnectionPtr connection, rclcpp::Node::SharedPtr node)
:request_(request), connection_(connection), node_(node), inactive_(false)
{
  topic_ = request.get_query_param_value_or_default("topic", "");
}

ImageStreamer::~ImageStreamer()
{
}

ImageTransportImageStreamer::ImageTransportImageStreamer(
  const async_web_server_cpp::HttpRequest & request,
  async_web_server_cpp::HttpConnectionPtr connection, rclcpp::Node::SharedPtr node)
:ImageStreamer(request, connection, node), it_(node), initialized_(false)
{
  output_width_ = request.get_query_param_value_or_default<int>("width", -1);
  output_height_ = request.get_query_param_value_or_default<int>("height", -1);
  invert_ = request.has_query_param("invert");
  default_transport_ = request.get_query_param_value_or_default("default_transport", "raw");
  qos_profile_name_ = request.get_query_param_value_or_default("qos_profile", "default");
}

ImageTransportImageStreamer::~ImageTransportImageStreamer()
{
}

void ImageTransportImageStreamer::start()
{
  image_transport::TransportHints hints(node_.get(), default_transport_);
  auto tnat = node_->get_topic_names_and_types();
  inactive_ = true;
  for (auto topic_and_types : tnat) {
    if (topic_and_types.second.size() > 1) {
      // explicitly avoid topics with more than one type
      break;
    }
    auto & topic_name = topic_and_types.first;
    if(topic_name == topic_ || (topic_name.find("/") == 0 && topic_name.substr(1) == topic_)) {
      inactive_ = false;
      break;
    }
  }

  // Get QoS profile from query parameter
  RCLCPP_INFO(node_->get_logger(), "Streaming topic %s with QoS profile %s", topic_.c_str(),
      qos_profile_name_.c_str());
  auto qos_profile = get_qos_profile_from_name(qos_profile_name_);
  if (!qos_profile) {
    qos_profile = rmw_qos_profile_default;
    RCLCPP_ERROR(
      node_->get_logger(),
      "Invalid QoS profile %s specified. Using default profile.",
      qos_profile_name_.c_str());
  }

  // Create subscriber
  image_sub_ = image_transport::create_subscription(
      node_.get(), topic_,
      std::bind(&ImageTransportImageStreamer::imageCallback, this, std::placeholders::_1),
      default_transport_, qos_profile.value());
}

void ImageTransportImageStreamer::initialize(const cv::Mat &)
{
}

void ImageTransportImageStreamer::restreamFrame(double max_age)
{
  if (inactive_ || !initialized_) {
    return;
  }
  try {
    if (last_frame + rclcpp::Duration::from_seconds(max_age) < node_->now() ) {
      std::scoped_lock lock(send_mutex_);
      sendImage(output_size_image, node_->now() ); // don't update last_frame, it may remain an old value.
    }
  } catch (boost::system::system_error & e) {
    // happens when client disconnects
    RCLCPP_DEBUG(node_->get_logger(), "system_error exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (std::exception & e) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (...) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "exception");
    inactive_ = true;
    return;
  }
}

void ImageTransportImageStreamer::imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr & msg)
{
  if (inactive_) {
    return;
  }

  cv::Mat img;
  try {
    if (msg->encoding.find("F") != std::string::npos) {
      // scale floating point images
      cv::Mat float_image_bridge = cv_bridge::toCvCopy(msg, msg->encoding)->image;
      cv::Mat_<float> float_image = float_image_bridge;
      double max_val;
      cv::minMaxIdx(float_image, 0, &max_val);

      if (max_val > 0) {
        float_image *= (255 / max_val);
      }
      img = float_image;
    } else {
      // Convert to OpenCV native BGR color
      img = cv_bridge::toCvCopy(msg, "bgr8")->image;
    }

    int input_width = img.cols;
    int input_height = img.rows;

    output_width_ = input_width;
    output_height_ = input_height;

    if (invert_) {
      // Rotate 180 degrees
      cv::flip(img, img, false);
      cv::flip(img, img, true);
    }

    std::scoped_lock lock(send_mutex_); // protects output_size_image
    if (output_width_ != input_width || output_height_ != input_height) {
      cv::Mat img_resized;
      cv::Size new_size(output_width_, output_height_);
      cv::resize(img, img_resized, new_size);
      output_size_image = img_resized;
    } else {
      output_size_image = img;
    }

    if (!initialized_) {
      initialize(output_size_image);
      initialized_ = true;
    }

    last_frame = node_->now();
    sendImage(output_size_image, msg->header.stamp);

  } catch (cv_bridge::Exception & e) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "cv_bridge exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (cv::Exception & e) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "cv_bridge exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (boost::system::system_error & e) {
    // happens when client disconnects
    RCLCPP_DEBUG(node_->get_logger(), "system_error exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (std::exception & e) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "exception: %s", e.what());
    inactive_ = true;
    return;
  } catch (...) {
    // TODO THROTTLE with 30
    RCLCPP_ERROR(node_->get_logger(), "exception");
    inactive_ = true;
    return;
  }
}

}
