#ifndef SERVICECONTEXT_H
#define SERVICECONTEXT_H


#include <string>
#include <vector>
#include <map>

#include "soapH.h"
#include "eth_dev_param.h"





class StreamProfile
{
    public:

        StreamProfile() { clear(); }

        std::string  get_name   (void) const { return name;   }
        int          get_width  (void) const { return width;  }
        int          get_height (void) const { return height; }
        std::string  get_url    (void) const { return url;    }
        std::string  get_snapurl(void) const { return snapurl;}
        int          get_type   (void) const { return type;   }



        tt__Profile*     get_profile(struct soap *soap) const;
        tt__VideoSource* get_video_src(struct soap *soap) const;

        tt__VideoSourceConfiguration*  get_video_src_cnf(struct soap *soap) const;
        tt__VideoEncoderConfiguration* get_video_enc_cfg(struct soap *soap) const;
        tt__PTZConfiguration*          get_ptz_cfg(struct soap *soap) const;



        //methods for parsing opt from cmd
        bool set_name   (const char *new_val);
        bool set_width  (const char *new_val);
        bool set_height (const char *new_val);
        bool set_url    (const char *new_val);
        bool set_snapurl(const char *new_val);
        bool set_type   (const char *new_val);


        std::string get_str_err()  const { return str_err;         }
        const char* get_cstr_err() const { return str_err.c_str(); }

        void clear(void);
        bool is_valid(void) const;


    private:

        std::string  name;
        int          width;
        int          height;
        std::string  url;
        std::string  snapurl;
        int          type;


        std::string  str_err;
};





class PTZNode
{
    public:

        PTZNode() { clear(); }

        bool         enable;

        std::string  get_move_left   (void) const { return move_left;   }
        std::string  get_move_right  (void) const { return move_right;  }
        std::string  get_move_up     (void) const { return move_up;     }
        std::string  get_move_down   (void) const { return move_down;   }
        std::string  get_move_stop   (void) const { return move_stop;   }
        std::string  get_move_preset (void) const { return move_preset;   }



        //methods for parsing opt from cmd
        bool set_move_left   (const char *new_val) { return set_str_value(new_val, move_left  ); }
        bool set_move_right  (const char *new_val) { return set_str_value(new_val, move_right ); }
        bool set_move_up     (const char *new_val) { return set_str_value(new_val, move_up    ); }
        bool set_move_down   (const char *new_val) { return set_str_value(new_val, move_down  ); }
        bool set_move_stop   (const char *new_val) { return set_str_value(new_val, move_stop  ); }
        bool set_move_preset (const char *new_val) { return set_str_value(new_val, move_preset); }


        std::string get_str_err()  const { return str_err;         }
        const char* get_cstr_err() const { return str_err.c_str(); }

        void clear(void);


    private:

        std::string  move_left;
        std::string  move_right;
        std::string  move_up;
        std::string  move_down;
        std::string  move_stop;
        std::string  move_preset;


        std::string  str_err;

        bool set_str_value(const char *new_val, std::string& value);
};





class ServiceContext
{
    public:

        ServiceContext();


        int         port;
        std::string user;
        std::string password;


        //Device Information
        std::string manufacturer;
        std::string model;
        std::string firmware_version;
        std::string serial_number;
        std::string hardware_id;

        std::vector<std::string> scopes;


        std::vector<Eth_Dev_Param> eth_ifs; //ethernet interfaces


        std::string getServerIpFromClientIp(uint32_t client_ip) const;
        std::string getXAddr(struct soap* soap) const;



        std::string get_str_err() const { return str_err;         }
        const char* get_cstr_err()const { return str_err.c_str(); }


        bool add_profile(const StreamProfile& profile);


        std::string get_stream_uri(const std::string& profile_url, uint32_t client_ip) const;
        std::string get_snapshot_uri(const std::string& profile_url, uint32_t client_ip) const;


        const std::map<std::string, StreamProfile> &get_profiles(void) { return profiles; }
        PTZNode* get_ptz_node(void) { return &ptz_node; }

        // service capabilities
        tds__DeviceServiceCapabilities* getDeviceServiceCapabilities(struct soap* soap);
        trt__Capabilities*  getMediaServiceCapabilities    (struct soap* soap);
        tptz__Capabilities*  getPTZServiceCapabilities     (struct soap* soap);
//        timg__Capabilities* getImagingServiceCapabilities  (struct soap* soap);
//        trc__Capabilities*  getRecordingServiceCapabilities(struct soap* soap);
//        tse__Capabilities*  getSearchServiceCapabilities   (struct soap* soap);
//        trv__Capabilities*  getReceiverServiceCapabilities (struct soap* soap);
//        trp__Capabilities*  getReplayServiceCapabilities   (struct soap* soap);
//        tev__Capabilities*  getEventServiceCapabilities    (struct soap* soap);
//        tls__Capabilities*  getDisplayServiceCapabilities  (struct soap* soap);
//        tmd__Capabilities*  getDeviceIOServiceCapabilities (struct soap* soap);

    private:

        std::map<std::string, StreamProfile> profiles;
        PTZNode ptz_node;

        std::string  str_err;
};





#endif // SERVICECONTEXT_H
