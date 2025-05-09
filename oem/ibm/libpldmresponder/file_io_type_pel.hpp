#pragma once

#include "file_io_by_type.hpp"

namespace pldm
{
namespace responder
{

/** @class PelHandler
 *
 *  @brief Inherits and implements FileHandler. This class is used
 *  to read/write pels.
 */
class PelHandler : public FileHandler
{
  public:
    /** @brief PelHandler constructor
     */
    PelHandler(uint32_t fileHandle) : FileHandler(fileHandle) {}

    virtual int writeFromMemory(uint32_t offset, uint32_t length,
                                uint64_t address,
                                oem_platform::Handler* /*oemPlatformHandler*/);

    virtual int readIntoMemory(uint32_t offset, uint32_t length,
                               uint64_t address,
                               oem_platform::Handler* /*oemPlatformHandler*/);

    virtual int read(uint32_t offset, uint32_t& length, Response& response,
                     oem_platform::Handler* /*oemPlatformHandler*/);

    virtual int write(const char* /*buffer*/, uint32_t /*offset*/,
                      uint32_t& /*length*/,
                      oem_platform::Handler* /*oemPlatformHandler*/);

    virtual int fileAck(uint8_t fileStatus);

    virtual int fileAckWithMetaData(
        uint8_t /*fileStatus*/, uint32_t /*metaDataValue1*/,
        uint32_t /*metaDataValue2*/, uint32_t /*metaDataValue3*/,
        uint32_t /*metaDataValue4*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    /** @brief method to store a pel file in tempfs and send
     *  d-bus notification to pel daemon that it is ready for consumption
     *
     *  @param[in] pelFileName - the pel file path
     */
    virtual int storePel(std::string&& pelFileName);

    virtual int newFileAvailable(uint64_t /*length*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    virtual int newFileAvailableWithMetaData(
        uint64_t /*length*/, uint32_t /*metaDataValue1*/,
        uint32_t /*metaDataValue2*/, uint32_t /*metaDataValue3*/,
        uint32_t /*metaDataValue4*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    /** @brief PelHandler destructor
     */
    ~PelHandler() {}
};

} // namespace responder
} // namespace pldm
