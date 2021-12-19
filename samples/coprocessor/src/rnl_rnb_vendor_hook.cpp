/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**
 * @file
 *   This file shows how to implement the NCP vendor hook.
 */
#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
#include "nrf_802154_radio_wrapper.h"
#include <logging/log.h>
#include <ncp_base.hpp>
#include <ncp_hdlc.hpp>
#include <common/new.hpp>

#include "rnb_coprocessor_version.h"

LOG_MODULE_REGISTER(ncp_sample_vendor_hook, CONFIG_OT_COPROCESSOR_LOG_LEVEL);

namespace ot {
namespace Ncp {

static uint8_t mReceiveBuffer[300];


otError NcpBase::VendorCommandHandler(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;
    unsigned int propKey = 0;
    uint16_t len = 0;
    const uint8_t *rnbRequest = mReceiveBuffer;

    LOG_DBG("VendorCommandHandler");

    switch (aCommand)
    {
    case SPINEL_CMD_VENDOR_RNL_RNB_SEND_REQUEST: {
            LOG_WRN("Got SPINEL_CMD_VENDOR_RNL_RNB_SEND_REQUEST");

            SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

            if (propKey == SPINEL_PROP_VENDOR_RNL_RNB_REQUEST)
            {
                SuccessOrExit(error = mDecoder.ReadDataWithLen(rnbRequest, len));

                // LOG_DBG("Got SPINEL_PROP_VENDOR_RNL_RNB_REQUEST with length: %u", len);

                if ((len > 0) && (len <= OT_RADIO_RNL_RNB_REQUEST_MAX_SIZE))
                {
                    error = otPlatRadioRnlRnbSendRequest(mInstance, (const otRadioRnlRnbRequest*) rnbRequest, len);
                }
                else
                {
                    error = OT_ERROR_INVALID_ARGS;
                }
            }
            else
            {
                error = OT_ERROR_INVALID_ARGS;
            }
        }
        break;
    default:
        LOG_ERR("Got UNKNOWN cmd");
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    error = WriteLastStatusFrame(aHeader, ThreadErrorToSpinelStatus(error));

    return error;
}

void NcpBase::VendorHandleFrameRemovedFromNcpBuffer(Spinel::Buffer::FrameTag aFrameTag)
{
    // This method is a callback which mirrors `NcpBase::HandleFrameRemovedFromNcpBuffer()`.
    // It is called when a spinel frame is sent and removed from NCP buffer.
    //
    // (a) This can be used to track and verify that a vendor spinel frame response is
    //     delivered to the host (tracking the frame using its tag).
    //
    // (b) It indicates that NCP buffer space is now available (since a spinel frame is
    //     removed). This can be used to implement reliability mechanisms to re-send
    //     a failed spinel command response (or an async spinel frame) transmission
    //     (failed earlier due to NCP buffer being full).

    OT_UNUSED_VARIABLE(aFrameTag);
}

otError NcpBase::VendorGetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;
    // uint16_t sendLength;
    // uint8_t header;

    switch (aPropKey)
    {
    case SPINEL_PROP_VENDOR_RNL_RNB_COPROCESSOR_VERSION:
        LOG_DBG("Got SPINEL_PROP_VENDOR_RNL_RNB_COPROCESSOR_VERSION");

        SuccessOrExit(error = mEncoder.WriteUtf8(m_rnb_coprocessor_version));
        break;
    default:
        LOG_ERR("Got UNKNOWN prop");
        error = OT_ERROR_NOT_FOUND;
        break;
    }

exit:
    return error;
}

otError NcpBase::VendorSetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
        // TODO: Implement your set properties handlers here.
    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

// exit:
    return error;
}

} // namespace Ncp
} // namespace ot

//------------------------------------------------------------------
// When OPENTHREAD_ENABLE_NCP_VENDOR_HOOK is enabled, vendor code is
// expected to provide the `otNcpInit()` function. The reason behind
// this is to enable vendor code to define its own sub-class of
// `NcpBase` or `NcpHdlc`/`NcpSpi`.
//
// Example below show how to add a vendor sub-class over `NcpHdlc`.

class NcpVendorUart : public ot::Ncp::NcpHdlc
{
    static int SendHdlc(const uint8_t *aBuf, uint16_t aBufLength)
    {
        return mSendCallback(aBuf, aBufLength);
    }

public:
    NcpVendorUart(ot::Instance *aInstance, otNcpHdlcSendCallback aSendCallback)
        : ot::Ncp::NcpHdlc(aInstance, &NcpVendorUart::SendHdlc)
    {
        OT_ASSERT(aSendCallback);
        mSendCallback = aSendCallback;
    }

    // Add public/private methods or member variables
private:
    static otNcpHdlcSendCallback mSendCallback;
};

otNcpHdlcSendCallback NcpVendorUart::mSendCallback;

static OT_DEFINE_ALIGNED_VAR(sNcpVendorRaw, sizeof(NcpVendorUart), uint64_t);


extern "C" void otNcpHdlcInit(otInstance *aInstance, otNcpHdlcSendCallback aSendCallback)
{
    NcpVendorUart *ncpVendor = NULL;
    ot::Instance *instance = static_cast<ot::Instance *>(aInstance);

    ncpVendor = new (&sNcpVendorRaw) NcpVendorUart(instance, aSendCallback);
}
#endif
