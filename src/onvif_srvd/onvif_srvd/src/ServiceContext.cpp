#include <arpa/inet.h>

#include <sstream>

#include "ServiceContext.h"
#include "stools.h"





ServiceContext::ServiceContext():
    port     ( 1000    ),
    user     ( "admin" ),
    password ( "admin" ),


    //Device Information
    manufacturer     ( "Manufacturer"   ),
    model            ( "Model"          ),
    firmware_version ( "FirmwareVersion"),
    serial_number    ( "SerialNumber"   ),
    hardware_id      ( "HardwareId"     )
{
}



std::string ServiceContext::getServerIpFromClientIp(uint32_t client_ip) const
{
    char server_ip[INET_ADDRSTRLEN];


    if (eth_ifs.size() == 1)
    {
        eth_ifs[0].get_ip(server_ip);
        return server_ip;
    }

    for(size_t i = 0; i < eth_ifs.size(); ++i)
    {
        uint32_t if_ip, if_mask;
        eth_ifs[i].get_ip(&if_ip);
        eth_ifs[i].get_mask(&if_mask);

        if( (if_ip & if_mask) == (client_ip & if_mask) )
        {
            eth_ifs[i].get_ip(server_ip);
            return server_ip;
        }
    }


    return "127.0.0.1";  //localhost
}



std::string ServiceContext::getXAddr(soap *soap) const
{
    std::ostringstream os;

    os << "http://" << getServerIpFromClientIp(htonl(soap->ip)) << ":" << port;

    return os.str();
}



bool ServiceContext::add_profile(const StreamProfile &profile)
{
    if( !profile.is_valid() )
    {
        str_err = "profile has unset parameters";
        return false;
    }


    if( profiles.find(profile.get_name()) != profiles.end() )
    {
        str_err = "profile: " + profile.get_name() +  " already exist";
        return false;
    }


    profiles[profile.get_name()] = profile;
    return true;
}



std::string ServiceContext::get_stream_uri(const std::string &profile_url, uint32_t client_ip) const
{
    std::string uri(profile_url);
    std::string template_str("%s");


    auto it = uri.find(template_str, 0);

    if( it != std::string::npos )
        uri.replace(it, template_str.size(), getServerIpFromClientIp(client_ip));


    return uri;
}



std::string ServiceContext::get_snapshot_uri(const std::string &profile_url, uint32_t client_ip) const
{
    std::string uri(profile_url);
    std::string template_str("%s");


    auto it = uri.find(template_str, 0);

    if( it != std::string::npos )
        uri.replace(it, template_str.size(), getServerIpFromClientIp(client_ip));


    return uri;
}



tds__DeviceServiceCapabilities *ServiceContext::getDeviceServiceCapabilities(soap *soap)
{
    tds__DeviceServiceCapabilities *capabilities = soap_new_tds__DeviceServiceCapabilities(soap);

    capabilities->Network = soap_new_tds__NetworkCapabilities(soap);

    capabilities->Network->IPFilter            = soap_new_ptr(soap, false);
    capabilities->Network->ZeroConfiguration   = soap_new_ptr(soap, false);
    capabilities->Network->IPVersion6          = soap_new_ptr(soap, false);
    capabilities->Network->DynDNS              = soap_new_ptr(soap, false);
    capabilities->Network->Dot11Configuration  = soap_new_ptr(soap, false);
    capabilities->Network->Dot1XConfigurations = soap_new_ptr(soap, 0);
    capabilities->Network->HostnameFromDHCP    = soap_new_ptr(soap, false);
    capabilities->Network->NTP                 = soap_new_ptr(soap, 0);
    capabilities->Network->DHCPv6              = soap_new_ptr(soap, false);


    capabilities->Security = soap_new_tds__SecurityCapabilities(soap);

    capabilities->Security->TLS1_x002e0          = soap_new_ptr(soap, false);
    capabilities->Security->TLS1_x002e1          = soap_new_ptr(soap, false);
    capabilities->Security->TLS1_x002e2          = soap_new_ptr(soap, false);
    capabilities->Security->OnboardKeyGeneration = soap_new_ptr(soap, false);
    capabilities->Security->AccessPolicyConfig   = soap_new_ptr(soap, false);
    capabilities->Security->DefaultAccessPolicy  = soap_new_ptr(soap, false);
    capabilities->Security->Dot1X                = soap_new_ptr(soap, false);
    capabilities->Security->RemoteUserHandling   = soap_new_ptr(soap, false);
    capabilities->Security->X_x002e509Token      = soap_new_ptr(soap, false);
    capabilities->Security->SAMLToken            = soap_new_ptr(soap, false);
    capabilities->Security->KerberosToken        = soap_new_ptr(soap, false);
    capabilities->Security->UsernameToken        = soap_new_ptr(soap, false);
    capabilities->Security->HttpDigest           = soap_new_ptr(soap, false);
    capabilities->Security->RELToken             = soap_new_ptr(soap, false);
    capabilities->Security->MaxUsers             = soap_new_ptr(soap, 0);
    capabilities->Security->MaxUserNameLength    = soap_new_ptr(soap, 0);
    capabilities->Security->MaxPasswordLength    = soap_new_ptr(soap, 0);


    capabilities->System = soap_new_tds__SystemCapabilities(soap);

    capabilities->System->DiscoveryResolve       = soap_new_ptr(soap, true);
    capabilities->System->DiscoveryBye           = soap_new_ptr(soap, true);
    capabilities->System->RemoteDiscovery        = soap_new_ptr(soap, true);
    capabilities->System->SystemBackup           = soap_new_ptr(soap, false);
    capabilities->System->SystemLogging          = soap_new_ptr(soap, false);
    capabilities->System->FirmwareUpgrade        = soap_new_ptr(soap, false);
    capabilities->System->HttpFirmwareUpgrade    = soap_new_ptr(soap, false);
    capabilities->System->HttpSystemBackup       = soap_new_ptr(soap, false);
    capabilities->System->HttpSystemLogging      = soap_new_ptr(soap, false);
    capabilities->System->HttpSupportInformation = soap_new_ptr(soap, false);
    capabilities->System->StorageConfiguration   = soap_new_ptr(soap, false);


    return capabilities;
}



trt__Capabilities *ServiceContext::getMediaServiceCapabilities(soap *soap)
{
    trt__Capabilities *capabilities = soap_new_trt__Capabilities(soap);

    auto profiles = this->get_profiles();
    for( auto it = profiles.cbegin(); it != profiles.cend(); ++it ) {
        if (( !it->second.get_snapurl().empty() ) && ( capabilities->SnapshotUri == NULL )) {
            capabilities->SnapshotUri = soap_new_ptr(soap, true);
        }
    }

    capabilities->ProfileCapabilities = soap_new_trt__ProfileCapabilities(soap);
    capabilities->ProfileCapabilities->MaximumNumberOfProfiles = soap_new_ptr(soap, 1);

    capabilities->StreamingCapabilities = soap_new_trt__StreamingCapabilities(soap);
    capabilities->StreamingCapabilities->RTPMulticast = soap_new_ptr(soap, false);
    capabilities->StreamingCapabilities->RTP_USCORETCP = soap_new_ptr(soap, false);
    capabilities->StreamingCapabilities->RTP_USCORERTSP_USCORETCP = soap_new_ptr(soap, true);


    return capabilities;
}



tptz__Capabilities *ServiceContext::getPTZServiceCapabilities(soap *soap)
{
    tptz__Capabilities *capabilities = soap_new_tptz__Capabilities(soap);

    return capabilities;
}




// ------------------------------- StreamProfile -------------------------------




tt__VideoSourceConfiguration* StreamProfile::get_video_src_cnf(struct soap *soap) const
{
    tt__VideoSourceConfiguration* src_cfg = soap_new_tt__VideoSourceConfiguration(soap);

    src_cfg->token       = name;
    src_cfg->SourceToken = name;
    src_cfg->Bounds      = soap_new_req_tt__IntRectangle(soap, 0, 0, width, height);

    return src_cfg;
}



tt__VideoEncoderConfiguration* StreamProfile::get_video_enc_cfg(struct soap *soap) const
{
    tt__VideoEncoderConfiguration* enc_cfg = soap_new_tt__VideoEncoderConfiguration(soap);

    enc_cfg->Name               = name;
    enc_cfg->token              = name;
    enc_cfg->Resolution         = soap_new_req_tt__VideoResolution(soap, width, height);
    enc_cfg->RateControl        = soap_new_req_tt__VideoRateControl(soap, 0, 0, 0);
    enc_cfg->Multicast          = soap_new_tt__MulticastConfiguration(soap);
    enc_cfg->Multicast->Address = soap_new_tt__IPAddress(soap);
    enc_cfg->Encoding           = static_cast<tt__VideoEncoding>(type);
//    enc_cfg->GuaranteedFrameRate= soap_new_ptr(soap, true);
    enc_cfg->H264               = soap_new_tt__H264Configuration(soap);
    enc_cfg->H264->GovLength    = 40;
    enc_cfg->H264->H264Profile  = tt__H264Profile__Main;

    return enc_cfg;
}



tt__PTZConfiguration* StreamProfile::get_ptz_cfg(struct soap *soap) const
{
    tt__PTZConfiguration* ptz_cfg = soap_new_tt__PTZConfiguration(soap);

    ptz_cfg->Name               = "PTZ";
    ptz_cfg->token              = "PTZToken";
    ptz_cfg->NodeToken          = "PTZNodeToken";

    ptz_cfg->DefaultAbsolutePantTiltPositionSpace    = soap_new_std__string(soap);
    *ptz_cfg->DefaultAbsolutePantTiltPositionSpace   = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
    ptz_cfg->DefaultAbsoluteZoomPositionSpace        = soap_new_std__string(soap);
    *ptz_cfg->DefaultAbsoluteZoomPositionSpace       = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
    ptz_cfg->DefaultRelativePanTiltTranslationSpace  = soap_new_std__string(soap);
    *ptz_cfg->DefaultRelativePanTiltTranslationSpace = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
    ptz_cfg->DefaultRelativeZoomTranslationSpace     = soap_new_std__string(soap);
    *ptz_cfg->DefaultRelativeZoomTranslationSpace    = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace";
    ptz_cfg->DefaultContinuousPanTiltVelocitySpace   = soap_new_std__string(soap);
    *ptz_cfg->DefaultContinuousPanTiltVelocitySpace  = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";
    ptz_cfg->DefaultContinuousZoomVelocitySpace      = soap_new_std__string(soap);
    *ptz_cfg->DefaultContinuousZoomVelocitySpace     = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace";

    ptz_cfg->DefaultPTZSpeed                   = soap_new_tt__PTZSpeed(soap);
    ptz_cfg->DefaultPTZSpeed->PanTilt          = soap_new_req_tt__Vector2D(soap, 0.1f, 0.1f);
    ptz_cfg->DefaultPTZSpeed->Zoom             = soap_new_req_tt__Vector1D(soap, 1.0f);

    ptz_cfg->DefaultPTZTimeout                 = soap_new_ptr(soap, (LONG64)1000);

    ptz_cfg->PanTiltLimits                     = soap_new_tt__PanTiltLimits(soap);
    ptz_cfg->PanTiltLimits->Range              = soap_new_tt__Space2DDescription(soap);
    ptz_cfg->PanTiltLimits->Range->URI         = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
    ptz_cfg->PanTiltLimits->Range->XRange      = soap_new_req_tt__FloatRange(soap, -INFINITY, INFINITY);
    ptz_cfg->PanTiltLimits->Range->YRange      = soap_new_req_tt__FloatRange(soap, -INFINITY, INFINITY);

    ptz_cfg->ZoomLimits                        = soap_new_tt__ZoomLimits(soap);
    ptz_cfg->ZoomLimits->Range                 = soap_new_tt__Space1DDescription(soap);
    ptz_cfg->ZoomLimits->Range->URI            = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
    ptz_cfg->ZoomLimits->Range->XRange         = soap_new_req_tt__FloatRange(soap, -INFINITY, INFINITY);

    return ptz_cfg;
}



tt__Profile* StreamProfile::get_profile(struct soap *soap) const
{
    ServiceContext* ctx = (ServiceContext*)soap->user;

    tt__Profile* profile = soap_new_tt__Profile(soap);

    profile->Name  = name;
    profile->token = name;
    profile->fixed = soap_new_ptr(soap, true);

    profile->VideoSourceConfiguration  = get_video_src_cnf(soap);
    profile->VideoEncoderConfiguration = get_video_enc_cfg(soap);
    if (ctx->get_ptz_node()->enable) {
        profile->PTZConfiguration = get_ptz_cfg(soap);
    }

    return profile;
}



tt__VideoSource* StreamProfile::get_video_src(soap *soap) const
{
    tt__VideoSource* video_src = soap_new_tt__VideoSource(soap);

    video_src->token      = name;
    video_src->Resolution = soap_new_req_tt__VideoResolution(soap, width, height);
    video_src->Imaging    = soap_new_tt__ImagingSettings(soap);

    return video_src;
}



bool StreamProfile::set_name(const char *new_val)
{
    if(!new_val)
    {
        str_err = "Name is empty";
        return false;
    }


    name = new_val;
    return true;
}



bool StreamProfile::set_width(const char *new_val)
{

    std::istringstream ss(new_val);
    int tmp_val;
    ss >> tmp_val;


    if( (tmp_val < 100) || (tmp_val >= 10000) )
    {
        str_err = "width is bad, correct range: 100-10000";
        return false;
    }


    width = tmp_val;
    return true;
}



bool StreamProfile::set_height(const char *new_val)
{
    std::istringstream ss(new_val);
    int tmp_val;
    ss >> tmp_val;


    if( (tmp_val < 100) || (tmp_val >= 10000) )
    {
        str_err = "height is bad, correct range: 100-10000";
        return false;
    }


    height = tmp_val;
    return true;
}



bool StreamProfile::set_url(const char *new_val)
{
    if(!new_val)
    {
        str_err = "URL is empty";
        return false;
    }


    url = new_val;
    return true;
}



bool StreamProfile::set_snapurl(const char *new_val)
{
    if(!new_val)
    {
        str_err = "URL is empty";
        return false;
    }


    snapurl = new_val;
    return true;
}



bool StreamProfile::set_type(const char *new_val)
{
    std::string new_type(new_val);


    if( new_type == "JPEG" )
        type = tt__VideoEncoding__JPEG;
    else if( new_type == "MPEG4" )
        type = tt__VideoEncoding__MPEG4;
    else if( new_type == "H264" )
        type = tt__VideoEncoding__H264;
    else
    {
        str_err = "type dont support";
        return false;
    }


    return true;
}



void StreamProfile::clear()
{
    name.clear();
    url.clear();
    snapurl.clear();

    width  = -1;
    height = -1;
    type   = -1;
}



bool StreamProfile::is_valid() const
{
    return ( !name.empty()  &&
             !url.empty()   &&
             (width  != -1) &&
             (height != -1) &&
             (type   != -1)
           );
}




// ------------------------------- PTZNode -------------------------------




void PTZNode::clear()
{
    enable = false;

    move_left.clear();
    move_right.clear();
    move_up.clear();
    move_down.clear();
    move_stop.clear();
    move_preset.clear();
}



bool PTZNode::set_str_value(const char* new_val, std::string& value)
{
    if(!new_val)
    {
        str_err = "Process is empty";
        return false;
    }


    value = new_val;
    return true;
}
