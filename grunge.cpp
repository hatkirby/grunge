#include "grunge.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <curl_easy.h>
#include <curl_header.h>
#include <deque>
#include "palette.h"

grunge::grunge(
  std::string configFile,
  std::mt19937& rng) :
    rng_(rng)
{
  // Load the config file.
  YAML::Node config = YAML::LoadFile(configFile);

  // Set up the verbly database.
  database_ = std::unique_ptr<verbly::database>(
    new verbly::database(config["verbly_datafile"].as<std::string>()));

  // Set up the Twitter client.
  twitter::auth auth;
  auth.setConsumerKey(config["consumer_key"].as<std::string>());
  auth.setConsumerSecret(config["consumer_secret"].as<std::string>());
  auth.setAccessKey(config["access_key"].as<std::string>());
  auth.setAccessSecret(config["access_secret"].as<std::string>());

  client_ = std::unique_ptr<twitter::client>(new twitter::client(auth));
}

void grunge::run() const
{
  for (;;)
  {
    std::cout << "Generating tweet..." << std::endl;

    try
    {
      // Find a noun to use as the pictured item.
      std::cout << "Choosing pictured noun..." << std::endl;

      verbly::word pictured = getPicturedNoun();

      std::cout << "Noun: " << pictured.getBaseForm().getText() << std::endl;

      // Choose a picture of that noun.
      std::cout << "Finding an image..." << std::endl;

      Magick::Image image = getImageForNoun(pictured);

      // Pixelate the image.
      std::cout << "Pixelating image..." << std::endl;

      image = pixelateImage(std::move(image));

      // Pastelize the image.
      std::cout << "Pastelizing image..." << std::endl;

      image = pastelizeImage(std::move(image));

      // Generate the tweet text.
      std::cout << "Generating text..." << std::endl;

      std::string text = generateTweetText(pictured);

      std::cout << "Tweet text: " << text << std::endl;

      // Send the tweet.
      std::cout << "Sending tweet..." << std::endl;

      sendTweet(std::move(text), std::move(image));

      std::cout << "Tweeted!" << std::endl;

      // Wait.
      std::this_thread::sleep_for(std::chrono::hours(1));
    } catch (const could_not_get_images& ex)
    {
      std::cout << ex.what() << std::endl;
    } catch (const Magick::ErrorImage& ex)
    {
      std::cout << "Image error: " << ex.what() << std::endl;
    } catch (const Magick::ErrorCorruptImage& ex)
    {
      std::cout << "Corrupt image: " << ex.what() << std::endl;
    } catch (const twitter::twitter_error& ex)
    {
      std::cout << "Twitter error: " << ex.what() << std::endl;

      std::this_thread::sleep_for(std::chrono::hours(1));
    }

    std::cout << std::endl;
  }
}

verbly::word grunge::getPicturedNoun() const
{
  verbly::filter whitelist =
    (verbly::notion::wnid == 109287968)    // Geological formations
    || (verbly::notion::wnid == 109208496) // Asterisms (collections of stars)
    || (verbly::notion::wnid == 109239740) // Celestial bodies
    || (verbly::notion::wnid == 109277686) // Exterrestrial objects (comets and meteroids)
    || (verbly::notion::wnid == 109403211) // Radiators (supposedly natural radiators but actually these are just pictures of radiators)
    || (verbly::notion::wnid == 109416076) // Rocks
    || (verbly::notion::wnid == 105442131) // Chromosomes
    || (verbly::notion::wnid == 100324978) // Tightrope walking
    || (verbly::notion::wnid == 100326094) // Rock climbing
    || (verbly::notion::wnid == 100433458) // Contact sports
    || (verbly::notion::wnid == 100433802) // Gymnastics
    || (verbly::notion::wnid == 100439826) // Track and field
    || (verbly::notion::wnid == 100440747) // Skiing
    || (verbly::notion::wnid == 100441824) // Water sport
    || (verbly::notion::wnid == 100445351) // Rowing
    || (verbly::notion::wnid == 100446980) // Archery
      // TODO: add more sports
    || (verbly::notion::wnid == 100021939) // Artifacts
    || (verbly::notion::wnid == 101471682) // Vertebrates
      ;

  verbly::filter blacklist =
    (verbly::notion::wnid == 106883725) // swastika
    || (verbly::notion::wnid == 104416901) // tetraskele
    || (verbly::notion::wnid == 102512053) // fish
    || (verbly::notion::wnid == 103575691) // instrument of execution
    || (verbly::notion::wnid == 103829563) // noose
      ;

  verbly::query<verbly::word> pictureQuery = database_->words(
    (verbly::notion::fullHypernyms %= whitelist)
    && !(verbly::notion::fullHypernyms %= blacklist)
    && (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
    && (verbly::notion::numOfImages >= 1));

  verbly::word pictured = pictureQuery.first();

  return pictured;
}

Magick::Image grunge::getImageForNoun(verbly::word pictured) const
{
  // Accept string from Google Chrome
  std::string accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
  curl::curl_header headers;
  headers.add(accept);

  int backoff = 0;

  std::cout << "Getting URLs..." << std::endl;

  std::string lstdata;
  while (lstdata.empty())
  {
    std::ostringstream lstbuf;
    curl::curl_ios<std::ostringstream> lstios(lstbuf);
    curl::curl_easy lsthandle(lstios);
    std::string lsturl = pictured.getNotion().getImageNetUrl();
    lsthandle.add<CURLOPT_URL>(lsturl.c_str());
    lsthandle.add<CURLOPT_CONNECTTIMEOUT>(30);
    lsthandle.add<CURLOPT_TIMEOUT>(300);

    try
    {
      lsthandle.perform();
    } catch (const curl::curl_easy_exception& e)
    {
      e.print_traceback();

      backoff++;
      std::cout << "Waiting for " << backoff << " seconds..." << std::endl;

      std::this_thread::sleep_for(std::chrono::seconds(backoff));

      continue;
    }

    backoff = 0;

    if (lsthandle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
    {
      throw could_not_get_images();
    }

    std::cout << "Got URLs." << std::endl;
    lstdata = lstbuf.str();
  }

  std::vector<std::string> lstvec = verbly::split<std::vector<std::string>>(lstdata, "\r\n");
  if (lstvec.empty())
  {
    throw could_not_get_images();
  }

  std::shuffle(std::begin(lstvec), std::end(lstvec), rng_);

  std::deque<std::string> urls;
  for (std::string& url : lstvec)
  {
    urls.push_back(url);
  }

  bool found = false;
  Magick::Blob img;
  Magick::Image pic;

  while (!found && !urls.empty())
  {
    std::string url = urls.front();
    urls.pop_front();

    std::ostringstream imgbuf;
    curl::curl_ios<std::ostringstream> imgios(imgbuf);
    curl::curl_easy imghandle(imgios);

    imghandle.add<CURLOPT_HTTPHEADER>(headers.get());
    imghandle.add<CURLOPT_URL>(url.c_str());
    imghandle.add<CURLOPT_CONNECTTIMEOUT>(30);
    imghandle.add<CURLOPT_TIMEOUT>(300);

    try
    {
      imghandle.perform();
    } catch (const curl::curl_easy_exception& error) {
      error.print_traceback();

      continue;
    }

    if (imghandle.get_info<CURLINFO_RESPONSE_CODE>().get() != 200)
    {
      continue;
    }

    std::string content_type = imghandle.get_info<CURLINFO_CONTENT_TYPE>().get();
    if (content_type.substr(0, 6) != "image/")
    {
      continue;
    }

    std::string imgstr = imgbuf.str();
    img = Magick::Blob(imgstr.c_str(), imgstr.length());

    try
    {
      pic.read(img);

      if (pic.rows() > 0)
      {
        std::cout << url << std::endl;
        found = true;
      }
    } catch (const Magick::ErrorOption& e)
    {
      // Occurs when the the data downloaded from the server is malformed
      std::cout << "Magick: " << e.what() << std::endl;
    }
  }

  if (!found)
  {
    throw could_not_get_images();
  }

  return pic;
}

Magick::Image grunge::pixelateImage(Magick::Image image) const
{
  // Check that the image is at least 800 pixels in width.
  if (image.columns() < 800)
  {
    Magick::Geometry blownUp(
      800,
      image.rows() * 800 / image.columns());

    image.zoom(blownUp);
  }

  // Check that the image dimensions are a multiple of four.
  if ((image.rows() % 4 != 0) || (image.columns() % 4 != 0))
  {
    Magick::Geometry cropped(
      image.columns() - (image.columns() % 4),
      image.rows() - (image.rows() % 4));

    image.crop(cropped);
  }

  // Downscale the image.
  Magick::Geometry originalSize = image.size();
  Magick::Geometry pixelatedSize(
    originalSize.width() / 4,
    originalSize.height() / 4);

  image.scale(pixelatedSize);

  // Scale the image back up.
  image.scale(originalSize);

  return image;
}

Magick::Image grunge::pastelizeImage(Magick::Image input) const
{
  input.quantizeColorSpace(Magick::GRAYColorspace);
  input.quantizeColors(256);
  input.quantize();

  palette pastelPalette = palette::randomPalette(rng_);
  Magick::Geometry size = input.size();
  Magick::Image pastelized(size, "white");

  for (int y=0; y<size.height(); y++)
  {
    for (int x=0; x<size.width(); x++)
    {
      Magick::ColorGray grade =
        static_cast<Magick::ColorGray>(input.pixelColor(x, y));
      Magick::Color mapped = pastelPalette.getColor(grade.shade());

      pastelized.pixelColor(x, y, mapped);
    }
  }

  return pastelized;
}

std::string grunge::generateTweetText(verbly::word pictured) const
{
  verbly::filter formFilter =
    (verbly::form::proper == false)
    // Blacklist slurs and slur homographys
    && !(verbly::word::usageDomains %= (verbly::notion::wnid == 106717170));

  verbly::word simpler = database_->words(
    (verbly::notion::partOfSpeech == verbly::part_of_speech::noun)
    && (verbly::notion::fullHyponyms %= pictured)
    && (verbly::word::forms(verbly::inflection::base) %= formFilter)).first();

  std::vector<std::string> symbols = {"☯","✡","☨","✞","✝","☮","☥","☦","☪","✌"};
  std::string prefix;
  std::string suffix;
  int length = std::geometric_distribution<int>(0.5)(rng_) + 1;
  for (int i=0; i<length; i++)
  {
    std::string choice = symbols[
      std::uniform_int_distribution<int>(0, symbols.size()-1)(rng_)];

    prefix += choice;
    suffix = choice + suffix;
  }

  verbly::token action = {
    prefix,
    "follow for more soft grunge",
    simpler,
    suffix
  };

  return action.compile();
}

void grunge::sendTweet(std::string text, Magick::Image image) const
{
  Magick::Blob outputBlob;
  image.magick("jpg");
  image.write(&outputBlob);

  long media_id = client_->uploadMedia("image/jpeg", (const char*) outputBlob.data(), outputBlob.length());
  client_->updateStatus(std::move(text), {media_id});
}
