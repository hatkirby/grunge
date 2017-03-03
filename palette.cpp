#include "palette.h"
#include <algorithm>

const std::vector<Magick::ColorRGB> palette::reds = {
  {"#ff4848"}, {"#ff7575"}, {"#ff8a8a"}, {"#ff9797"}, {"#ffa8a8"}, {"#ffbbbb"}
};

const std::vector<Magick::ColorRGB> palette::oranges = {
  {"#ffbf85"}, {"#ffcfa4"}, {"#ffbd82"}, {"#ffc48e"}
};

const std::vector<Magick::ColorRGB> palette::yellows = {
  {"#fdfda5"}, {"#eff0ac"}, {"#fef495"}
};

const std::vector<Magick::ColorRGB> palette::greens = {
  {"#c6fcb4"}, {"#d4ffa2"}, {"#93eeaa"}
};

const std::vector<Magick::ColorRGB> palette::blues = {
  {"#62d0ff"}, {"#62a9ff"}, {"#63e9fc"}, {"#7bcae1"}, {"#92fef9"}
};

const std::vector<Magick::ColorRGB> palette::purples = {
  {"#dfb0fe"}, {"#b0a7f1"}, {"#ff86ff"}, {"#ffacec"}, {"#ff86c2"}, {"#ea8dfe"}
};

palette::palette(std::vector<Magick::Color> foci)
{
  if (foci.size() < 1)
  {
    throw std::invalid_argument("Must have at least one focus");
  }

  if (foci.size() < 2)
  {
    // Degenerate scenario, but deal with it gracefully.
    foci.push_back(foci.front());
  }

  int sections = foci.size() - 1;
  double sectionSize = 256.0 / static_cast<double>(sections);
  for (int i=0; i<256; i++)
  {
    int section = std::floor(static_cast<double>(i) / sectionSize);
    double interpolation = (static_cast<double>(i) / sectionSize) - section;

    Magick::ColorHSL interpLeft = foci[section];
    Magick::ColorHSL interpRight = foci[section+1];

    double newHue;
    double diff = interpRight.hue() - interpLeft.hue();
    if (diff < 0)
    {
      std::swap(interpLeft, interpRight);
      
      diff = -diff;
      interpolation = 1 - interpolation;
    }
    
    if (diff > 0.5)
    {
      newHue = 1.0 + interpLeft.hue()
        * (interpolation * (interpRight.hue() - interpLeft.hue() - 1.0));
      
      if (newHue > 1.0)
      {
        newHue -= 1.0;
      }
    } else {
      newHue = interpLeft.hue() + interpolation * diff;
    }
    
    Magick::ColorHSL interpolated(
      newHue,
      ((1.0 - interpolation) * interpLeft.saturation())
        + (interpolation * interpRight.saturation()),
      ((1.0 - interpolation) * interpLeft.luminosity())
        + (interpolation * interpRight.luminosity()));

    gradient_.push_back(interpolated);
  }
}

palette palette::randomPalette(std::mt19937& rng)
{
  std::vector<Magick::Color> foci;
  foci.push_back(reds[
      std::uniform_int_distribution<int>(0, reds.size()-1)(rng)]);
  foci.push_back(oranges[
      std::uniform_int_distribution<int>(0, oranges.size()-1)(rng)]);
  foci.push_back(yellows[
      std::uniform_int_distribution<int>(0, yellows.size()-1)(rng)]);
  foci.push_back(greens[
      std::uniform_int_distribution<int>(0, greens.size()-1)(rng)]);
  foci.push_back(blues[
      std::uniform_int_distribution<int>(0, blues.size()-1)(rng)]);
  foci.push_back(purples[
      std::uniform_int_distribution<int>(0, purples.size()-1)(rng)]);

  std::shuffle(std::begin(foci), std::end(foci), rng);

  int origNum = foci.size();
  int numOfFoci = std::uniform_int_distribution<int>(4, origNum)(rng);
  for (int i=0; i<(origNum-numOfFoci); i++)
  {
    foci.pop_back();
  }

  foci.push_back(foci[0]);

  return palette(std::move(foci));
}

Magick::Color palette::getColor(double shade) const
{
  int index = std::min((int)floor(shade * 256.0), 255);
  return gradient_[index];
}
