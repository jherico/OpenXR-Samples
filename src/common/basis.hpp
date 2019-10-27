#pragma once

#include <basisu/transcoder/basisu_transcoder.h>
#include <basisu/encoder/basisu_enc.h>

#include <fmt/format.h>

#if defined(XR_USE_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.hpp>
#endif

class BasisReader {
public:
    basist::etc1_global_selector_codebook sel_codebook{ basist::g_global_selector_cb_size, basist::g_global_selector_cb };
    basist::basisu_transcoder dec{ &sel_codebook };
    basist::basisu_file_info fileInfo;
    basist::basisu_image_info imageInfo;
    const uint8_t* const data;
    const uint32_t dataSize;

public:
    BasisReader(const uint8_t* const data_, const size_t size_) : data(data_), dataSize((uint32_t)size_) {
        if (!size_) {
            throw std::runtime_error("File is empty!");
        }

        if (size_ > UINT32_MAX) {
            throw std::runtime_error("File is too large!");
        }

        // Validate the file - note this isn't necessary for transcoding
        if (!dec.validate_file_checksums(data, dataSize, true)) {
            throw std::runtime_error("File version is unsupported, or file fail CRC checks!");
        }

        if (!dec.get_file_info(data, dataSize, fileInfo)) {
            throw std::runtime_error("Failed retrieving Basis file information!");
        }

        if (!dec.get_image_info(data, dataSize, imageInfo, 0)) {
            throw std::runtime_error("get_image_info() failed!\n");
        }

        if (!dec.start_transcoding(data, dataSize)) {
            throw std::runtime_error("start_transcoding() failed!");
        }
    }

    uint32_t getImageSize(uint32_t arrayIndex = 0, uint32_t faceIndex = 0) const {
        return imageInfo.m_orig_height * imageInfo.m_orig_width * sizeof(uint32_t);
    }

    void readImageToBuffer(void* outputBuffer, uint32_t arrayIndex = 0, uint32_t faceIndex = 0) const {
        uint32_t imageIndex = arrayIndex * (basist::cBASISTexTypeCubemapArray ? 6 : 1);
        imageIndex += faceIndex;
        if (!dec.transcode_image_level(data, dataSize, imageIndex, 0, outputBuffer,
                                       imageInfo.m_orig_height * imageInfo.m_orig_width, basist::cTFRGBA32, 0)) {
            throw std::runtime_error(fmt::format("Failed transcoding image level (%u)!\n", imageIndex));
        }
    }
};
