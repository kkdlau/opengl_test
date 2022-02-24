#if !defined(__IMAGE_UTILS__)
#define __IMAGE_UTILS__
#include "Image.hpp"
#include "gl_helper.hpp"
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>
namespace fs = std::filesystem;

using namespace std;

typedef tuple<GLubyte &, GLubyte &, GLubyte &, GLubyte &> RGB8888;

namespace ImageUtils {
static const short sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};

static const short sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

static Image median_filter(Image &img) {
  Image filtered = img;
  return filtered;
}

static tuple<float, float, float> sobel(Image &img, int y, int x) {
  float gx = 0;
  float gy = 0;
  y--;
  x--;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (img.valid_point(y + i, x + j)) {
        auto color = img(y + i, x + j);
        const float grey = 0.299F * get<0>(color) + 0.587F * get<1>(color) +
                           0.114F * get<2>(color);
        gx += sobel_x[i][j] * grey;
        gy += sobel_y[i][j] * grey;
      }
    }
  }
  return {gx, gy, atan2(gy, gx)};
}

static Image generate_edge_image(Image &img) {
  Image return_img = img;
  return_img.for_each_pixel([&](int y, int x) {
    auto result = sobel(img, y, x);
    float gx = get<0>(result);
    float gy = get<1>(result);
    float gradient2 = gx * gx + gy * gy;

    auto color = return_img(y, x);

    if (gradient2 > 127 * 127) {
      // white
      get<0>(color) = 255;
      get<1>(color) = 255;
      get<2>(color) = 255;
      get<3>(color) = 255;
    } else {
      // black
      get<0>(color) = 0;
      get<1>(color) = 0;
      get<2>(color) = 0;
      get<3>(color) = 255;
    }
  });

  return return_img;
}

static Image dissolve(Image &source, Image &target) {
  Image output = target;

  output.for_each_pixel([&](int y, int x) {
    auto color = output(y, x);
    if (source.valid_point(y, x)) {
      RGBA avg_color = (source(y, x) + color) / 2;
      get<0>(color) = get<0>(avg_color);
      get<1>(color) = get<1>(avg_color);
      get<2>(color) = get<2>(avg_color);
      get<3>(color) = get<3>(avg_color);
    }
  });

  return output;
}

static float
luma_cal(tuple<GLubyte &, GLubyte &, GLubyte &, GLubyte &> &color) {
  return get<0>(color) / 3.0 + get<1>(color) / 3.0 + get<2>(color) / 3.0;
}

static tuple<float, float, float>
to_ybr(const tuple<GLubyte &, GLubyte &, GLubyte &, GLubyte &> &color) {
  auto r = get<0>(color);
  auto g = get<1>(color);
  auto b = get<2>(color);

  auto y = 16 + 65.738 * r / 256 + 129.057 * g / 256 + 25.064 * b / 256;
  auto cb = 128 + 37.945 * r / 256 + 74.494 * g / 256 + 112.439 * b / 256;
  auto cr = 128 + 112.439 * r / 256 + 94.154 * g / 256 + 18.285 * b / 256;

  return {y, cb, cr};
}

/**
 * @brief structural similarity index measure method.
 * This function is designed for measuring the similarity between two images.
 *
 * @param src source image
 * @param target target image
 * @return float similarity ranging from 0 to 1.
 */
static float structural_similarity(Image &src, Image &target) {
  float MSSIM[3] = {0}; // Y, Cr, Cb

  const int win_width_size = src.width / 2;
  const int win_height_size = src.height / 2;
  const int num_win_pixel = win_width_size * win_height_size;

  const int num_window = 2;

  const float c1 = 0.0049;
  const float c2 = 0.0441;

  static auto for_each_pixel_window = [&](int wy, int wx,
                                          function<void(int, int)> handler) {
    for (int y = 0; y < win_height_size; y++)
      for (int x = 0; x < win_width_size; x++) {
        const int x_coord = x + win_width_size * wx;
        const int y_coord = y + win_height_size * wy;
        handler(y_coord, x_coord);
      }
  };

  for (int wy = 0; wy < num_window; wy++) {
    for (int wx = 0; wx < num_window; wx++) {
      float src_luma_avg[3] = {0};
      float tar_luma_avg[3] = {0};
      float src_var[3] = {0};
      float tar_var[3] = {0};
      float cvar[3] = {0};

      for_each_pixel_window(wy, wx, [&](int y, int x) {
        auto src_color = to_ybr(src(y, x));
        auto tar_color = to_ybr(target(y, x));

        // TODO: optimize the algorithm by storing the result
        src_luma_avg[0] += get<0>(src_color) / 235;
        src_luma_avg[1] += get<1>(src_color) / 240;
        src_luma_avg[2] += get<2>(src_color) / 240;

        tar_luma_avg[0] += get<0>(tar_color) / 235;
        tar_luma_avg[1] += get<1>(tar_color) / 240;
        tar_luma_avg[2] += get<2>(tar_color) / 240;
      });

      src_luma_avg[0] /= num_win_pixel;
      src_luma_avg[1] /= num_win_pixel;
      src_luma_avg[2] /= num_win_pixel;

      tar_luma_avg[0] /= num_win_pixel;
      tar_luma_avg[1] /= num_win_pixel;
      tar_luma_avg[2] /= num_win_pixel;

      for_each_pixel_window(wy, wx, [&](int y, int x) {
        auto src_color = to_ybr(src(y, x));
        auto tar_color = to_ybr(target(y, x));

        float src_luma[3];
        float tar_luma[3];

        src_luma[0] = get<0>(src_color) / 235;
        src_luma[1] = get<1>(src_color) / 240;
        src_luma[2] = get<2>(src_color) / 240;

        tar_luma[0] = get<0>(tar_color) / 235;
        tar_luma[1] = get<1>(tar_color) / 240;
        tar_luma[2] = get<2>(tar_color) / 240;

        for (int i = 0; i < 3; i++) {
          float nom_src_var = src_luma[i] - src_luma_avg[i];
          float nom_tar_var = tar_luma[i] - tar_luma_avg[i];
          src_var[i] += nom_src_var * nom_src_var;
          tar_var[i] += nom_tar_var * nom_tar_var;
          cvar[i] += nom_src_var * nom_tar_var;
        }
      });

      for (int i = 0; i < 3; i++) {
        src_var[i] /= num_win_pixel;
        tar_var[i] /= num_win_pixel;
        cvar[i] /= num_win_pixel;
        MSSIM[i] += ((2 * src_luma_avg[i] * tar_luma_avg[i] + c1) *
                     (2 * cvar[i] + c2)) /
                    ((src_luma_avg[i] * src_luma_avg[i] +
                      tar_luma_avg[i] * tar_luma_avg[i] + c1) *
                     (src_var[i] + tar_var[i] + c2));
      }
    }

    return MSSIM[0] / num_window * 0.8 + MSSIM[1] / num_window * 0.1 +
           MSSIM[2] / num_window * 0.1;
  }
}

static float mse(Image &src, Image &tar) {
  double mse[3] = {0}; // r, g, b
  src.for_each_pixel([&](int y, int x) {
    // debugger("read %d %d", x, y);

    auto src_color = src(y, x);
    auto tar_color = tar(y, x);
    int diff[3] = {
        get<0>(src_color) - get<0>(tar_color),
        get<1>(src_color) - get<1>(tar_color),
        get<2>(src_color) - get<2>(tar_color),
    };
    for (int i = 0; i < 3; i++) {
      mse[i] += diff[i] * diff[i];
    }
  });

  return (mse[0] + mse[1] + mse[2]) / (3 * src.width * src.height);
}

static double image_l1(Image &src, Image &tar) {
  float l1[3] = {0};
  auto tcolor = tar(0, 0);

  // debugger("%d %d %d", get<0>(tcolor), get<1>(tcolor), get<2>(tcolor));

  src.for_each_pixel([&](int y, int x) {
    // debugger("read %d %d", x, y);
    auto tcolor = src(0, 0);

    debugger("%d %d %d", get<0>(tcolor), get<1>(tcolor), get<2>(tcolor));

    auto src_color = src(y, x);
    auto tar_color = tar(y, x);
    l1[0] += abs(get<0>(src_color) - get<0>(tar_color));
    l1[1] += abs(get<1>(src_color) - get<1>(tar_color));
    l1[2] += abs(get<2>(src_color) - get<2>(tar_color));
  });

  return l1[0] + l1[1] + l1[2];
}

static double l2_distance(RGB8888 c1, RGB8888 c2) {
  return sqrt(pow(static_cast<double>(get<0>(c1) - get<0>(c2)), 2.0) +
              pow(static_cast<double>(get<1>(c1) - get<1>(c2)), 2.0) +
              pow(static_cast<double>(get<2>(c1) - get<2>(c2)), 2.0));
}

static double image_l2(Image &src, Image &tar) {
  double distance = 0;
  auto tcolor = tar(0, 0);
  debugger("%d %d %d", get<0>(tcolor), get<1>(tcolor), get<2>(tcolor));
  src.for_each_pixel([&](int y, int x) {
    // debugger("read %d %d", x, y);

    auto src_color = src(y, x);
    auto tar_color = tar(y, x);
    distance += l2_distance(src_color, tar_color);
  });

  return distance;
}

static Image mosaics(Image &img) {
  // load images dataset
  static vector<Image> dataset{};
  static vector<string> alias{};
  static bool init = false;

  const int DWIDTH = 5;
  const int DHEIGHT = DWIDTH;
  int count = 0;

  if (!init) {
    std::string path =
        "/Users/dannylau/Program/COMP4411-Impressionist/thumbnails";
    for (const auto &entry : fs::directory_iterator(path)) {
      auto path = entry.path().u8string();
      if (path.find("bmp") != std::string::npos) {
        alias.push_back(string{path});
        dataset.push_back(Image::from(path.c_str()));

        count++;
      }
    }
  }

  debugger("loading done: %d", count);

  vector<decltype(dataset.begin())> replacement{};

  auto best_entry = dataset.begin();

  for (int x = 0; x < img.width - DWIDTH; x += DWIDTH) {
    for (int y = 0; y < img.height - DHEIGHT; y += DHEIGHT) {

      Point start{x, y};
      Point end = start.shift_x(DWIDTH).shift_y(DHEIGHT);
      debugger("crop %d %d  to %d %d", start.x, start.y, end.x, end.y);

      Image cropped = img.crop(start, end);

      double best = 100000000;
      int i = 0;
      for (auto entry = dataset.begin(); entry != dataset.end(); entry++) {

        // float sim = structural_similarity(cropped, *entry);
        // debugger("%s - similar: %.3f", alias[i++].c_str(), sim);

        // if (sim > best) {
        //   best = sim;
        //   best_entry = entry;
        // }

        float sim = image_l1(cropped, *entry);
        debugger("%s - similar: %.3f", alias[i++].c_str(), sim);

        if (sim < best) {
          best = sim;
          best_entry = entry;
        }
      }
      debugger("best similar: %.3f", best);
      img.paint(*best_entry, start);
    }
  }

  return {};
}
} // namespace ImageUtils

#endif // __IMAGE_UTILS__
