/*
 *  This file is part of vban.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
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

#ifndef __VBAN_H__
#define __VBAN_H__

#include <inttypes.h>

#define VBAN_HEADER_SIZE            (4 + 4 + 16 + 4)
#define VBAN_HEADER_FOURC           'NABV'
#define VBAN_STREAM_NAME_SIZE       16
#define VBAN_PROTOCOL_MAX_SIZE      1464
#define VBAN_DATA_MAX_SIZE          (VBAN_PROTOCOL_MAX_SIZE - VBAN_HEADER_SIZE)
#define VBAN_CHANNELS_MAX_NB        256
#define VBAN_SAMPLES_MAX_NB         256

struct VBanHeader
{
    uint32_t    vban;                               /* contains 'V' 'B', 'A', 'N' */
    uint8_t     format_SR;                          /* SR index (see SRList above) */
    uint8_t     format_nbs;                         /* nb sample per frame (1 to 256) */
    uint8_t     format_nbc;                         /* nb channel (1 to 256) */
    uint8_t     format_bit;                         /* mask = 0x07 (nb Byte integer from 1 to 4) */
    char        streamname[VBAN_STREAM_NAME_SIZE];  /* stream name */
    uint32_t    nuFrame;                            /* growing frame number. */
} __attribute__((packed));


#define VBAN_SR_MASK                0x1F
#define VBAN_SR_MAXNUMBER           21
static long const VBanSRList[VBAN_SR_MAXNUMBER]=
{
    6000, 12000, 24000, 48000, 96000, 192000, 384000,
    8000, 16000, 32000, 64000, 128000, 256000, 512000,
    11025, 22050, 44100, 88200, 176400, 352800, 705600
};

#define VBAN_PROTOCOL_MASK          0xE0
enum VBanProtocol
{
    VBAN_PROTOCOL_AUDIO         =   0x00,
    VBAN_PROTOCOL_SERIAL        =   0x20,
    VBAN_PROTOCOL_TXT           =   0x40,
    VBAN_PROTOCOL_SERVICE       =   0x60,
    VBAN_PROTOCOL_UNDEFINED_1   =   0x80,
    VBAN_PROTOCOL_UNDEFINED_2   =   0xA0,
    VBAN_PROTOCOL_UNDEFINED_3   =   0xC0,
    VBAN_PROTOCOL_USER          =   0xE0
};

#define VBAN_BIT_RESOLUTION_MASK    0x07
enum VBanBitResolution
{
    VBAN_BITFMT_8_INT = 0,
    VBAN_BITFMT_16_INT,
    VBAN_BITFMT_24_INT,
    VBAN_BITFMT_32_INT,
    VBAN_BITFMT_32_FLOAT,
    VBAN_BITFMT_64_FLOAT,
    VBAN_BITFMT_12_INT,
    VBAN_BITFMT_10_INT,
    VBAN_BIT_RESOLUTION_MAX,
};

static int const VBanBitResolutionSize[VBAN_BIT_RESOLUTION_MAX] =
{
    1, 2, 3, 4, 4, 8, 
};

#define VBAN_RESERVED_MASK          0x08

#define VBAN_CODEC_MASK             0xF0
enum VBanCodec
{
    VBAN_CODEC_PCM              =   0x00,
    VBAN_CODEC_VBCA             =   0x10,
    VBAN_CODEC_VBCV             =   0x20,
    VBAN_CODEC_UNDEFINED_3      =   0x30,
    VBAN_CODEC_UNDEFINED_4      =   0x40,
    VBAN_CODEC_UNDEFINED_5      =   0x50,
    VBAN_CODEC_UNDEFINED_6      =   0x60,
    VBAN_CODEC_UNDEFINED_7      =   0x70,
    VBAN_CODEC_UNDEFINED_8      =   0x80,
    VBAN_CODEC_UNDEFINED_9      =   0x90,
    VBAN_CODEC_UNDEFINED_10     =   0xA0,
    VBAN_CODEC_UNDEFINED_11     =   0xB0,
    VBAN_CODEC_UNDEFINED_12     =   0xC0,
    VBAN_CODEC_UNDEFINED_13     =   0xD0,
    VBAN_CODEC_UNDEFINED_14     =   0xE0,
    VBAN_CODEC_USER             =   0xF0
};


/********************************************************
 *              TEXT SUB PROTOCOL                       *
 ********************************************************/

#define VBAN_BPS_MASK           0xE0
#define VBAN_BPS_MAXNUMBER      25
static long const VBanBPSList[VBAN_BPS_MAXNUMBER] =
{
    0,      110,    150,    300,    600,
    1200,   2400,   4800,   9600,   14400,
    19200,  31250,  38400,  57600,  115200,
    128000, 230400, 250000, 256000, 460800,
    921600,1000000,1500000,2000000, 3000000
};

#define VBAN_DATATYPE_MASK          0x07
#define VBAN_DATATYPE_MAXNUMBER     1
enum VBanDataTypeList
{
    VBAN_DATATYPE_8BITS = 0
};

#define VBAN_STREAMTYPE_MASK        0xF0
enum VBanStreamType
{
    VBAN_TXT_ASCII          =   0x00,
    VBAN_TXT_UTF8           =   0x10,
    VBAN_TXT_WCHAR          =   0x20,
    VBAN_TXT_UNDEFINED_3    =   0x30,
    VBAN_TXT_UNDEFINED_4    =   0x40,
    VBAN_TXT_UNDEFINED_5    =   0x50,
    VBAN_TXT_UNDEFINED_6    =   0x60,
    VBAN_TXT_UNDEFINED_7    =   0x70,
    VBAN_TXT_UNDEFINED_8    =   0x80,
    VBAN_TXT_UNDEFINED_9    =   0x90,
    VBAN_TXT_UNDEFINED_10   =   0xA0,
    VBAN_TXT_UNDEFINED_11   =   0xB0,
    VBAN_TXT_UNDEFINED_12   =   0xC0,
    VBAN_TXT_UNDEFINED_13   =   0xD0,
    VBAN_TXT_UNDEFINED_14   =   0xE0,
    VBAN_TXT_USER           =   0xF0
};

enum VBanServiceType
{
    VBAN_SERVICE_IDENTIFICATION   =   0,
    VBAN_SERVICE_CHATUTF8         =   1,
    VBAN_SERVICE_RTPACKETREGISTER =   32,
    VBAN_SERVICE_RTPACKET         =   33
};

enum VBanServiceFunction
{
    VBAN_SERVICE_FNCT_PING0   =   0x00,
    VBAN_SERVICE_FNCT_REPLY   =   0x80,
};

enum VBanPingType 
{
    VBANPING_TYPE_RECEPTOR        = 0x00000001, // Simple receptor
    VBANPING_TYPE_TRANSMITTER     = 0x00000002, // Simple Transmitter
    VBANPING_TYPE_RECEPTORSPOT    = 0x00000004, // SPOT receptor (able to receive several streams)
    VBANPING_TYPE_TRANSMITTERSPOT = 0x00000008, // SPOT transmitter (able to send several streams)
    VBANPING_TYPE_VIRTUALDEVICE   = 0x00000010, // Virtual Device
    VBANPING_TYPE_VIRTUALMIXER    = 0x00000020, // Virtual Mixer
    VBANPING_TYPE_MATRIX          = 0x00000040, // MATRIX
    VBANPING_TYPE_DAW             = 0x00000080, // Workstation
    VBANPING_TYPE_SERVER          = 0x01000000, // VBAN SERVER
};

enum VBanPingFeatures 
{
    VBANPING_FEATURE_AUDIO  = 0x00000001,
    VBANPING_FEATURE_AOIP   = 0x00000002,
    VBANPING_FEATURE_VOIP   = 0x00000004,
    VBANPING_FEATURE_SERIAL = 0x00000100,
    VBANPING_FEATURE_MIDI   = 0x00000300,
    VBANPING_FEATURE_FRAME  = 0x00001000,
    VBANPING_FEATURE_TXT    = 0x00010000,
};

struct tagVBAN_PING0
{
    uint32_t bitType; /* VBAN device type*/
    uint32_t bitfeature; /* VBAN bit feature */
    uint32_t bitfeatureEx; /* VBAN extra bit feature */
    uint32_t PreferedRate; /* VBAN Preferred sample rate */
    uint32_t MinRate; /* VBAN Min samplerate supported */
    uint32_t MaxRate; /* VBAN Max Samplerate supported */
    uint32_t color_rgb; /* user color */
    uint8_t nVersion[4]; /* App version 4 bytes number */
    char GPS_Position[8]; /* Device position */
    char USER_Position[8]; /* Device position defined by a user process */
    char LangCode_ascii[8]; /* main language used by user FR, EN, etc..*/
    char reserved_ascii[8]; /* unused : must be ZERO*/
    char reservedEx[64]; /* unused : must be ZERO*/
    char DistantIP_ascii[32]; /* Distant IP*/
    uint16_t DistantPort; /* Distant port*/
    uint16_t DistantReserved; /* Reserved*/
    char DeviceName_ascii[64]; /* Device Name (physical device) */
    char ManufacturerName_ascii[64]; /* Manufacturer Name */
    char ApplicationName_ascii[64]; /* Application Name */
    char HostName_ascii[64]; /* dns host name */
    char UserName_utf8[128]; /* User Name */
    char UserComment_utf8[128]; /* User Comment/ Mood/ Remark/ message */
} __attribute__((packed));

#endif /*__VBAN_H__*/
