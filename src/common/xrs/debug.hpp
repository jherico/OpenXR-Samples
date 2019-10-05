#pragma once

#include <openxr/openxr.hpp>
#include "../logging.hpp"

namespace xrs {

namespace DebugUtilsEXT {

using MessageSeverityFlagBits = xr::DebugUtilsMessageSeverityFlagBitsEXT;
using MessageTypeFlagBits = xr::DebugUtilsMessageTypeFlagBitsEXT;
using MessageSeverityFlags = xr::DebugUtilsMessageSeverityFlagsEXT;
using MessageTypeFlags = xr::DebugUtilsMessageTypeFlagsEXT;
using CallbackData = xr::DebugUtilsMessengerCallbackDataEXT;
using Messenger = xr::DebugUtilsMessengerEXT;

// Raw C callback
static XrBool32 debugCallback(XrDebugUtilsMessageSeverityFlagsEXT sev_,
                              XrDebugUtilsMessageTypeFlagsEXT type_,
                              const XrDebugUtilsMessengerCallbackDataEXT* data_,
                              void* userData) {
    LOG_FORMATTED((logging::Level)sev_, "{}: message: {}", data_->functionName, data_->message);
    return XR_TRUE;
}

Messenger create(const xr::Instance& instance,
                 const MessageSeverityFlags& severityFlags = MessageSeverityFlagBits::AllBits,
                 const MessageTypeFlags& typeFlags = MessageTypeFlagBits::AllBits,
                 void* userData = nullptr) {
    return instance.createDebugUtilsMessengerEXT({ severityFlags, typeFlags, debugCallback, userData },
                                                 xr::DispatchLoaderDynamic{ instance });
}

}}  // namespace DebugUtilsEXT

