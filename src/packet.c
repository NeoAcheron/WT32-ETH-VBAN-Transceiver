/*
 *  This file is part of vban.
 *  Copyright (c) 2017 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "packet.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static size_t vban_sr_from_value(unsigned int value);

int packet_get_max_payload_size(char const* buffer)
{
    int sample_count = 0;
    int sample_size = 0;

    struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);

    if (buffer == 0)
    {
        return -EINVAL;
    }

    // size in bytes cannot exceed VBAN_DATA_MAX_SIZE
    // size in samples cannot exceed VBAN_SAMPLES_MAX_NB
    sample_size = ((hdr->format_nbc+1) * VBanBitResolutionSize[(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)]);
    sample_count = VBAN_DATA_MAX_SIZE / sample_size;
    if (sample_count > VBAN_SAMPLES_MAX_NB)
    {
        sample_count = VBAN_SAMPLES_MAX_NB;
    }

    return sample_count * sample_size;
}

int packet_get_stream_config(char const* buffer, struct stream_config_t* stream_config)
{
    struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);

    memset(stream_config, 0, sizeof(struct stream_config_t));

    if ((buffer == 0) || (stream_config == 0))
    {
        return -EINVAL;
    }

    /** no, I don't check again if this is a valid audio pcm packet...*/

    stream_config->nb_channels  = hdr->format_nbc + 1;
    stream_config->sample_rate  = VBanSRList[hdr->format_SR & VBAN_SR_MASK];
    stream_config->bit_fmt      = hdr->format_bit & VBAN_BIT_RESOLUTION_MASK;

    return 0;
}

int packet_init_header(char* buffer, struct stream_config_t const* stream_config, char const* streamname)
{
    struct VBanHeader* const hdr = PACKET_HEADER_PTR(buffer);

    if ((buffer == 0) || (stream_config == 0))
    {
        return -EINVAL;
    }

    hdr->vban       = VBAN_HEADER_FOURC;
    hdr->format_nbc = stream_config->nb_channels - 1;
    hdr->format_SR  = vban_sr_from_value(stream_config->sample_rate);
    hdr->format_bit = stream_config->bit_fmt;
    strncpy(hdr->streamname, streamname, VBAN_STREAM_NAME_SIZE-1);
    hdr->nuFrame    = 0;

    return 0;
}

int packet_set_new_content(char* buffer, size_t payload_size)
{
    struct VBanHeader* const hdr = PACKET_HEADER_PTR(buffer);

    if (buffer == 0)
    {
        return -EINVAL;
    }
    hdr->format_nbs = (payload_size / ((hdr->format_nbc+1) * VBanBitResolutionSize[(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)])) - 1;
    ++hdr->nuFrame;

    return 0;
}

