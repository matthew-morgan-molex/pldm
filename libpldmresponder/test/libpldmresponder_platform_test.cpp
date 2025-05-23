#include "common/test/mocked_utils.hpp"
#include "common/types.hpp"
#include "common/utils.hpp"
#include "host-bmc/dbus_to_event_handler.hpp"
#include "libpldmresponder/event_parser.hpp"
#include "libpldmresponder/pdr.hpp"
#include "libpldmresponder/pdr_utils.hpp"
#include "libpldmresponder/platform.hpp"
#include "libpldmresponder/platform_numeric_effecter.hpp"
#include "libpldmresponder/platform_state_effecter.hpp"
#include "libpldmresponder/platform_state_sensor.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>
#include <sdeventplus/event.hpp>

using namespace pldm::pdr;
using namespace pldm::utils;
using namespace pldm::responder;
using namespace pldm::responder::platform;
using namespace pldm::responder::pdr;
using namespace pldm::responder::pdr_utils;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

TEST(getPDR, testGoodPath)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    struct pldm_get_pdr_req* request = new (req->payload) pldm_get_pdr_req;
    request->request_count = 100;

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto pdrRepo = pldm_pdr_init();
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    pdrRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo repo(pdrRepo);
    ASSERT_EQ(repo.empty(), false);
    auto response = handler.getPDR(req, requestPayloadLength);
    auto responsePtr = new (response.data()) pldm_msg;

    struct pldm_get_pdr_resp* resp = new (responsePtr->payload)
        pldm_get_pdr_resp;
    ASSERT_EQ(PLDM_SUCCESS, resp->completion_code);
    ASSERT_EQ(2, resp->next_record_handle);
    ASSERT_EQ(true, resp->response_count != 0);

    pldm_pdr_hdr* hdr = new (resp->record_data) pldm_pdr_hdr;
    ASSERT_EQ(hdr->record_handle, 1);
    ASSERT_EQ(hdr->version, 1);

    pldm_pdr_destroy(pdrRepo);
}

TEST(getPDR, testShortRead)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    struct pldm_get_pdr_req* request = new (req->payload) pldm_get_pdr_req;
    request->request_count = 1;

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto pdrRepo = pldm_pdr_init();
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    pdrRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo repo(pdrRepo);
    ASSERT_EQ(repo.empty(), false);
    auto response = handler.getPDR(req, requestPayloadLength);
    auto responsePtr = new (response.data()) pldm_msg;
    struct pldm_get_pdr_resp* resp = new (responsePtr->payload)
        pldm_get_pdr_resp;
    ASSERT_EQ(PLDM_SUCCESS, resp->completion_code);
    ASSERT_EQ(1, resp->response_count);
    pldm_pdr_destroy(pdrRepo);
}

TEST(getPDR, testBadRecordHandle)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    struct pldm_get_pdr_req* request = new (req->payload) pldm_get_pdr_req;
    request->record_handle = 100000;
    request->request_count = 1;

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto pdrRepo = pldm_pdr_init();
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    pdrRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo repo(pdrRepo);
    ASSERT_EQ(repo.empty(), false);
    auto response = handler.getPDR(req, requestPayloadLength);
    auto responsePtr = new (response.data()) pldm_msg;

    ASSERT_EQ(responsePtr->payload[0], PLDM_PLATFORM_INVALID_RECORD_HANDLE);

    pldm_pdr_destroy(pdrRepo);
}

TEST(getPDR, testNoNextRecord)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    struct pldm_get_pdr_req* request = new (req->payload) pldm_get_pdr_req;
    request->record_handle = 1;

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto pdrRepo = pldm_pdr_init();
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    pdrRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo repo(pdrRepo);
    ASSERT_EQ(repo.empty(), false);
    auto response = handler.getPDR(req, requestPayloadLength);
    auto responsePtr = new (response.data()) pldm_msg;
    struct pldm_get_pdr_resp* resp = new (responsePtr->payload)
        pldm_get_pdr_resp;
    ASSERT_EQ(PLDM_SUCCESS, resp->completion_code);
    ASSERT_EQ(2, resp->next_record_handle);

    pldm_pdr_destroy(pdrRepo);
}

TEST(getPDR, testFindPDR)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    struct pldm_get_pdr_req* request = new (req->payload) pldm_get_pdr_req;
    request->request_count = 100;

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto pdrRepo = pldm_pdr_init();
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    pdrRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo repo(pdrRepo);
    ASSERT_EQ(repo.empty(), false);
    auto response = handler.getPDR(req, requestPayloadLength);

    // Let's try to find a PDR of type stateEffecter (= 11) and entity type =
    // 100
    bool found = false;
    uint32_t handle = 0; // start asking for PDRs from recordHandle 0
    while (!found)
    {
        request->record_handle = handle;
        auto response = handler.getPDR(req, requestPayloadLength);
        auto responsePtr = new (response.data()) pldm_msg;
        struct pldm_get_pdr_resp* resp = new (responsePtr->payload)
            pldm_get_pdr_resp;
        ASSERT_EQ(PLDM_SUCCESS, resp->completion_code);

        handle = resp->next_record_handle; // point to the next pdr in case
                                           // current is not what we want

        pldm_pdr_hdr* hdr = new (resp->record_data) pldm_pdr_hdr;
        if (hdr->type == PLDM_STATE_EFFECTER_PDR)
        {
            pldm_state_effecter_pdr* pdr = new (resp->record_data)
                pldm_state_effecter_pdr;
            if (pdr->entity_type == 100)
            {
                found = true;
                // Rest of the PDR can be accessed as need be
                break;
            }
        }
        if (!resp->next_record_handle) // no more records
        {
            break;
        }
    }
    ASSERT_EQ(found, true);

    pldm_pdr_destroy(pdrRepo);
}

TEST(setStateEffecterStatesHandler, testGoodRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    handler.getPDR(req, requestPayloadLength);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_EFFECTER_PDR);
    pdr_utils::PdrEntry e;
    auto record1 = pdr::getRecordByHandle(outRepo, 2, e);
    ASSERT_NE(record1, nullptr);
    pldm_state_effecter_pdr* pdr = new (e.data) pldm_state_effecter_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_STATE_EFFECTER_PDR);

    std::vector<set_effecter_state_field> stateField;
    stateField.push_back({PLDM_REQUEST_SET, 1});
    stateField.push_back({PLDM_REQUEST_SET, 1});
    std::string value = "xyz.openbmc_project.Foo.Bar.V1";
    PropertyValue propertyValue = value;

    DBusMapping dbusMapping{"/foo/bar", "xyz.openbmc_project.Foo.Bar",
                            "propertyName", "string"};

    EXPECT_CALL(mockedUtils, setDbusProperty(dbusMapping, propertyValue))
        .Times(2);
    auto rc = platform_state_effecter::setStateEffecterStatesHandler<
        MockdBusHandler, Handler>(mockedUtils, handler, 0x1, stateField);
    ASSERT_EQ(rc, 0);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(setStateEffecterStatesHandler, testBadRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_PDR_REQ_BYTES>
        requestPayload{};
    auto req = new (requestPayload.data()) pldm_msg;
    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    handler.getPDR(req, requestPayloadLength);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_EFFECTER_PDR);
    pdr_utils::PdrEntry e;
    auto record1 = pdr::getRecordByHandle(outRepo, 2, e);
    ASSERT_NE(record1, nullptr);
    pldm_state_effecter_pdr* pdr = new (e.data) pldm_state_effecter_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_STATE_EFFECTER_PDR);

    std::vector<set_effecter_state_field> stateField;
    stateField.push_back({PLDM_REQUEST_SET, 3});
    stateField.push_back({PLDM_REQUEST_SET, 4});

    auto rc = platform_state_effecter::setStateEffecterStatesHandler<
        MockdBusHandler, Handler>(mockedUtils, handler, 0x1, stateField);
    ASSERT_EQ(rc, PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE);

    rc = platform_state_effecter::setStateEffecterStatesHandler<
        MockdBusHandler, Handler>(mockedUtils, handler, 0x9, stateField);
    ASSERT_EQ(rc, PLDM_PLATFORM_INVALID_EFFECTER_ID);

    stateField.push_back({PLDM_REQUEST_SET, 4});
    rc = platform_state_effecter::setStateEffecterStatesHandler<
        MockdBusHandler, Handler>(mockedUtils, handler, 0x1, stateField);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(setNumericEffecterValueHandler, testGoodRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto numericEffecterPdrRepo = pldm_pdr_init();
    Repo numericEffecterPDRs(numericEffecterPdrRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, numericEffecterPDRs, PLDM_NUMERIC_EFFECTER_PDR);

    pdr_utils::PdrEntry e;
    auto record4 = pdr::getRecordByHandle(numericEffecterPDRs, 4, e);
    ASSERT_NE(record4, nullptr);

    pldm_numeric_effecter_value_pdr* pdr = new (e.data)
        pldm_numeric_effecter_value_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_NUMERIC_EFFECTER_PDR);

    uint16_t effecterId = 3;
    uint32_t effecterValue = 2100000000; // 2036-07-18 21:20:00
    PropertyValue propertyValue = static_cast<uint64_t>(effecterValue);

    DBusMapping dbusMapping{"/foo/bar", "xyz.openbmc_project.Foo.Bar",
                            "propertyName", "uint64_t"};
    EXPECT_CALL(mockedUtils, setDbusProperty(dbusMapping, propertyValue))
        .Times(1);

    auto rc = platform_numeric_effecter::setNumericEffecterValueHandler<
        MockdBusHandler, Handler>(
        mockedUtils, handler, effecterId, PLDM_EFFECTER_DATA_SIZE_UINT32,
        reinterpret_cast<uint8_t*>(&effecterValue), 4);
    ASSERT_EQ(rc, 0);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(numericEffecterPdrRepo);
}

TEST(setNumericEffecterValueHandler, testBadRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto numericEffecterPdrRepo = pldm_pdr_init();
    Repo numericEffecterPDRs(numericEffecterPdrRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, numericEffecterPDRs, PLDM_NUMERIC_EFFECTER_PDR);

    pdr_utils::PdrEntry e;
    auto record4 = pdr::getRecordByHandle(numericEffecterPDRs, 4, e);
    ASSERT_NE(record4, nullptr);

    pldm_numeric_effecter_value_pdr* pdr = new (e.data)
        pldm_numeric_effecter_value_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_NUMERIC_EFFECTER_PDR);

    uint16_t effecterId = 3;
    uint64_t effecterValue = 9876543210;
    auto rc = platform_numeric_effecter::setNumericEffecterValueHandler<
        MockdBusHandler, Handler>(
        mockedUtils, handler, effecterId, PLDM_EFFECTER_DATA_SIZE_SINT32,
        reinterpret_cast<uint8_t*>(&effecterValue), 3);
    ASSERT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(numericEffecterPdrRepo);
}

TEST(getNumericEffecterValueHandler, testGoodRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto numericEffecterPdrRepo = pldm_pdr_init();
    Repo numericEffecterPDRs(numericEffecterPdrRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, numericEffecterPDRs, PLDM_NUMERIC_EFFECTER_PDR);

    pdr_utils::PdrEntry e;
    auto record4 = pdr::getRecordByHandle(numericEffecterPDRs, 4, e);
    ASSERT_NE(record4, nullptr);

    pldm_numeric_effecter_value_pdr* pdr = new (e.data)
        pldm_numeric_effecter_value_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_NUMERIC_EFFECTER_PDR);

    uint16_t effecterId = 3;

    uint8_t effecterDataSize{};
    pldm::utils::PropertyValue dbusValue;
    std::string propertyType;

    // effecterValue return the present numeric setting
    uint32_t effecterValue = 2100000000;
    using effecterOperationalState = uint8_t;
    using completionCode = uint8_t;

    EXPECT_CALL(mockedUtils,
                getDbusPropertyVariant(StrEq("/foo/bar"), StrEq("propertyName"),
                                       StrEq("xyz.openbmc_project.Foo.Bar")))
        .WillOnce(Return(PropertyValue(static_cast<uint64_t>(effecterValue))));

    auto rc = platform_numeric_effecter::getNumericEffecterData<
        MockdBusHandler, Handler>(mockedUtils, handler, effecterId,
                                  effecterDataSize, propertyType, dbusValue);

    ASSERT_EQ(rc, 0);

    size_t responsePayloadLength =
        sizeof(completionCode) + sizeof(effecterDataSize) +
        sizeof(effecterOperationalState) +
        getEffecterDataSize(effecterDataSize) +
        getEffecterDataSize(effecterDataSize);

    Response response(responsePayloadLength + sizeof(pldm_msg_hdr));
    auto responsePtr = new (response.data()) pldm_msg;

    rc = platform_numeric_effecter::getNumericEffecterValueHandler(
        propertyType, dbusValue, effecterDataSize, responsePtr,
        responsePayloadLength, 1);

    ASSERT_EQ(rc, 0);

    struct pldm_get_numeric_effecter_value_resp* resp =
        new (responsePtr->payload) pldm_get_numeric_effecter_value_resp;
    ASSERT_EQ(PLDM_SUCCESS, resp->completion_code);
    uint32_t valPresent;
    memcpy(&valPresent, &resp->pending_and_present_values[4],
           sizeof(valPresent));

    ASSERT_EQ(effecterValue, valPresent);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(numericEffecterPdrRepo);
}

TEST(getNumericEffecterValueHandler, testBadRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto numericEffecterPdrRepo = pldm_pdr_init();
    Repo numericEffecterPDRs(numericEffecterPdrRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_effecter/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, numericEffecterPDRs, PLDM_NUMERIC_EFFECTER_PDR);

    pdr_utils::PdrEntry e;
    auto record4 = pdr::getRecordByHandle(numericEffecterPDRs, 4, e);
    ASSERT_NE(record4, nullptr);

    pldm_numeric_effecter_value_pdr* pdr = new (e.data)
        pldm_numeric_effecter_value_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_NUMERIC_EFFECTER_PDR);

    uint16_t effecterId = 4;

    uint8_t effecterDataSize{};
    pldm::utils::PropertyValue dbusValue;
    std::string propertyType;

    auto rc = platform_numeric_effecter::getNumericEffecterData<
        MockdBusHandler, Handler>(mockedUtils, handler, effecterId,
                                  effecterDataSize, propertyType, dbusValue);

    ASSERT_EQ(rc, 128);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(numericEffecterPdrRepo);
}

TEST(parseStateSensor, allScenarios)
{
    // Sample state sensor with SensorID - 1, EntityType - Processor Module(67)
    // State Set ID - Operational Running Status(11), Supported States - 3,4
    std::vector<uint8_t> sample1PDR{
        0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x17,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x43, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x00, 0x01, 0x18};

    const auto& [terminusHandle1, sensorID1, sensorInfo1] =
        parseStateSensorPDR(sample1PDR);
    const auto& [containerID1, entityType1, entityInstance1] =
        std::get<0>(sensorInfo1);
    const auto& states1 = std::get<1>(sensorInfo1);
    CompositeSensorStates statesCmp1{{3u, 4u}};

    ASSERT_EQ(le16toh(terminusHandle1), 0u);
    ASSERT_EQ(le16toh(sensorID1), 1u);
    ASSERT_EQ(le16toh(containerID1), 0u);
    ASSERT_EQ(le16toh(entityType1), 67u);
    ASSERT_EQ(le16toh(entityInstance1), 1u);
    ASSERT_EQ(states1, statesCmp1);

    // Sample state sensor with SensorID - 2, EntityType - System Firmware(31)
    // State Set ID - Availability(2), Supported States - 3,4,9,10,11,13
    std::vector<uint8_t> sample2PDR{
        0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x17, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x1F, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x18, 0x2E};

    const auto& [terminusHandle2, sensorID2, sensorInfo2] =
        parseStateSensorPDR(sample2PDR);
    const auto& [containerID2, entityType2, entityInstance2] =
        std::get<0>(sensorInfo2);
    const auto& states2 = std::get<1>(sensorInfo2);
    CompositeSensorStates statesCmp2{{3u, 4u, 9u, 10u, 11u, 13u}};

    ASSERT_EQ(le16toh(terminusHandle2), 0u);
    ASSERT_EQ(le16toh(sensorID2), 2u);
    ASSERT_EQ(le16toh(containerID2), 0u);
    ASSERT_EQ(le16toh(entityType2), 31u);
    ASSERT_EQ(le16toh(entityInstance2), 1u);
    ASSERT_EQ(states2, statesCmp2);

    // Sample state sensor with SensorID - 3, EntityType - Virtual Machine
    // Manager(33), Composite State Sensor -2 , State Set ID - Link State(33),
    // Supported States - 1,2, State Set ID - Configuration State(15),
    // Supported States - 1,2,3,4
    std::vector<uint8_t> sample3PDR{
        0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x17, 0x00, 0x00,
        0x00, 0x03, 0x00, 0x21, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x02, 0x21, 0x00, 0x01, 0x06, 0x0F, 0x00, 0x01, 0x1E};

    const auto& [terminusHandle3, sensorID3, sensorInfo3] =
        parseStateSensorPDR(sample3PDR);
    const auto& [containerID3, entityType3, entityInstance3] =
        std::get<0>(sensorInfo3);
    const auto& states3 = std::get<1>(sensorInfo3);
    CompositeSensorStates statesCmp3{{1u, 2u}, {1u, 2u, 3u, 4u}};

    ASSERT_EQ(le16toh(terminusHandle3), 0u);
    ASSERT_EQ(le16toh(sensorID3), 3u);
    ASSERT_EQ(le16toh(containerID3), 1u);
    ASSERT_EQ(le16toh(entityType3), 33u);
    ASSERT_EQ(le16toh(entityInstance3), 2u);
    ASSERT_EQ(states3, statesCmp3);
}

TEST(StateSensorHandler, allScenarios)
{
    using namespace pldm::responder::events;

    StateSensorHandler handler{"./event_jsons/good"};
    constexpr uint8_t eventState0 = 0;
    constexpr uint8_t eventState1 = 1;
    constexpr uint8_t eventState2 = 2;
    constexpr uint8_t eventState3 = 3;

    // Event Entry 1
    {
        StateSensorEntry entry{1, 64, 1, 0, 1, false};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/def",
                            "xyz.openbmc_project.example1.value", "value1",
                            "string"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        const auto& propValue2 = eventStateMap.at(eventState2);
        PropertyValue value0{std::in_place_type<std::string>,
                             "xyz.openbmc_project.State.Normal"};
        PropertyValue value1{std::in_place_type<std::string>,
                             "xyz.openbmc_project.State.Critical"};
        PropertyValue value2{std::in_place_type<std::string>,
                             "xyz.openbmc_project.State.Fatal"};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
        ASSERT_EQ(value2 == propValue2, true);
    }

    // Event Entry 2
    {
        StateSensorEntry entry{1, 64, 1, 1, 1, false};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/def",
                            "xyz.openbmc_project.example2.value", "value2",
                            "uint8_t"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState2);
        const auto& propValue1 = eventStateMap.at(eventState3);
        PropertyValue value0{std::in_place_type<uint8_t>, 9};
        PropertyValue value1{std::in_place_type<uint8_t>, 10};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
    }

    // Event Entry 3
    {
        StateSensorEntry entry{2, 67, 2, 0, 1, false};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/ghi",
                            "xyz.openbmc_project.example3.value", "value3",
                            "bool"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        PropertyValue value0{std::in_place_type<bool>, false};
        PropertyValue value1{std::in_place_type<bool>, true};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
    }

    // Event Entry 4
    {
        StateSensorEntry entry{2, 67, 2, 0, 2, false};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/jkl",
                            "xyz.openbmc_project.example4.value", "value4",
                            "string"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        const auto& propValue2 = eventStateMap.at(eventState2);
        PropertyValue value0{std::in_place_type<std::string>, "Enabled"};
        PropertyValue value1{std::in_place_type<std::string>, "Disabled"};
        PropertyValue value2{std::in_place_type<std::string>, "Auto"};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
        ASSERT_EQ(value2 == propValue2, true);
    }

    // Event Entry 5
    {
        StateSensorEntry entry{0xFFFF, 120, 2, 0, 2, true};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/mno",
                            "xyz.openbmc_project.example5.value", "value5",
                            "string"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        const auto& propValue2 = eventStateMap.at(eventState2);
        PropertyValue value0{std::in_place_type<std::string>, "Enabled"};
        PropertyValue value1{std::in_place_type<std::string>, "Disabled"};
        PropertyValue value2{std::in_place_type<std::string>, "Auto"};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
        ASSERT_EQ(value2 == propValue2, true);
    }
    // Event Entry 6
    {
        StateSensorEntry entry{10, 120, 2, 0, 2, true};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/opk",
                            "xyz.openbmc_project.example6.value", "value6",
                            "string"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        const auto& propValue2 = eventStateMap.at(eventState2);
        PropertyValue value0{std::in_place_type<std::string>, "Enabled"};
        PropertyValue value1{std::in_place_type<std::string>, "Disabled"};
        PropertyValue value2{std::in_place_type<std::string>, "Auto"};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
        ASSERT_EQ(value2 == propValue2, true);
    }
    // Event Entry 7
    {
        StateSensorEntry entry{10, 120, 2, 0, 2, false};
        const auto& [dbusMapping, eventStateMap] = handler.getEventInfo(entry);
        DBusMapping mapping{"/xyz/abc/opk",
                            "xyz.openbmc_project.example6.value", "value6",
                            "string"};
        ASSERT_EQ(mapping == dbusMapping, true);

        const auto& propValue0 = eventStateMap.at(eventState0);
        const auto& propValue1 = eventStateMap.at(eventState1);
        const auto& propValue2 = eventStateMap.at(eventState2);
        PropertyValue value0{std::in_place_type<std::string>, "Enabled"};
        PropertyValue value1{std::in_place_type<std::string>, "Disabled"};
        PropertyValue value2{std::in_place_type<std::string>, "Auto"};
        ASSERT_EQ(value0 == propValue0, true);
        ASSERT_EQ(value1 == propValue1, true);
        ASSERT_EQ(value2 == propValue2, true);
    }

    // Invalid Entry
    {
        StateSensorEntry entry{0, 0, 0, 0, 1, false};
        ASSERT_THROW(handler.getEventInfo(entry), std::out_of_range);
    }
}

TEST(TerminusLocatorPDR, BMCTerminusLocatorPDR)
{
    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    MockdBusHandler mockedUtils;
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "", inPDRRepo, nullptr, nullptr,
                    nullptr, nullptr, nullptr, event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_TERMINUS_LOCATOR_PDR);

    // 1 BMC terminus locator PDR in the PDR repository
    ASSERT_EQ(outRepo.getRecordCount(), 1);

    pdr_utils::PdrEntry entry;
    auto record = pdr::getRecordByHandle(outRepo, 1, entry);
    ASSERT_NE(record, nullptr);

    auto pdr = reinterpret_cast<const pldm_terminus_locator_pdr*>(entry.data);
    EXPECT_EQ(pdr->hdr.record_handle, 1);
    EXPECT_EQ(pdr->hdr.version, 1);
    EXPECT_EQ(pdr->hdr.type, PLDM_TERMINUS_LOCATOR_PDR);
    EXPECT_EQ(pdr->hdr.record_change_num, 0);
    EXPECT_EQ(pdr->hdr.length,
              sizeof(pldm_terminus_locator_pdr) - sizeof(pldm_pdr_hdr));
    EXPECT_EQ(pdr->terminus_handle, TERMINUS_HANDLE);
    EXPECT_EQ(pdr->validity, PLDM_TL_PDR_VALID);
    EXPECT_EQ(pdr->tid, TERMINUS_ID);
    EXPECT_EQ(pdr->container_id, 0);
    EXPECT_EQ(pdr->terminus_locator_type, PLDM_TERMINUS_LOCATOR_TYPE_MCTP_EID);
    EXPECT_EQ(pdr->terminus_locator_value_size,
              sizeof(pldm_terminus_locator_type_mctp_eid));
    auto locatorValue =
        reinterpret_cast<const pldm_terminus_locator_type_mctp_eid*>(
            pdr->terminus_locator_value);
    EXPECT_EQ(locatorValue->eid, pldm::BmcMctpEid);
    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(getStateSensorReadingsHandler, testGoodRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(1)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_sensor/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_SENSOR_PDR);
    pdr_utils::PdrEntry e;
    auto record = pdr::getRecordByHandle(outRepo, 2, e);
    ASSERT_NE(record, nullptr);
    pldm_state_sensor_pdr* pdr = new (e.data) pldm_state_sensor_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_STATE_SENSOR_PDR);

    std::vector<get_sensor_state_field> stateField;
    uint8_t compSensorCnt{};
    uint8_t sensorRearmCnt = 1;

    MockdBusHandler handlerObj;
    EXPECT_CALL(handlerObj,
                getDbusPropertyVariant(StrEq("/foo/bar"), StrEq("propertyName"),
                                       StrEq("xyz.openbmc_project.Foo.Bar")))
        .WillOnce(Return(
            PropertyValue(std::string("xyz.openbmc_project.Foo.Bar.V0"))));
    EventStates cache = {PLDM_SENSOR_NORMAL};
    pldm::stateSensorCacheMaps sensorCache;
    sensorCache.emplace(0x1, cache);
    auto rc = platform_state_sensor::getStateSensorReadingsHandler<
        MockdBusHandler, Handler>(handlerObj, handler, 0x1, sensorRearmCnt,
                                  compSensorCnt, stateField, sensorCache);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(compSensorCnt, 1);
    ASSERT_EQ(stateField[0].sensor_op_state, PLDM_SENSOR_UNAVAILABLE);
    ASSERT_EQ(stateField[0].present_state, PLDM_SENSOR_NORMAL);
    ASSERT_EQ(stateField[0].previous_state, PLDM_SENSOR_NORMAL);
    ASSERT_EQ(stateField[0].event_state, PLDM_SENSOR_UNKNOWN);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(getStateSensorReadingsHandler, testBadRequest)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(1)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    auto event = sdeventplus::Event::get_default();
    Handler handler(&mockedUtils, 0, nullptr, "./pdr_jsons/state_sensor/good",
                    inPDRRepo, nullptr, nullptr, nullptr, nullptr, nullptr,
                    event);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_SENSOR_PDR);
    pdr_utils::PdrEntry e;
    auto record = pdr::getRecordByHandle(outRepo, 2, e);
    ASSERT_NE(record, nullptr);
    pldm_state_sensor_pdr* pdr = new (e.data) pldm_state_sensor_pdr;
    EXPECT_EQ(pdr->hdr.type, PLDM_STATE_SENSOR_PDR);

    std::vector<get_sensor_state_field> stateField;
    uint8_t compSensorCnt{};
    uint8_t sensorRearmCnt = 3;

    MockdBusHandler handlerObj;
    EventStates cache = {PLDM_SENSOR_NORMAL};
    pldm::stateSensorCacheMaps sensorCache;
    sensorCache.emplace(0x1, cache);
    auto rc = platform_state_sensor::getStateSensorReadingsHandler<
        MockdBusHandler, Handler>(handlerObj, handler, 0x1, sensorRearmCnt,
                                  compSensorCnt, stateField, sensorCache);
    ASSERT_EQ(rc, PLDM_PLATFORM_REARM_UNAVAILABLE_IN_PRESENT_STATE);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}
