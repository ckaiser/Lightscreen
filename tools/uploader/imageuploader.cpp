#include "imageuploader.h"
#include "imguruploader.h"

ImageUploader::ImageUploader(QVariantHash &options) : mOptions(options)
{

}

ImageUploader* ImageUploader::getNewUploader(QString name, QVariantHash options)
{
  if (name == "imgur")
      return new ImgurUploader(options);

  return 0;
}
