#include <Zip.h>

using namespace Util;

Zip::Zip(Zip&& other)
{
    *this = std::move(other);
}

Zip& Zip::operator=(Zip&& other)
{
    std::swap(zip, other.zip);

    return *this;
}

#include <iostream>

void Zip::ZipFolder(std::filesystem::path dir)
{
    ZipFolder(dir, dir);
}

void Zip::ZipFolder(std::filesystem::path dir, std::filesystem::path rootFolder)
{
    for(const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if(entry.is_directory())
            ZipFolder(entry.path(), rootFolder);
        else
        {
            // This is necessary to prevent creating the full folder path in the zip file
            auto filePath = entry.path();
            auto parentPathStr = rootFolder.parent_path().string();
            auto fileStr = filePath.string().substr(parentPathStr.size());
            ZipFile(fileStr, filePath);
        }
    }
}

void Zip::ZipFile(std::filesystem::path zipPath, std::filesystem::path filePath)
{
    zip_entry_open(zip, zipPath.c_str());
    zip_entry_fwrite(zip, filePath.c_str());
    zip_entry_close(zip);
}

ZipFile::ZipFile(std::filesystem::path zipName, char mode, int compressionLevel)
{
    zip = zip_open(zipName.c_str(), compressionLevel, mode);
}

ZipFile::~ZipFile()
{
    zip_close(zip);
}

ZipStream::ZipStream(char mode, int compressionLevel)
{
    zip = zip_stream_open(nullptr, 0, compressionLevel, mode);
}

ZipStream::~ZipStream()
{
    zip_stream_close(zip);
}

std::string ZipStream::GetBuffer()
{
    // Write to buffer
    char* outbuf = nullptr;
    size_t size = 0;
    zip_stream_copy(zip, (void**)&outbuf, &size);

    // Copy it to the string
    std::string buffer{outbuf, size};
    auto stringSize = buffer.size();

    // Free memory
    free(outbuf);

    return std::move(buffer);
}