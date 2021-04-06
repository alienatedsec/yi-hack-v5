/*
 * eth_dev_param.h
 *
 *
 * version 1.0
 *
 *
 * Copyright (c) 2015, Koynov Stas - skojnov@yandex.ru
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ETH_DEV_PARAM_H
#define ETH_DEV_PARAM_H

#include <stdint.h>
#include <net/if.h>





class Eth_Dev_Param
{
    public:
         Eth_Dev_Param();
        ~Eth_Dev_Param();

        bool is_open() const { return _opened; }

        int open(const char *dev_name);
        void close();

        const char *dev_name() const { return _ifr.ifr_name; }


        int set_ip(const char *IP);
        int set_ip(uint32_t IP);

        int get_ip(char *IP) const;
        int get_ip(uint32_t *IP) const;


        int set_mask(const char *mask);
        int set_mask(uint32_t mask);

        int get_mask(char *mask) const;
        int get_mask(uint32_t *mask) const;
        int get_mask_prefix() const;


        int set_gateway(const char *gateway);
        int set_gateway(uint32_t gateway);

        int get_gateway(char *gateway) const;
        int get_gateway(uint32_t *gateway) const;


        int set_hwaddr(const char *hwaddr);
        int set_hwaddr(const uint8_t *hwaddr);

        int get_hwaddr(char *hwaddr) const;
        int get_hwaddr(uint8_t *hwaddr) const;


    private:
        int          _sd;
        bool         _opened;
        struct ifreq _ifr;

        int run_shell_cmd(const char *cmd) const;
};





#endif // ETH_DEV_PARAM_H
