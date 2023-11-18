#include "sigma_connect.h"

void SigmaTcpServer::setup_sigma_tcp_server()
{
    sigma_tcp_server.begin(SIGMA_TCP_PORT);
}

void SigmaTcpServer::handle_sigma_tcp_client()
{
    WiFiClient client = sigma_tcp_server.available();
    size_t count, ret = 0;

    u_int8_t command;
    u_int16_t addr, len;

    u_int16_t buf_size = SIGMA_TCP_BUFFER_SIZE;

    u_int8_t *buf = (u_int8_t *)malloc(buf_size);
    u_int8_t *p = buf;

    while (client)
    {
        memmove(buf, p, count);
        p = buf + count;

        ret = client.readBytes(p, buf_size - count);
        if (ret <= 0)
            break;

        p = buf;

        count += ret;

        while (count >= 7)
        {
            command = p[0];
            /*			total_len = (p[1] << 8) | p[2];*/
            len = (p[4] << 8) | p[5];
            addr = (p[6] << 8) | p[7];

            if (command == SIGMA_TCP_COMMAND_READ)
            {
                p += 8;
                count -= 8;

                i2s_device.requestFrom(addr, len);

                buf[0] = SIGMA_TCP_COMMAND_WRITE;
                buf[1] = (0x4 + len) >> 8;
                buf[2] = (0x4 + len) & 0xff;
                buf[3] = i2s_device.readBytes(buf + 4, len);
                client.write(buf, 4 + len);
            }
            else
            {
                /* not enough data, fetch next bytes */
                if (count < len + 8)
                {
                    if (buf_size < len + 8)
                    {
                        buf_size = len + 8;
                        buf = (u_int8_t *)realloc(buf, buf_size);
                        if (!buf)
							goto exit;
                    }
                    break;
                }
                i2s_device.beginTransmission(addr);
                i2s_device.write(p + 8, len);
                Wire.endTransmission();

                p += len + 8;
                count -= len + 8;
            }
        }

        if (!client.connected())
        {
            break;
        }
    }
    
exit:
    free(buf);
}
