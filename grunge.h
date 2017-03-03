#ifndef GRUNGE_H
#define GRUNGE_H

#include <random>
#include <twitter.h>
#include <verbly.h>
#include <string>
#include <memory>
#include <Magick++.h>
#include <stdexcept>

class grunge {
public:

  grunge(
    std::string configFile,
    std::mt19937& rng);

  void run() const;

private:

  verbly::word getPicturedNoun() const;

  Magick::Image getImageForNoun(verbly::word pictured) const;

  Magick::Image pixelateImage(Magick::Image image) const;

  Magick::Image pastelizeImage(Magick::Image image) const;

  std::string generateTweetText(verbly::word pictured) const;

  void sendTweet(std::string text, Magick::Image image) const;

  class could_not_get_images : public std::runtime_error {
  public:

    could_not_get_images() : std::runtime_error("Could not get images for noun")
    {
    }
  };

  std::mt19937& rng_;
  std::unique_ptr<verbly::database> database_;
  std::unique_ptr<twitter::client> client_;

};

#endif
