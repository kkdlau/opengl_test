#if !defined(__GL_HELPER__)
#define __GL_HELPER__

#include "Image.hpp"
#include "gl_inc.hpp"
#include <functional>

using namespace std;

#ifdef PROJ_DEBUG
#define debugger(...)                                                          \
  printf(__VA_ARGS__);                                                         \
  printf("\n");                                                                \
  fflush(stdout);
#else
#define debugger(...)
#endif

#define RED_COLOR 255, 0, 0
namespace GLHelper {

static void gl_draw_shape(int drawing_mode, function<void()> drawing_funcs) {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glBegin(drawing_mode);

  drawing_funcs();

  glEnd();
}

static void gl_set_point(double x, double y) {
  // p = pDoc->clip(p);
  glVertex2d(x, y);
}

static void gl_set_point(const Point &p) { gl_set_point(p.x, p.y); }

static float rad_to_deg(const float rad) { return rad / (2 * 3.1415926) * 360; }

static void gl_set_color(GLubyte *color) { glColor4ubv(color); }

} // namespace GLHelper

#endif // __GL_HELPER__
