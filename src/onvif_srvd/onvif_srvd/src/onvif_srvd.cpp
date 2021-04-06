#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>


#include "daemon.h"
#include "smacros.h"
#include "ServiceContext.h"

// ---- gsoap ----
#include "DeviceBinding.nsmap"
#include "soapDeviceBindingService.h"
#include "soapMediaBindingService.h"
#include "soapPTZBindingService.h"





static const char *help_str =
        " ===============  Help  ===============\n"
        " Daemon name:  " DAEMON_NAME          "\n"
        " Daemon  ver:  " DAEMON_VERSION_STR   "\n"
#ifdef  DEBUG
        " Build  mode:  debug\n"
#else
        " Build  mode:  release\n"
#endif
        " Build  date:  " __DATE__ "\n"
        " Build  time:  " __TIME__ "\n\n"
        "Options:                      description:\n\n"
        "       --no_chdir             Don't change the directory to '/'\n"
        "       --no_fork              Don't do fork\n"
        "       --no_close             Don't close standart IO files\n"
        "       --pid_file     [value] Set pid file name\n"
        "       --log_file     [value] Set log file name\n\n"
        "       --port         [value] Set socket port for Services   (default = 1000)\n"
        "       --user         [value] Set user name for Services     (default = admin)\n"
        "       --password     [value] Set user password for Services (default = admin)\n"
        "       --model        [value] Set model device for Services  (default = Model)\n"
        "       --scope        [value] Set scope for Services         (default don't set)\n"
        "       --ifs          [value] Set Net interfaces for work    (default don't set)\n"
        "       --hardware_id  [value] Set Hardware ID of device      (default = HardwareID)\n"
        "       --serial_num   [value] Set Serial number of device    (default = SerialNumber)\n"
        "       --firmware_ver [value] Set firmware version of device (default = FirmwareVersion)\n"
        "       --manufacturer [value] Set manufacturer for Services  (default = Manufacturer)\n\n"
        "       --name         [value] Set Name for Profile Media Services\n"
        "       --width        [value] Set Width for Profile Media Services\n"
        "       --height       [value] Set Height for Profile Media Services\n"
        "       --url          [value] Set URL (or template URL) for Profile Media Services\n"
        "       --snapurl      [value] Set URL (or template URL) for Snapshot\n"
        "                              in template mode %s will be changed to IP of interface (see opt ifs)\n"
        "       --type         [value] Set Type for Profile Media Services (JPEG|MPEG4|H264)\n"
        "                              It is also a sign of the end of the profile parameters\n\n"
        "       --ptz                  Enable PTZ support\n"
        "       --move_left    [value] Set process to call for PTZ pan left movement\n"
        "       --move_right   [value] Set process to call for PTZ pan right movement\n"
        "       --move_up      [value] Set process to call for PTZ tilt up movement\n"
        "       --move_down    [value] Set process to call for PTZ tilt down movement\n"
        "       --move_stop    [value] Set process to call for PTZ stop movement\n"
        "       --move_preset  [value] Set process to call for PTZ goto preset movement\n"
        "  -v,  --version              Display daemon version\n"
        "  -h,  --help                 Display this help\n\n";




// indexes for long_opt function
namespace LongOpts
{
    enum
    {
        version = 'v',
        help    = 'h',

        //daemon options
        no_chdir = 1,
        no_fork,
        no_close,
        pid_file,
        log_file,

        //ONVIF Service options (context)
        port,
        user,
        password,
        manufacturer,
        model,
        firmware_ver,
        serial_num,
        hardware_id,
        scope,
        ifs,

        //Media Profile for ONVIF Media Service
        name,
        width,
        height,
        url,
        snapurl,
        type,

        //PTZ Profile for ONVIF PTZ Service
        ptz,
        move_left,
        move_right,
        move_up,
        move_down,
        move_stop,
        move_preset
    };
}



static const char *short_opts = "hv";


static const struct option long_opts[] =
{
    { "version",      no_argument,       NULL, LongOpts::version       },
    { "help",         no_argument,       NULL, LongOpts::help          },

    //daemon options
    { "no_chdir",     no_argument,       NULL, LongOpts::no_chdir      },
    { "no_fork",      no_argument,       NULL, LongOpts::no_fork       },
    { "no_close",     no_argument,       NULL, LongOpts::no_close      },
    { "pid_file",     required_argument, NULL, LongOpts::pid_file      },
    { "log_file",     required_argument, NULL, LongOpts::log_file      },

    //ONVIF Service options (context)
    { "port",         required_argument, NULL, LongOpts::port          },
    { "user",         required_argument, NULL, LongOpts::user          },
    { "password",     required_argument, NULL, LongOpts::password      },
    { "manufacturer", required_argument, NULL, LongOpts::manufacturer  },
    { "model",        required_argument, NULL, LongOpts::model         },
    { "firmware_ver", required_argument, NULL, LongOpts::firmware_ver  },
    { "serial_num",   required_argument, NULL, LongOpts::serial_num    },
    { "hardware_id",  required_argument, NULL, LongOpts::hardware_id   },
    { "scope",        required_argument, NULL, LongOpts::scope         },
    { "ifs",          required_argument, NULL, LongOpts::ifs           },

    //Media Profile for ONVIF Media Service
    { "name",          required_argument, NULL, LongOpts::name         },
    { "width",         required_argument, NULL, LongOpts::width        },
    { "height",        required_argument, NULL, LongOpts::height       },
    { "url",           required_argument, NULL, LongOpts::url          },
    { "snapurl",       required_argument, NULL, LongOpts::snapurl      },
    { "type",          required_argument, NULL, LongOpts::type         },

    //PTZ Profile for ONVIF PTZ Service
    { "ptz",           no_argument,       NULL, LongOpts::ptz          },
    { "move_left",     required_argument, NULL, LongOpts::move_left    },
    { "move_right",    required_argument, NULL, LongOpts::move_right   },
    { "move_up",       required_argument, NULL, LongOpts::move_up      },
    { "move_down",     required_argument, NULL, LongOpts::move_down    },
    { "move_stop",     required_argument, NULL, LongOpts::move_stop    },
    { "move_preset",   required_argument, NULL, LongOpts::move_preset  },

    { NULL,           no_argument,       NULL,  0                      }
};





#define FOREACH_SERVICE(APPLY, soap)                    \
        APPLY(DeviceBindingService, soap)               \
        APPLY(MediaBindingService, soap)                \
        APPLY(PTZBindingService, soap)                  \


/*
 * If you need support for other services,
 * add the desired option to the macro FOREACH_SERVICE.
 *
 * Note: Do not forget to add the gsoap binding class for the service,
 * and the implementation methods for it, like for DeviceBindingService



        APPLY(ImagingBindingService, soap)               \
        APPLY(PTZBindingService, soap)                   \
        APPLY(RecordingBindingService, soap)             \
        APPLY(ReplayBindingService, soap)                \
        APPLY(SearchBindingService, soap)                \
        APPLY(ReceiverBindingService, soap)              \
        APPLY(DisplayBindingService, soap)               \
        APPLY(EventBindingService, soap)                 \
        APPLY(PullPointSubscriptionBindingService, soap) \
        APPLY(NotificationProducerBindingService, soap)  \
        APPLY(SubscriptionManagerBindingService, soap)   \
*/


#define DECLARE_SERVICE(service, soap) service service ## _inst(soap);

#define DISPATCH_SERVICE(service, soap)                                  \
                else if (service ## _inst.dispatch() != SOAP_NO_METHOD) {\
                    soap_send_fault(soap);                               \
                    soap_stream_fault(soap, std::cerr);                  \
                }




static struct soap *soap;

ServiceContext service_ctx;





void daemon_exit_handler(int sig)
{
    //Here we release resources

    UNUSED(sig);
    soap_destroy(soap); // delete managed C++ objects
    soap_end(soap);     // delete managed memory
    soap_free(soap);    // free the context


    unlink(daemon_info.pid_file);


    exit(EXIT_SUCCESS); // good job (we interrupted (finished) main loop)
}



void init_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = daemon_exit_handler;
    if( sigaction(SIGTERM, &sa, NULL) != 0 )
        daemon_error_exit("Can't set daemon_exit_handler: %m\n");


    signal(SIGCHLD, SIG_IGN); // ignore child
    signal(SIGTSTP, SIG_IGN); // ignore tty signals
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
}



void processing_cmd(int argc, char *argv[])
{
    int opt;

    StreamProfile  profile;


    while( (opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1 )
    {
        switch( opt )
        {

            case LongOpts::help:
                        puts(help_str);
                        exit_if_not_daemonized(EXIT_SUCCESS);
                        break;

            case LongOpts::version:
                        puts(DAEMON_NAME "  version  " DAEMON_VERSION_STR "\n");
                        exit_if_not_daemonized(EXIT_SUCCESS);
                        break;


                 //daemon options
            case LongOpts::no_chdir:
                        daemon_info.no_chdir = 1;
                        break;

            case LongOpts::no_fork:
                        daemon_info.no_fork = 1;
                        break;

            case LongOpts::no_close:
                        daemon_info.no_close_stdio = 1;
                        break;

            case LongOpts::pid_file:
                        daemon_info.pid_file = optarg;
                        break;

            case LongOpts::log_file:
                        daemon_info.log_file = optarg;
                        break;


            //ONVIF Service options (context)
            case LongOpts::port:
                        service_ctx.port = atoi(optarg);
                        break;

            case LongOpts::user:
                        service_ctx.user = optarg;
                        break;

            case LongOpts::password:
                        service_ctx.password = optarg;
                        break;

            case LongOpts::manufacturer:
                        service_ctx.manufacturer = optarg;
                        break;

            case LongOpts::model:
                        service_ctx.model = optarg;
                        break;

            case LongOpts::firmware_ver:
                        service_ctx.firmware_version = optarg;
                        break;

            case LongOpts::serial_num:
                        service_ctx.serial_number = optarg;
                        break;

            case LongOpts::hardware_id:
                        service_ctx.hardware_id = optarg;
                        break;

            case LongOpts::scope:
                        service_ctx.scopes.push_back(optarg);
                        break;

            case LongOpts::ifs:
                        service_ctx.eth_ifs.push_back(Eth_Dev_Param());

                        if( service_ctx.eth_ifs.back().open(optarg) != 0 )
                            daemon_error_exit("Can't open ethernet interface: %s - %m\n", optarg);

                        break;


            //Media Profile for ONVIF Media Service
            case LongOpts::name:
                        if( !profile.set_name(optarg) )
                            daemon_error_exit("Can't set name for Profile: %s\n", profile.get_cstr_err());

                        break;


            case LongOpts::width:
                        if( !profile.set_width(optarg) )
                            daemon_error_exit("Can't set width for Profile: %s\n", profile.get_cstr_err());

                        break;


            case LongOpts::height:
                        if( !profile.set_height(optarg) )
                            daemon_error_exit("Can't set height for Profile: %s\n", profile.get_cstr_err());

                        break;


            case LongOpts::url:
                        if( !profile.set_url(optarg) )
                            daemon_error_exit("Can't set URL for Profile: %s\n", profile.get_cstr_err());

                        break;


            case LongOpts::snapurl:
                        if( !profile.set_snapurl(optarg) )
                            daemon_error_exit("Can't set URL for Snapshot: %s\n", profile.get_cstr_err());

                        break;


            case LongOpts::type:
                        if( !profile.set_type(optarg) )
                            daemon_error_exit("Can't set type for Profile: %s\n", profile.get_cstr_err());

                        if( !service_ctx.add_profile(profile) )
                            daemon_error_exit("Can't add Profile: %s\n", service_ctx.get_cstr_err());

                        profile.clear(); //now we can add new profile (just uses one variable)

                        break;


            //PTZ Profile for ONVIF PTZ Service
            case LongOpts::ptz:
                        service_ctx.get_ptz_node()->enable = true;
                        break;


            case LongOpts::move_left:
                        if( !service_ctx.get_ptz_node()->set_move_left(optarg) )
                            daemon_error_exit("Can't set process for pan left movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            case LongOpts::move_right:
                        if( !service_ctx.get_ptz_node()->set_move_right(optarg) )
                            daemon_error_exit("Can't set process for pan right movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            case LongOpts::move_up:
                        if( !service_ctx.get_ptz_node()->set_move_up(optarg) )
                            daemon_error_exit("Can't set process for tilt up movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            case LongOpts::move_down:
                        if( !service_ctx.get_ptz_node()->set_move_down(optarg) )
                            daemon_error_exit("Can't set process for tilt down movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            case LongOpts::move_stop:
                        if( !service_ctx.get_ptz_node()->set_move_stop(optarg) )
                            daemon_error_exit("Can't set process for stop movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            case LongOpts::move_preset:
                        if( !service_ctx.get_ptz_node()->set_move_preset(optarg) )
                            daemon_error_exit("Can't set process for goto preset movement: %s\n", service_ctx.get_ptz_node()->get_cstr_err());

                        break;


            default:
                        puts("for more detail see help\n\n");
                        exit_if_not_daemonized(EXIT_FAILURE);
                        break;
        }
    }
}



void check_service_ctx(void)
{
    if(service_ctx.eth_ifs.empty())
        daemon_error_exit("Error: not set no one ehternet interface more details see opt --ifs\n");


    if(service_ctx.scopes.empty())
        daemon_error_exit("Error: not set scopes more details see opt --scope\n");


    if(service_ctx.get_profiles().empty())
        daemon_error_exit("Error: not set no one profile more details see --help\n");
}



void init_gsoap(void)
{
    soap = soap_new();

    if(!soap)
        daemon_error_exit("Can't get mem for SOAP\n");


    soap->bind_flags = SO_REUSEADDR;

    if( !soap_valid_socket(soap_bind(soap, NULL, service_ctx.port, 10)) )
    {
        soap_stream_fault(soap, std::cerr);
        exit(EXIT_FAILURE);
    }

    soap->send_timeout = 3; // timeout in sec
    soap->recv_timeout = 3; // timeout in sec


    //save pointer of service_ctx in soap
    soap->user = (void*)&service_ctx;
}



void init(void *data)
{
    UNUSED(data);
    init_signals();
    check_service_ctx();
    init_gsoap();
}



int main(int argc, char *argv[])
{
    processing_cmd(argc, argv);
    daemonize2(init, NULL);

    FOREACH_SERVICE(DECLARE_SERVICE, soap)

    while( true )
    {
        // wait new client
        if( !soap_valid_socket(soap_accept(soap)) )
        {
            soap_stream_fault(soap, std::cerr);
            return EXIT_FAILURE;
        }


        // process service
        if( soap_begin_serve(soap) )
        {
            soap_stream_fault(soap, std::cerr);
        }
        FOREACH_SERVICE(DISPATCH_SERVICE, soap)
        else
        {
            DEBUG_MSG("Unknown service\n");
        }

        soap_destroy(soap); // delete managed C++ objects
        soap_end(soap);     // delete managed memory
    }


    return EXIT_FAILURE; // Error, normal exit from the main loop only through the signal handler.
}
