/*
 * eth_dev_param.cpp
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
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if_arp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "eth_dev_param.h"





Eth_Dev_Param::Eth_Dev_Param() :

    //private
    _sd(-1),
    _opened(false)
{
}



Eth_Dev_Param::~Eth_Dev_Param()
{
//    if( is_open() )
//        this->close(); //close old socket
}



int Eth_Dev_Param::open(const char *dev_name)
{

    this->close(); //close old socket


    if( !dev_name || (strlen(dev_name) >= IFNAMSIZ) )
        return -1;


    _sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if( _sd == -1 )
        return -1;


    strcpy(_ifr.ifr_name, dev_name);


    // try get flags for interface
    if( ioctl(_sd, SIOCGIFFLAGS, &_ifr) == -1 )
    {
       this->close();
       return -1;     //can't get flags
    }


    _opened = true;
    return 0; //good job
}



void Eth_Dev_Param::close()
{
    if( _sd != -1 )
        ::close(_sd);

    memset(&_ifr, 0, sizeof(struct ifreq));
    _opened = false;
    _sd     = -1;
}



int Eth_Dev_Param::set_ip(const char *IP)
{
    if( !is_open() || !IP )
        return -1;


    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    memset(addr, 0, sizeof( struct sockaddr_in) );
    addr->sin_family  = AF_INET;

    if( inet_pton(AF_INET, IP, &addr->sin_addr) <= 0 )
        return -1;


    return ioctl(_sd, SIOCSIFADDR, &_ifr);
}



int Eth_Dev_Param::set_ip(uint32_t IP)
{
    if( !is_open() )
        return -1;


    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    memset(addr, 0, sizeof( struct sockaddr_in) );
    addr->sin_family      = AF_INET;
    addr->sin_addr.s_addr = IP;


    return ioctl(_sd, SIOCSIFADDR, &_ifr);
}



int Eth_Dev_Param::get_ip(char *IP) const
{
    if( !is_open() || !IP )
        return -1;


    if( ioctl(_sd, SIOCGIFADDR, &_ifr) != 0 )
        return -1;

    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;


    if( inet_ntop(AF_INET, &addr->sin_addr, IP, INET_ADDRSTRLEN) != NULL )
        return 0; //good job


    return -1;
}



int Eth_Dev_Param::get_ip(uint32_t *IP) const
{
    if( !is_open() || !IP )
        return -1;


    if( ioctl(_sd, SIOCGIFDSTADDR, &_ifr) != 0 )
        return -1;

    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    *IP = addr->sin_addr.s_addr;


    return 0; //good job
}



int Eth_Dev_Param::set_mask(const char *mask)
{
    if( !is_open() || !mask )
        return -1;


    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    memset(addr, 0, sizeof( struct sockaddr_in) );
    addr->sin_family  = AF_INET;

    if( inet_pton(AF_INET, mask, &addr->sin_addr) <= 0 )
        return -1;


    return ioctl(_sd, SIOCSIFNETMASK, &_ifr);
}



int Eth_Dev_Param::set_mask(uint32_t mask)
{
    if( !is_open() )
        return -1;


    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    memset(addr, 0, sizeof( struct sockaddr_in) );
    addr->sin_family      = AF_INET;
    addr->sin_addr.s_addr = mask;


    return ioctl(_sd, SIOCSIFNETMASK, &_ifr);
}



int Eth_Dev_Param::get_mask(char *mask) const
{
    if( !is_open() || !mask )
        return -1;


    if( ioctl(_sd, SIOCGIFNETMASK, &_ifr) != 0 )
        return -1;

    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;


    if( inet_ntop(AF_INET, &addr->sin_addr, mask, INET_ADDRSTRLEN) != NULL )
        return 0; //good job


    return -1;
}



int Eth_Dev_Param::get_mask(uint32_t *mask) const
{
    if( !is_open() || !mask )
        return -1;


    if( ioctl(_sd, SIOCGIFNETMASK, &_ifr) != 0 )
        return -1;

    struct sockaddr_in* addr = (struct sockaddr_in*)&_ifr.ifr_addr;

    *mask = addr->sin_addr.s_addr;


    return 0; //good job
}



int Eth_Dev_Param::get_mask_prefix() const
{
    int prefix = 0;
    uint32_t mask;
    get_mask(&mask);

    mask = ntohl(mask);

    while(mask != 0)
    {
        mask <<= 1;
        prefix++;
    }

    return prefix;
}



int Eth_Dev_Param::set_gateway(const char *gateway)
{
    if( !is_open() || !gateway )
        return -1;

    char cmd[64];


    sprintf(cmd, "route add %s %s", gateway, _ifr.ifr_name);

    return run_shell_cmd(cmd);
}



int Eth_Dev_Param::set_gateway(uint32_t gateway)
{
    if( !is_open() )
        return -1;

    char cmd[64];

    struct in_addr in;
    in.s_addr = gateway;


    sprintf(cmd, "route add %s %s", inet_ntoa(in), _ifr.ifr_name);

    return run_shell_cmd(cmd);
}



int Eth_Dev_Param::get_gateway(char *gateway) const
{
    if( !is_open() || !gateway )
        return -1;

    uint32_t tmp_gateway;

    struct sockaddr_in s_addr;

    if( get_gateway(&tmp_gateway) != 0 )
        return -1;

    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    s_addr.sin_family      = AF_INET;
    s_addr.sin_addr.s_addr = tmp_gateway;


    if( inet_ntop(AF_INET, &s_addr.sin_addr, gateway, INET_ADDRSTRLEN) != NULL )
        return 0; //good job
    else
        return -1;
}



int Eth_Dev_Param::get_gateway(uint32_t *gateway) const
{
    if( !is_open() || !gateway )
        return -1;


    char devname[64];
    unsigned long d, g, m;
    int flgs, ref, use, metric, mtu, win, ir;


    FILE *fp = fopen("/proc/net/route", "r");

    if( !fp )
        return -1; //can't open file


    if( fscanf(fp, "%*[^\n]\n") < 0 ) // Skip the first line
    {
        fclose(fp);
        return -1;   // Empty or missing line, or read error
    }


    while( !feof(fp) )
    {
        int r;
        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &win, &ir);


        if (r != 11)
        {
            fclose(fp);
            return -1;
        }

        if( strcmp(devname, _ifr.ifr_name) != 0 )
            continue;

        fclose(fp);
        *gateway = g;

        return 0; //good job
    }


    fclose(fp);
    return -1; //can't finde
}



int Eth_Dev_Param::set_hwaddr(const char *hwaddr)
{

    if( !is_open() || !hwaddr || (strlen(hwaddr) != 17) )
        return -1;


    int tmp_mac[6]; // int for sscanf!!!


    sscanf(hwaddr, "%x:%x:%x:%x:%x:%x", &tmp_mac[0], &tmp_mac[1], &tmp_mac[2], &tmp_mac[3], &tmp_mac[4], &tmp_mac[5]);


    _ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    for(int i = 0; i < 6; i++)
        _ifr.ifr_hwaddr.sa_data[i] = tmp_mac[i];


    return ioctl(_sd, SIOCSIFHWADDR, &_ifr);
}



int Eth_Dev_Param::set_hwaddr(const uint8_t *hwaddr)
{
    if( !is_open() || !hwaddr )
        return -1;


    _ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(_ifr.ifr_hwaddr.sa_data, hwaddr, 6);


    char cmd[32];

    sprintf(cmd, "ifconfig %s down", dev_name());
    run_shell_cmd(cmd);

    int ret = ioctl(_sd, SIOCSIFHWADDR, &_ifr);

    sprintf(cmd, "ifconfig %s up", dev_name());
    run_shell_cmd(cmd);


    return ret;
}



int Eth_Dev_Param::get_hwaddr(char *hwaddr) const
{
    if( !is_open() || !hwaddr )
        return -1;


    if( ioctl(_sd, SIOCGIFHWADDR, &_ifr) != 0 )
        return -1;


    if( _ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER )
        return -1;


    uint8_t *tmp_mac = (uint8_t *)_ifr.ifr_hwaddr.sa_data;


    sprintf(hwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
            tmp_mac[0], tmp_mac[1], tmp_mac[2], tmp_mac[3], tmp_mac[4], tmp_mac[5]);


    return 0; //good job
}



int Eth_Dev_Param::get_hwaddr(uint8_t *hwaddr) const
{
    if( !is_open() || !hwaddr )
        return -1;


    if( ioctl(_sd, SIOCGIFHWADDR, &_ifr) != 0 )
        return -1;


    if( _ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER )
        return -1;


    memcpy(hwaddr, _ifr.ifr_hwaddr.sa_data, 6);


    return 0; //good job
}



int Eth_Dev_Param::run_shell_cmd(const char *cmd) const
{
    FILE *ptr;

    if(!cmd)
        return -1;


    ptr = popen(cmd, "w");
    if(!ptr)
        return -1;


    if(pclose(ptr)<0)
        return -1;


    return 0;  //good job
}
