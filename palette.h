#ifndef PALETTE_H
#define PALETTE_H

#include <Magick++.h>
#include <random>
#include <vector>

class palette {
public:

  palette(std::vector<Magick::Color> foci);

  static palette randomPalette(std::mt19937& rng);

  Magick::Color getColor(double shade) const;

private:

  static const std::vector<Magick::ColorRGB> reds;
  static const std::vector<Magick::ColorRGB> oranges;
  static const std::vector<Magick::ColorRGB> yellows;
  static const std::vector<Magick::ColorRGB> greens;
  static const std::vector<Magick::ColorRGB> blues;
  static const std::vector<Magick::ColorRGB> purples;

  std::vector<Magick::Color> gradient_;

};

#endif
