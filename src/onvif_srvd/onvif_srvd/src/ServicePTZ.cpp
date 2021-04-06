/*
 --------------------------------------------------------------------------
 ServicePTZ.cpp

 Implementation of functions (methods) for the service:
 ONVIF ptz.wsdl server side
-----------------------------------------------------------------------------
*/


#include <sstream>
#include "soapPTZBindingService.h"
#include "ServiceContext.h"
#include "smacros.h"





int PTZBindingService::GetServiceCapabilities(_tptz__GetServiceCapabilities *tptz__GetServiceCapabilities, _tptz__GetServiceCapabilitiesResponse &tptz__GetServiceCapabilitiesResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetServiceCapabilities, "PTZ");
}



int PTZBindingService::GetConfigurations(_tptz__GetConfigurations *tptz__GetConfigurations, _tptz__GetConfigurationsResponse &tptz__GetConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetConfigurations, "PTZ");
}



int GetPTZPreset(struct soap *soap, tt__PTZPreset* ptzp, int number)
{
    std::stringstream stream;

    stream << number;
    ptzp->token = soap_new_std__string(soap);
    *ptzp->token = stream.str();
    ptzp->Name = soap_new_std__string(soap);
    *ptzp->Name  = stream.str();

    ptzp->PTZPosition             = soap_new_tt__PTZVector(soap);
    ptzp->PTZPosition->PanTilt    = soap_new_req_tt__Vector2D(soap, 0.0f, 0.0f);
    ptzp->PTZPosition->Zoom       = soap_new_req_tt__Vector1D(soap, 1.0f);

    return SOAP_OK;
}



int PTZBindingService::GetPresets(_tptz__GetPresets *tptz__GetPresets, _tptz__GetPresetsResponse &tptz__GetPresetsResponse)
{
    UNUSED(tptz__GetPresets);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);


    soap_default_std__vectorTemplateOfPointerTott__PTZPreset(soap, &tptz__GetPresetsResponse._tptz__GetPresetsResponse::Preset);
    for (int i = 0; i < 8; i++) {
        tt__PTZPreset* ptzp;
        ptzp = soap_new_tt__PTZPreset(soap);
        tptz__GetPresetsResponse.Preset.push_back(ptzp);
        GetPTZPreset(this->soap, ptzp, i);
    }

    return SOAP_OK;
}



int PTZBindingService::SetPreset(_tptz__SetPreset *tptz__SetPreset, _tptz__SetPresetResponse &tptz__SetPresetResponse)
{
    SOAP_EMPTY_HANDLER(tptz__SetPreset, "PTZ");
}



int PTZBindingService::RemovePreset(_tptz__RemovePreset *tptz__RemovePreset, _tptz__RemovePresetResponse &tptz__RemovePresetResponse)
{
    SOAP_EMPTY_HANDLER(tptz__RemovePreset, "PTZ");
}



int PTZBindingService::GotoPreset(_tptz__GotoPreset *tptz__GotoPreset, _tptz__GotoPresetResponse &tptz__GotoPresetResponse)
{
    UNUSED(tptz__GotoPresetResponse);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);


    std::string preset_cmd;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    if (tptz__GotoPreset == NULL) {
        return SOAP_OK;
    }
    if (tptz__GotoPreset->ProfileToken.c_str() == NULL) {
        return SOAP_OK;
    }
    if (tptz__GotoPreset->PresetToken.c_str() == NULL) {
        return SOAP_OK;
    }

    if (!ctx->get_ptz_node()->get_move_preset().empty()) {
        preset_cmd = ctx->get_ptz_node()->get_move_preset().c_str();
    } else {
        return SOAP_OK;
    }

    std::string template_str_t("%t");

    auto it_t = preset_cmd.find(template_str_t, 0);

    if( it_t != std::string::npos ) {
        preset_cmd.replace(it_t, template_str_t.size(), tptz__GotoPreset->PresetToken.c_str());
    }

    system(preset_cmd.c_str());

    return SOAP_OK;
}



int PTZBindingService::GetStatus(_tptz__GetStatus *tptz__GetStatus, _tptz__GetStatusResponse &tptz__GetStatusResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetStatus, "PTZ");
}



int PTZBindingService::GetConfiguration(_tptz__GetConfiguration *tptz__GetConfiguration, _tptz__GetConfigurationResponse &tptz__GetConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetConfiguration, "PTZ");
}



int GetPTZNode(struct soap *soap, tt__PTZNode* ptzn)
{
    ptzn->token = "PTZNodeToken";
    ptzn->Name  = soap_new_std__string(soap);
    *ptzn->Name = "PTZ";

    ptzn->SupportedPTZSpaces = soap_new_tt__PTZSpaces(soap);;
    soap_default_std__vectorTemplateOfPointerTott__Space2DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::RelativePanTiltTranslationSpace);
    soap_default_std__vectorTemplateOfPointerTott__Space1DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::RelativeZoomTranslationSpace);
    soap_default_std__vectorTemplateOfPointerTott__Space2DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::ContinuousPanTiltVelocitySpace);
    soap_default_std__vectorTemplateOfPointerTott__Space1DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::ContinuousZoomVelocitySpace);
    soap_default_std__vectorTemplateOfPointerTott__Space1DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::PanTiltSpeedSpace);
    soap_default_std__vectorTemplateOfPointerTott__Space1DDescription(soap, &ptzn->SupportedPTZSpaces->tt__PTZSpaces::ZoomSpeedSpace);


    auto ptzs1 = soap_new_tt__Space2DDescription(soap);
    ptzn->SupportedPTZSpaces->RelativePanTiltTranslationSpace.push_back(ptzs1);

    auto ptzs2 = soap_new_tt__Space1DDescription(soap);
    ptzn->SupportedPTZSpaces->RelativeZoomTranslationSpace.push_back(ptzs2);

    auto ptzs3 = soap_new_tt__Space2DDescription(soap);
    ptzn->SupportedPTZSpaces->ContinuousPanTiltVelocitySpace.push_back(ptzs3);

    auto ptzs4 = soap_new_tt__Space1DDescription(soap);
    ptzn->SupportedPTZSpaces->ContinuousZoomVelocitySpace.push_back(ptzs4);

    auto ptzs5 = soap_new_tt__Space1DDescription(soap);
    ptzn->SupportedPTZSpaces->PanTiltSpeedSpace.push_back(ptzs5);

    auto ptzs6 = soap_new_tt__Space1DDescription(soap);
    ptzn->SupportedPTZSpaces->ZoomSpeedSpace.push_back(ptzs6);

    ptzs1->URI         = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
    ptzs1->XRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);
    ptzs1->YRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);

    ptzs2->URI         = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace";
    ptzs2->XRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);

    ptzs3->URI         = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";
    ptzs3->XRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);
    ptzs3->YRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);

    ptzs4->URI         = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace";
    ptzs4->XRange      = soap_new_req_tt__FloatRange(soap, -1.0f, 1.0f);

    ptzs5->URI         = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace";
    ptzs5->XRange      = soap_new_req_tt__FloatRange(soap, 0.0f, 1.0f);

    ptzs6->URI         = "http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace";
    ptzs6->XRange      = soap_new_req_tt__FloatRange(soap, 0.0f, 1.0f);


    ptzn->MaximumNumberOfPresets = 8;
    ptzn->HomeSupported          = true;
    ptzn->FixedHomePosition      = (bool *)soap_malloc(soap, sizeof(bool));
    soap_s2bool(soap, "true", ptzn->FixedHomePosition);

    return SOAP_OK;
}



int PTZBindingService::GetNodes(_tptz__GetNodes *tptz__GetNodes, _tptz__GetNodesResponse &tptz__GetNodesResponse)
{
    UNUSED(tptz__GetNodes);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);


    soap_default_std__vectorTemplateOfPointerTott__PTZNode(soap, &tptz__GetNodesResponse._tptz__GetNodesResponse::PTZNode);
    tt__PTZNode* ptzn;
    ptzn = soap_new_tt__PTZNode(soap);
    tptz__GetNodesResponse.PTZNode.push_back(ptzn);
    GetPTZNode(this->soap, ptzn);

    return SOAP_OK;
}



int PTZBindingService::GetNode(_tptz__GetNode *tptz__GetNode, _tptz__GetNodeResponse &tptz__GetNodeResponse)
{
    UNUSED(tptz__GetNode);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);

    tptz__GetNodeResponse.PTZNode = soap_new_tt__PTZNode(this->soap);
    GetPTZNode(this->soap, tptz__GetNodeResponse.PTZNode);

    return SOAP_OK;
}



int PTZBindingService::SetConfiguration(_tptz__SetConfiguration *tptz__SetConfiguration, _tptz__SetConfigurationResponse &tptz__SetConfigurationResponse)
{
    SOAP_EMPTY_HANDLER(tptz__SetConfiguration, "PTZ");
}



int PTZBindingService::GetConfigurationOptions(_tptz__GetConfigurationOptions *tptz__GetConfigurationOptions, _tptz__GetConfigurationOptionsResponse &tptz__GetConfigurationOptionsResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetConfigurationOptions, "PTZ");
}



int PTZBindingService::GotoHomePosition(_tptz__GotoHomePosition *tptz__GotoHomePosition, _tptz__GotoHomePositionResponse &tptz__GotoHomePositionResponse)
{
    UNUSED(tptz__GotoHomePosition);
    UNUSED(tptz__GotoHomePositionResponse);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);

    std::string preset_cmd;
	
    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    if (tptz__GotoHomePosition == NULL) {
        return SOAP_OK;
    }
    if (tptz__GotoHomePosition->ProfileToken.c_str() == NULL) {
        return SOAP_OK;
    }

    if (!ctx->get_ptz_node()->get_move_preset().empty()) {
        preset_cmd = ctx->get_ptz_node()->get_move_preset().c_str();
    } else {
        return SOAP_OK;
    }

    std::string template_str_t("%t");

    auto it_t = preset_cmd.find(template_str_t, 0);

    if( it_t != std::string::npos ) {
        preset_cmd.replace(it_t, template_str_t.size(), "1");
    }

    system(preset_cmd.c_str());

    return SOAP_OK;
}

int PTZBindingService::SetHomePosition(_tptz__SetHomePosition *tptz__SetHomePosition, _tptz__SetHomePositionResponse &tptz__SetHomePositionResponse)
{
    SOAP_EMPTY_HANDLER(tptz__SetHomePosition, "PTZ");
}



int PTZBindingService::ContinuousMove(_tptz__ContinuousMove *tptz__ContinuousMove, _tptz__ContinuousMoveResponse &tptz__ContinuousMoveResponse)
{
    UNUSED(tptz__ContinuousMoveResponse);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);


    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    if (tptz__ContinuousMove == NULL) {
        return SOAP_OK;
    }
    if (tptz__ContinuousMove->Velocity == NULL) {
        return SOAP_OK;
    }
    if (tptz__ContinuousMove->Velocity->PanTilt == NULL) {
        return SOAP_OK;
    }

    if (tptz__ContinuousMove->Velocity->PanTilt->x > 0) {
        system(ctx->get_ptz_node()->get_move_right().c_str());
    } else if (tptz__ContinuousMove->Velocity->PanTilt->x < 0) {
        system(ctx->get_ptz_node()->get_move_left().c_str());
    }
    if (tptz__ContinuousMove->Velocity->PanTilt->y > 0) {
        system(ctx->get_ptz_node()->get_move_up().c_str());
    } else if (tptz__ContinuousMove->Velocity->PanTilt->y < 0) {
        system(ctx->get_ptz_node()->get_move_down().c_str());
    }

    return SOAP_OK;
}



int PTZBindingService::RelativeMove(_tptz__RelativeMove *tptz__RelativeMove, _tptz__RelativeMoveResponse &tptz__RelativeMoveResponse)
{
    UNUSED(tptz__RelativeMoveResponse);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);


    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    if (tptz__RelativeMove == NULL) {
        return SOAP_OK;
    }
    if (tptz__RelativeMove->Translation == NULL) {
        return SOAP_OK;
    }
    if (tptz__RelativeMove->Translation->PanTilt == NULL) {
        return SOAP_OK;
    }

    if (tptz__RelativeMove->Translation->PanTilt->x > 0) {
        system(ctx->get_ptz_node()->get_move_right().c_str());
        usleep(300000);
        system(ctx->get_ptz_node()->get_move_stop().c_str());
    } else if (tptz__RelativeMove->Translation->PanTilt->x < 0) {
        system(ctx->get_ptz_node()->get_move_left().c_str());
        usleep(300000);
        system(ctx->get_ptz_node()->get_move_stop().c_str());
    }
    if (tptz__RelativeMove->Translation->PanTilt->y > 0) {
        system(ctx->get_ptz_node()->get_move_up().c_str());
        usleep(300000);
        system(ctx->get_ptz_node()->get_move_stop().c_str());
    } else if (tptz__RelativeMove->Translation->PanTilt->y < 0) {
        system(ctx->get_ptz_node()->get_move_down().c_str());
        usleep(300000);
        system(ctx->get_ptz_node()->get_move_stop().c_str());
    }

    return SOAP_OK;
}



int PTZBindingService::SendAuxiliaryCommand(_tptz__SendAuxiliaryCommand *tptz__SendAuxiliaryCommand, _tptz__SendAuxiliaryCommandResponse &tptz__SendAuxiliaryCommandResponse)
{
    SOAP_EMPTY_HANDLER(tptz__SendAuxiliaryCommand, "PTZ");
}



int PTZBindingService::AbsoluteMove(_tptz__AbsoluteMove *tptz__AbsoluteMove, _tptz__AbsoluteMoveResponse &tptz__AbsoluteMoveResponse)
{
    SOAP_EMPTY_HANDLER(tptz__AbsoluteMove, "PTZ");
}



int PTZBindingService::Stop(_tptz__Stop *tptz__Stop, _tptz__StopResponse &tptz__StopResponse)
{
    UNUSED(tptz__Stop);
    UNUSED(tptz__StopResponse);
    DEBUG_MSG("PTZ: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    system(ctx->get_ptz_node()->get_move_stop().c_str());

    return SOAP_OK;
}



int PTZBindingService::GetPresetTours(_tptz__GetPresetTours *tptz__GetPresetTours, _tptz__GetPresetToursResponse &tptz__GetPresetToursResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetPresetTours, "PTZ");
}



int PTZBindingService::GetPresetTour(_tptz__GetPresetTour *tptz__GetPresetTour, _tptz__GetPresetTourResponse &tptz__GetPresetTourResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetPresetTour, "PTZ");
}



int PTZBindingService::GetPresetTourOptions(_tptz__GetPresetTourOptions *tptz__GetPresetTourOptions, _tptz__GetPresetTourOptionsResponse &tptz__GetPresetTourOptionsResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetPresetTourOptions, "PTZ");
}



int PTZBindingService::CreatePresetTour(_tptz__CreatePresetTour *tptz__CreatePresetTour, _tptz__CreatePresetTourResponse &tptz__CreatePresetTourResponse)
{
    SOAP_EMPTY_HANDLER(tptz__CreatePresetTour, "PTZ");
}



int PTZBindingService::ModifyPresetTour(_tptz__ModifyPresetTour *tptz__ModifyPresetTour, _tptz__ModifyPresetTourResponse &tptz__ModifyPresetTourResponse)
{
    SOAP_EMPTY_HANDLER(tptz__ModifyPresetTour, "PTZ");
}



int PTZBindingService::OperatePresetTour(_tptz__OperatePresetTour *tptz__OperatePresetTour, _tptz__OperatePresetTourResponse &tptz__OperatePresetTourResponse)
{
    SOAP_EMPTY_HANDLER(tptz__OperatePresetTour, "PTZ");
}



int PTZBindingService::RemovePresetTour(_tptz__RemovePresetTour *tptz__RemovePresetTour, _tptz__RemovePresetTourResponse &tptz__RemovePresetTourResponse)
{
    SOAP_EMPTY_HANDLER(tptz__RemovePresetTour, "PTZ");
}



int PTZBindingService::GetCompatibleConfigurations(_tptz__GetCompatibleConfigurations *tptz__GetCompatibleConfigurations, _tptz__GetCompatibleConfigurationsResponse &tptz__GetCompatibleConfigurationsResponse)
{
    SOAP_EMPTY_HANDLER(tptz__GetCompatibleConfigurations, "PTZ");
}
