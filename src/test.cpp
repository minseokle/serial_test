#include <cstdio>
#include <rclcpp/rclcpp.hpp>

#include <serial/serial.h>

#include <std_msgs/msg/string.hpp>

class SerialTest : public rclcpp::Node
{
public:
  SerialTest() : Node("serial_test")
  {
    try
    {
      serial_.setPort("/dev/ttyUSB0");
      serial_.setBaudrate(1000000);
      serial_.open();
    }
    catch (serial::IOException &e)
    {
      RCLCPP_ERROR(this->get_logger(), "Unable to open port");
      rclcpp::shutdown();
      return;
    }

    if (serial_.isOpen())
    {
      RCLCPP_INFO(this->get_logger(), "Serial port opened successfully");
    }
    else
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to open serial port");
      rclcpp::shutdown();
      return;
    }

    sub_ = this->create_subscription<std_msgs::msg::String>("serial_test/in", 10, std::bind(&SerialTest::sub_callback, this, std::placeholders::_1));
    pub_ = this->create_publisher<std_msgs::msg::String>("serial_test/out", 10);
    read_timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&SerialTest::timer_callback, this));
  }

private:
  serial::Serial serial_;

  std::shared_ptr<rclcpp::Subscription<std_msgs::msg::String>> sub_;
  void sub_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    serial_.write(msg->data);
  }
  std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String>> pub_;
  std::shared_ptr<rclcpp::TimerBase> read_timer_;
  void timer_callback()
  {
    unsigned char *readBuffer;

    try // 중간에 통신선이 뽑히거나 할 경우 대비해 예외처리
    {
      int size = serial_.available();
      if (size != 0) // 받은 바이트 수가 0이 아닐때
      {
        readBuffer = new uint8_t[size];
        serial_.read(readBuffer, size); // 버퍼에 읽어옴

        std_msgs::msg::String out_msg;
        out_msg.data = std::string(reinterpret_cast<char *>(readBuffer), size);
        pub_->publish(out_msg);

        delete[] readBuffer;
      }
    }
    catch (serial::IOException e) // 예외 발생시 메시지 띄우고 포트 닫는다.
    {
      RCLCPP_ERROR(this->get_logger(), "Port disconnected. closing port(%s)", e.what());
      serial_.close();
      rclcpp::shutdown();
    }
  }
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SerialTest>());
  rclcpp::shutdown();
  return 0;
}
