#include "imageuploader.h"
#include "imguruploader.h"

ImageUploader *ImageUploader::getNewUploader(const QString &name, const QVariantHash &options)
{
    if (name == "imgur") {
        return new ImgurUploader(options);
    }

    return 0;
}
