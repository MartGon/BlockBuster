#pragma once

#include <zip/zip.h>
#include <filesystem>

namespace Util
{
    class Zip
    {
    public:
        Zip() = default;
        virtual ~Zip(){};

        Zip(const Zip&) = delete;
        Zip& operator=(const Zip&) = delete;

        Zip(Zip&&);
        Zip& operator=(Zip&&);

        void ZipFolder(std::filesystem::path folder);
        void ZipFile(std::filesystem::path zipPath, std::filesystem::path filePath);

    protected:
        void ZipFolder(std::filesystem::path folder, std::filesystem::path rootFolder);
        zip_t* zip = nullptr;
    };

    class ZipFile : public Zip
    {
    public:
        ZipFile(std::filesystem::path zipName, char mode, int compressionLevel = ZIP_DEFAULT_COMPRESSION_LEVEL);
        ~ZipFile() override;
    };

    class ZipStream : public Zip
    {
    public:
        ZipStream(char mode, int compressionLevel = ZIP_DEFAULT_COMPRESSION_LEVEL);
        ~ZipStream() override;

        std::string GetBuffer();
    };
}