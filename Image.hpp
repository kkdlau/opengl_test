#if !defined(__IMAGE_H_)
#define __IMAGE_H_

#include "Bitmap.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Window.H>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <cmath>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

static float frand() { return (float)rand() / RAND_MAX; }

static int irand(int max) { return rand() % max; }

class Point {
public:
  Point(){};
  Point(int xx, int yy) {
    x = xx;
    y = yy;
  };

  int x, y;

  friend Point operator+(const Point &p1, const Point &p2) {
    return Point(p1.x + p2.x, p1.y + p2.y);
  }

  friend Point operator-(const Point &p1, const Point &p2) {
    return Point(p1.x - p2.x, p1.y - p2.y);
  }

  /**
   * @brief Point division operation.
   *
   * This function will return the radian by calculating tan((p2.y - p1.y) /
   * (p2.x - p1.x)).
   */
  friend float operator/(const Point &p1, const Point &p2) {
    return atan2((float)(p2.y - p1.y), (float)(p2.x - p1.x));
  }

  Point shift_x(int x) const { return Point(this->x + x, y); }
  Point shift_y(int y) const { return Point(x, this->y + y); }
  Point rotate(float rad) const {
    return Point((int)(cos(rad) * (x)-sin(rad) * y),
                 (int)(sin(rad) * (x)-cos(rad) * y));
  }

  static Point zero() { return Point(0, 0); }
  static Point rand(float min, float max) {
    const float span = max - min;
    return Point(frand() * span + min, frand() * span + min);
  }

  const char *to_string() const {
    return (std::to_string(this->x) + ", " + std::to_string(this->y)).c_str();
  }
};

typedef tuple<GLubyte, GLubyte, GLubyte, GLubyte> RGBA;

static RGBA operator+(RGBA c1, RGBA c2) {
  return {get<0>(c1) + get<0>(c2), get<1>(c1) + get<1>(c2),
          get<2>(c1) + get<2>(c2), get<3>(c1) + get<3>(c2)};
}

static RGBA operator/(RGBA c1, double d) {
  return {(int)(get<0>(c1) / d), (int)(get<1>(c1) / d), int(get<2>(c1) / d),
          int(get<3>(c1) / d)};
}

static RGBA operator*(RGBA c1, double d) {
  return {(int)(get<0>(c1) * d), (int)(get<1>(c1) * d), int(get<2>(c1) * d),
          int(get<3>(c1) * d)};
}

class Image {
public:
  vector<GLubyte> bytes;

  int width;
  int height;
  Image() : Image{nullptr, 0, 0} {}
  Image(GLubyte *buf, int w, int h) { set(buf, w, h); }
  static Image from(const char *path) {
    unsigned char *data;
    int width, height;

    data = readBMP(path, width, height);

    // int w = 100, h = 200;

    // GLubyte *tmp = new GLubyte[w * h * 4];

    // gluScaleImage(GL_RGB, width, height, GL_UNSIGNED_BYTE, data, w, h,
    //               GL_UNSIGNED_BYTE, tmp);

    return Image(data, width, height);
  }

  void set(GLubyte *buf, int w, int h) {
    width = w, height = h;
    bytes = {};
    const int length = w * h * 3;
    int count = 0;
    for (int i = 0; i < length; i++) {
      bytes.push_back(buf[i]);
      count++;
      if (count == 3) {
        bytes.push_back(255);
        count = 0;
      }
    }
  }

  bool valid_point(int y, int x) const {
    if (x < 0)
      return false;
    if (y < 0)
      return false;
    if (x >= width)
      return false;
    if (y >= height)
      return false;
    return true;
  }

  Point clip(const Point &p) { return clip(p.x, p.y); }

  Point clip(int x, int y) {
    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;
    if (x >= width)
      x = width - 1;
    if (y >= height)
      y = height - 1;
    return Point{x, y};
  }

  const tuple<GLubyte &, GLubyte &, GLubyte &, GLubyte &> operator()(int y,
                                                                     int x) {
    int i = 4 * (y * width + x);
    return {bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3]};
  }

  GLubyte *raw_fmt() { return bytes.data(); }
  GLubyte *paint_byte() { return bytes.data() + height * 4; }

  void set_pixel(int y, int x, const RGBA &rgba) {
    auto color = (*this)(y, x);
    get<0>(color) = get<0>(rgba);
    get<1>(color) = get<1>(rgba);
    get<2>(color) = get<2>(rgba);
    get<3>(color) = get<3>(rgba);
  }

  void set_pixel(const Point &p, const RGBA &rgb) { set_pixel(p.y, p.x, rgb); }

  void set_alpha(float a) {
    for_each_pixel(
        [&](int y, int x) { get<3>((*this)(y, x)) = (GLubyte)(255 * a); });
  }

  void for_range_pixel(const Point &s, const Point &e,
                       function<void(int, int)> handler) {
    for (int y = s.y; y <= e.y; y++) {
      for (int x = s.x; x <= e.x; x++) {
        handler(y, x);
      }
    }
  }

  void for_each_pixel(function<void(int, int)> handler) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        handler(y, x);
      }
    }
  }

  void crop(size_t sx, size_t sy) {
    vector<GLubyte> buf{};
    for_each_pixel([&](int y, int x) {
      if (y >= sy || x >= sx)
        return;
      auto color = (*this)(y, x);
      buf.push_back(get<0>(color));
      buf.push_back(get<1>(color));
      buf.push_back(get<2>(color));
      buf.push_back(get<3>(color));
    });

    bytes = buf;
    width = width > sx ? sx : width;
    height = height > sy ? sy : height;
  }

  void resize(int w, int h) {
    w = 100;
    h = 100;

    GLubyte *tmp = new GLubyte[100 * 3];
    GLubyte *tmp2 = new GLubyte[100 * 3];
    printf("ok1");
    fflush(stdout);
    memset(tmp, 0, 10 * 10 * 3);
    memset(tmp2, 0, 10 * 10 * 3);

    printf("ok2");
    fflush(stdout);

    gluScaleImage(GL_RGB, 5, 5, GL_UNSIGNED_BYTE, tmp, 1, 1, GL_UNSIGNED_BYTE,
                  tmp2);
    printf("ok3");
    fflush(stdout);

    // bytes.data() = tmp;
    // width = w;
    // height = h;
  }

  void paint(Image &img, const Point &at) {
    img.for_each_pixel([&](int y, int x) {
      Point p{y, x};
      p = p + at;
      if (valid_point(p.y, p.x)) {
        auto src_color = img(y, x);
        auto tar_color = (*this)(p.y, p.x);
        tar_color = src_color;
      }
    });
  }

  Image crop(const Point &from, const Point &to) {
    vector<GLubyte> buf{};
    for_range_pixel(from, to, [&](int y, int x) {
      auto color = (*this)(y, x);
      buf.push_back(get<0>(color));
      buf.push_back(get<1>(color));
      buf.push_back(get<2>(color));
      buf.push_back(get<3>(color));
    });
    const Point diff = to - from;

    Image tmp;
    tmp.set(buf.data(), diff.x + 1, diff.y + 1);

    return tmp;
  }
};
#endif // __IMAGE_H_
