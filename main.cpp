#include "gl_helper.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Window.H>
#include <opencv2/opencv.hpp>
#include <stdio.h>

#include "Image.hpp"
#include "ImageUtils.hpp"

using namespace GLHelper;

class View : public Fl_Gl_Window {
public:
  GLuint color;
  GLuint depth;
  View(int x, int y, int w, int h, const char *l)
      : Fl_Gl_Window(x, y, w, h, l) {
    m_nWindowWidth = w;
    m_nWindowHeight = h;
  }

  void draw() {
#ifndef MESA
    // To avoid flicker on some machines.
    glDrawBuffer(GL_FRONT_AND_BACK);
#endif // !MESA

    if (!valid()) {
      glClearColor(0.7f, 0.7f, 0.7f, 1.0);

      // We're only using 2-D, so turn off depth
      glDisable(GL_DEPTH_TEST);

      ortho();

      // glClear(GL_COLOR_BUFFER_BIT);
    }

    Point scrollpos; // = GetScrollPosition();
    scrollpos.x = 0;
    scrollpos.y = 0;

    m_nWindowWidth = w();
    m_nWindowHeight = h();

    int drawWidth, drawHeight;
    drawWidth = min(m_nWindowWidth, render_content.width);
    drawHeight = min(m_nWindowHeight, render_content.height);

    int startrow = render_content.height - (scrollpos.y + drawHeight);
    if (startrow < 0)
      startrow = 0;

    m_pPaintBitstart = render_content.raw_fmt() +
                       4 * ((render_content.width * startrow) + scrollpos.x);

    m_nStartRow = startrow;
    m_nEndRow = startrow + drawHeight;
    m_nStartCol = scrollpos.x;
    m_nEndCol = m_nStartCol + drawWidth;

    render2();

    glFlush();

#ifndef MESA
    // To avoid flicker on some machines.
    glDrawBuffer(GL_BACK);
#endif // !MESA
    this->refresh();
  }

  void render2() {
    // RestoreContent(drawing.raw_fmt());
    // glPointSize(30);
    // gl_draw_shape(GL_POINTS, [&] {
    //   GLubyte color[] = {255, 0, 0, 255 / 2};
    //   gl_set_color(color);
    //   gl_set_point(50, 50);
    // });
    RestoreContent(render_content.raw_fmt());
  }

  void refresh() { redraw(); }

  void resizeWindow(int width, int height) { resize(x(), y(), width, height); }

  void SaveCurrentContent(GLvoid *ptr) {
    glReadBuffer(GL_FRONT_AND_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, render_content.width);

    glReadPixels(0, m_nWindowHeight - render_content.height,
                 render_content.width, render_content.height, GL_RGBA,
                 GL_UNSIGNED_BYTE, ptr);
  }

  void RestoreContent(GLvoid *ptr) {
    glDrawBuffer(GL_BACK);

    glRasterPos2i(0, m_nWindowHeight - render_content.height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, render_content.width);
    glDrawPixels(render_content.width, render_content.height, GL_RGBA,
                 GL_UNSIGNED_BYTE, ptr);

    //	glDrawBuffer(GL_FRONT);
  }

  void set_img(Image &img) {
    render_content = img;
    resizeWindow(700, 700);
    refresh();
  }

  int handle(int event) {
    redraw();
    return 1;
  }

  Image render_content;
  Image drawing;
  GLvoid *m_pPaintBitstart;
  int m_nStartRow, m_nEndRow, m_nStartCol, m_nEndCol, m_nWindowWidth,
      m_nWindowHeight;
};

int main() {
  View v{0, 0, 700, 700, nullptr};
  // Image bean = Image::from(
  //     "/Users/dannylau/Program/COMP4411-Impressionist/images/babyraptor.bmp");

  Image bean = Image::from("/Users/dannylau/Program/COMP4411-Impressionist/"
                           "images/test_image.bmp");

  // Image clone = bean;
  // clone.for_range_pixel(Point{100, 100}, Point{200, 200}, [&](int y, int x) {
  //   auto c = clone(y, x);
  //   get<0>(c) = 255;
  //   get<1>(c) = 255;
  //   get<2>(c) = 255;
  // });

  // debugger("similarity : %f", ImageUtils::structural_similarity(bean,
  // clone));
  // bean.resize(100, 100);
  ImageUtils::mosaics(bean);

  Fl::visual(FL_DOUBLE | FL_INDEX);

  v.set_img(bean);

  v.show();

  return Fl::run();
}