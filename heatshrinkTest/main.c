#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "heatshrink_common.h"
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"


typedef enum _E_CODE_STAGE_
{
    E_SINK = 0,
    E_POLL,
    E_NOTIFY,
    E_FINISH,
}E_CODE_STAGE;


static heatshrink_encoder hsEncoderObj;

static size_t heatshrink_encode(uint8_t* inbuf, uint16_t insize, uint8_t* otbuf, uint16_t outsize)
{
    E_CODE_STAGE stage = 0;

    size_t total_ot_size = 0;

    size_t sink_size_per_time = 0;
    size_t actual_sink_size_per_time = 0;

    size_t poll_size_per_time = 0;
    size_t actual_poll_size_per_time = 0;

    const size_t MAX_SINK_SIZE_PER_TIME = 1024; //(1UL<<HEATSHRINK_STATIC_WINDOW_BITS);

    HSE_sink_res sink_ret = HSER_SINK_OK;
    HSE_poll_res poll_ret = HSER_POLL_EMPTY;
    HSE_finish_res finish_ret = HSER_FINISH_DONE;

    heatshrink_encoder_reset(&hsEncoderObj);

    while(E_FINISH != stage)
    {
        switch(stage)
        {
            case E_SINK:
            {
                sink_size_per_time = (insize > MAX_SINK_SIZE_PER_TIME) ? MAX_SINK_SIZE_PER_TIME : insize;
                if (sink_size_per_time != 0)
                {
                    sink_ret = heatshrink_encoder_sink(&hsEncoderObj, inbuf, sink_size_per_time, &actual_sink_size_per_time);
                    if (sink_ret == HSER_SINK_OK)
                    {
                        insize -= actual_sink_size_per_time;
                        inbuf += actual_sink_size_per_time;

                        if (actual_sink_size_per_time == 0)
                        {
                            stage = E_POLL;
                        }
                    }
                }
                else
                {
                    stage = E_POLL;
                }
            }
            break;

            case E_POLL:
            {
                poll_ret = heatshrink_encoder_poll(&hsEncoderObj, otbuf, outsize, &actual_poll_size_per_time);

                outsize -= actual_poll_size_per_time;
                otbuf += actual_poll_size_per_time;

                total_ot_size += actual_poll_size_per_time;

                if (HSER_POLL_MORE == poll_ret)
                {
                }
                else if (HSER_POLL_EMPTY == poll_ret)
                {
                    if (insize == 0)
                    {
                        stage = E_NOTIFY;
                    }
                    else
                    {
                        stage = E_SINK;
                    }
                }
                else
                {}
            }
            break;

            case E_NOTIFY:
            {
                finish_ret = heatshrink_encoder_finish(&hsEncoderObj);
                if (HSER_FINISH_MORE == finish_ret)
                {
                    poll_ret = heatshrink_encoder_poll(&hsEncoderObj, otbuf, outsize, &actual_poll_size_per_time);

                    outsize -= actual_poll_size_per_time;
                    otbuf += actual_poll_size_per_time;

                    total_ot_size += actual_poll_size_per_time;
                }
                else if (HSER_FINISH_DONE == finish_ret)
                {
                    stage = E_FINISH;
                }
                else
                {}
            }
            break;

            default: break;
        }
    }

    return total_ot_size;
}


static size_t heatshrink_decode(uint8_t* inbuf, uint16_t insize, uint8_t* otbuf, uint16_t outsize)
{
    E_CODE_STAGE stage = 0;

    size_t total_ot_size = 0;

    size_t sink_size_per_time = 0;
    size_t actual_sink_size_per_time = 0;

    size_t poll_size_per_time = 0;
    size_t actual_poll_size_per_time = 0;

    const size_t MAX_SINK_SIZE_PER_TIME = HEATSHRINK_STATIC_INPUT_BUFFER_SIZE;

    HSD_sink_res sink_ret = HSDR_SINK_OK;
    HSD_finish_res finish_ret = HSDR_FINISH_DONE;
    HSD_poll_res poll_ret = HSDR_POLL_EMPTY;

    heatshrink_decoder_reset(&hsEncoderObj);

    while(E_FINISH != stage)
    {
        switch(stage)
        {
            case E_SINK:
            {
                sink_size_per_time = (insize > MAX_SINK_SIZE_PER_TIME) ? MAX_SINK_SIZE_PER_TIME : insize;
                if (sink_size_per_time != 0)
                {
                    sink_ret = heatshrink_decoder_sink(&hsEncoderObj, inbuf, sink_size_per_time, &actual_sink_size_per_time);
                    if (sink_ret == HSDR_SINK_OK)
                    {
                        insize -= actual_sink_size_per_time;
                        inbuf += actual_sink_size_per_time;

                        stage = E_POLL;
                    }
                }
                else
                {
                    stage = E_POLL;
                }
            }
            break;

            case E_POLL:
            {
                poll_ret = heatshrink_decoder_poll(&hsEncoderObj, otbuf, outsize, &actual_poll_size_per_time);

                outsize -= actual_poll_size_per_time;
                otbuf += actual_poll_size_per_time;

                total_ot_size += actual_poll_size_per_time;

                if (HSDR_POLL_MORE == poll_ret)
                {
                }
                else if (HSDR_POLL_EMPTY == poll_ret)
                {
                    if (insize == 0)
                    {
                        stage = E_NOTIFY;
                    }
                    else
                    {
                        stage = E_SINK;
                    }
                }
                else
                {}
            }
            break;

            case E_NOTIFY:
            {
                finish_ret = heatshrink_decoder_finish(&hsEncoderObj);
                if (HSDR_FINISH_MORE == finish_ret)
                {
                    poll_ret = heatshrink_decoder_poll(&hsEncoderObj, otbuf, outsize, &actual_poll_size_per_time);

                    outsize -= actual_poll_size_per_time;
                    otbuf += actual_poll_size_per_time;

                    total_ot_size += actual_poll_size_per_time;
                }
                else if (HSDR_FINISH_DONE == finish_ret)
                {
                    stage = E_FINISH;
                }
                else
                {}
            }
            break;

            default: break;
        }
    }

    return total_ot_size;
}

#define BUFFER_SIZE 1024

uint8_t bufferbeforeCompressed[BUFFER_SIZE] = {0};
uint8_t bufferCompressed[4*BUFFER_SIZE] = {0};
uint8_t bufferDecompressed[4*BUFFER_SIZE] = {0};

int main()
{
    uint16_t ii = 0;
    size_t compressed_size = 0;
    size_t decompressed_size = 0;

    for (ii=0;ii<BUFFER_SIZE;ii++)
    {
        bufferbeforeCompressed[ii] = ii%255;
        // bufferbeforeCompressed[ii] += rand();

        bufferbeforeCompressed[ii] = bufferbeforeCompressed[ii]%255;
    }

    printf("%d is the size of buffer to be compressed.\r\n", sizeof(bufferbeforeCompressed));

    compressed_size = heatshrink_encode(bufferbeforeCompressed, BUFFER_SIZE, bufferCompressed, 4*BUFFER_SIZE);

    printf("%d is the size of compressed buffer.\r\n", compressed_size);

    // for (ii=0;ii<compressed_size;ii++)
    // {
    //     printf("%d ", bufferCompressed[ii]);
    // }
    // printf("\r\n");

    decompressed_size = heatshrink_decode(bufferCompressed, compressed_size, bufferDecompressed, 4*BUFFER_SIZE);

    printf("the size of the recovery buffer is %d.\r\n", decompressed_size);
    // for (ii=0;ii<decompressed_size;ii++)
    // {
    //     printf("%d ", bufferDecompressed[ii]);
    // }
    // printf("\r\n");
    if (decompressed_size == BUFFER_SIZE)
    {
        printf("the size of the recovery buffer is the same as that of the original buffer.\r\n");

        if (memcmp(bufferbeforeCompressed, bufferDecompressed, BUFFER_SIZE) == 0)
        {
            printf("the buffer is recovered completely.\r\n");
        }
        else
        {
            printf("it seems the compression is not no loss compression.\r\n");
        }
    }
    else
    {
        printf("the size of the recovery buffer is not the same as that of the original buffer.\r\n");
    }
}
