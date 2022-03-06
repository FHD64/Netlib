#ifndef __BASE_TIMESTAMP__
#define __BASE_TIMESTAMP__

#include <string>
#include <boost/operators.hpp>

namespace netlib {

using std::string;
//BOOST库operators头文件存在自动推导运算符，可通过继承自动实现运算符重载
//equality_comparable需定义==重载，自动实现！=重载
//less_than_comparable需定义<重载，自动实现> >= <=重载
class Timestamp : public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp> {

 public:
  static const int64_t kUsPerSecond = 1000 * 1000;
  Timestamp() : ussinceepoch_(0) {
  }
  Timestamp(int64_t ussinceepoch) : ussinceepoch_(ussinceepoch) {
  }

  string toString() const;
  string toFormattedString(bool showInus = true) const;

  //若时间戳>0判断为有效时间戳
  bool valid() const {
      return (ussinceepoch_ > 0);
  }
  
  //获取时间戳时间
  int64_t getUsSinceEpoch() const {
      return ussinceepoch_;
  }
  time_t getSencondsSinceEpoch() const {
      return static_cast<time_t>(ussinceepoch_ / kUsPerSecond);
  }

  //根据当前时间，返回对应时间戳
  static Timestamp now();
  static Timestamp invalid() {
      return Timestamp();
  }
  //根据指定时间返回对应时间戳对象
  static Timestamp fromUnixTime(time_t t, int us) {
      return Timestamp(static_cast<int64_t>(t) * kUsPerSecond + us);
  }
  static Timestamp fromUnixTime(time_t t) {
      return fromUnixTime(t, 0);
  }

 private:
  int64_t ussinceepoch_;
};

 inline bool operator<(Timestamp lhs, Timestamp rhs) {
     return (lhs.getUsSinceEpoch() < rhs.getUsSinceEpoch());
 }
 inline bool operator==(Timestamp lhs, Timestamp rhs) {
     return (lhs.getUsSinceEpoch() == rhs.getUsSinceEpoch());
 }
 
 //将当前时间戳加上固定时间
 inline Timestamp addTime(Timestamp timestamp, double seconds) {
     int64_t us = static_cast<int64_t>(seconds * Timestamp::kUsPerSecond);
     return Timestamp(timestamp.getUsSinceEpoch() + us);
 }

}
#endif