

#include "drmCxxWrappers.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int
main()
{

  std::cout << "Hello" << std::endl;

  auto numDevices = drmGetDevices2(0, NULL, 0);
  std::cout << "Number of drm devices = " << numDevices << std::endl;

  std::vector<drmDevicePtr> drmDevicePtrs;

  if (numDevices > 0) {
    drmDevicePtrs = std::vector<drmDevicePtr>{static_cast<size_t>(numDevices)};

    numDevices = drmGetDevices2(0U, drmDevicePtrs.data(), numDevices);
    std::cout << "Number of drm devices = " << numDevices << std::endl;
    std::cout << "drmDevicePtrs.size() = " << drmDevicePtrs.size() << std::endl;

    if (numDevices != drmDevicePtrs.size()) {
      std::cout << "Error: drmGetDevices2 returned " << numDevices << ". Exiting" << std::endl;
      exit(1);
    }
  } // if (numFiles > 0) {

  for (int i = 0; i < numDevices; ++i) {
    auto &device = *(drmDevicePtrs[i]);
    std::cout << "Device no. " << i << ": " << std::endl;

    switch (device.bustype) {
    case DRM_BUS_PCI: {
      auto const &pciDevice = *device.deviceinfo.pci;
      std::cout << "\tBus type is PCI. Vendor ID = " << pciDevice.vendor_id
                << ", Device ID = " << pciDevice.device_id << std::endl;
    } break;
    case DRM_BUS_USB: {
      auto const &usbDevice = *device.deviceinfo.usb;
      std::cout << "\tBus type is USB. Vendor ID = " << usbDevice.vendor
                << ", product ID = " << usbDevice.product << std::endl;
    } break;
    case DRM_BUS_PLATFORM: {
      auto const &platformDevice = *device.deviceinfo.platform;
      auto const &platformBus    = *device.businfo.platform;
      std::cout << "\tBus type is Platform. Compatible strings = ";
      for (int i = 0; platformDevice.compatible[i] != nullptr; ++i) {
        std::cout << "\n\t\t[" << i << "]: " << platformDevice.compatible[i];
      }
      std::cout << "\n\t Bus full name = " << platformBus.fullname << std::endl;
    } break;
    case DRM_BUS_HOST1X:
      std::cout << "\tBus type is Host1X" << std::endl;
      break;
    default:
      std::cout << "\tUnkown bustype = " << device.bustype << std::endl;
      break;
    }
    std::cout << "\tavailable_nodes = " << std::hex << device.available_nodes << std::dec
              << std::endl;

    std::cout << "\tDRM_NODE_PRIMARY = "
              << ((device.available_nodes & (1 << DRM_NODE_PRIMARY))
                    ? device.nodes[DRM_NODE_PRIMARY]
                    : "<NULL>")
              << std::endl;
    std::cout << "\tDRM_NODE_CONTROL = "
              << ((device.available_nodes & (1 << DRM_NODE_PRIMARY))
                    ? device.nodes[DRM_NODE_CONTROL]
                    : "<NULL>")
              << std::endl;
    std::cout << "\tDRM_NODE_RENDER = "
              << ((device.available_nodes & (1 << DRM_NODE_PRIMARY)) ? device.nodes[DRM_NODE_RENDER]
                                                                     : "<NULL>")
              << std::endl;

    if (!(device.available_nodes & (1 << DRM_NODE_PRIMARY))) {
      std::cout << "Huu? Does not have a primary node???"
                   "Continue with next device"
                << std::endl;
      continue;
    }
    try {
      Drm::FileDescrShPtr fileDescr =
        std::make_shared<Drm::FileDescr>(device.nodes[DRM_NODE_PRIMARY], O_RDWR);
      std::cout << "fd for " << fileDescr->getFileName() << " = " << fileDescr->get() << std::endl;

      auto drmRes = Drm::DrmModeResource(fileDescr, 0);
      std::cout << "drmModeGetResources successful with"
                << " count_fbs = " << drmRes->count_fbs << " count_crtcs = " << drmRes->count_crtcs
                << " count_connectors = " << drmRes->count_connectors
                << " count_encoders = " << drmRes->count_encoders
                << " min_width = " << drmRes->min_width << " max_width = " << drmRes->max_width
                << " min_height = " << drmRes->min_height << " max_height = " << drmRes->max_height
                << std::endl;

      for (int j = 0; j < drmRes->count_crtcs; ++j) {
        Drm::DrmModeCrtc crtc{fileDescr, drmRes->crtcs[j]};
        std::cout << "Crtc [" << j << "]:"
                  << " crtc_id = " << crtc->crtc_id << " buffer_id = " << crtc->buffer_id
                  << " x = " << crtc->x << " y = " << crtc->y << " width = " << crtc->width
                  << " height = " << crtc->height << " mode_valid = " << crtc->mode_valid
                  << " mode.name = " << crtc->mode.name << " gamma_size = " << crtc->gamma_size
                  << std::endl;
      }

      for (int j = 0; j < drmRes->count_connectors; ++j) {

        Drm::DrmModeConnector conn{fileDescr, drmRes->connectors[j]};
        std::cout << "Connector [" << j << "]:"
                  << " connector_id = " << conn->connector_id
                  << ", encoder_id = " << conn->encoder_id
                  << ", connector_type = " << conn->connector_type << "= "
                  << (drmModeGetConnectorTypeName(conn->connector_type)
                        ? drmModeGetConnectorTypeName(conn->connector_type)
                        : "Unknown")
                  << "\n\t\t, connector_type_id = " << conn->connector_type_id
                  << ", connection = " << conn->connection << ", mmWidth = " << conn->mmWidth
                  << ", mmHeight = " << conn->mmHeight << ", subpixel = " << conn->subpixel
                  << ", count_modes = " << conn->count_modes
                  << ", count_props = " << conn->count_props
                  << ", count_encoders = " << conn->count_encoders << std::endl;
        for (int k = 0; k < conn->count_modes; ++k) {
          auto &mode = conn->modes[k];
          std::cout << "\tmodes[" << k << "]:"
                    << " clock = " << mode.clock << ", hdisplay = " << mode.hdisplay
                    << ", vdisplay = " << mode.vdisplay << ", vrefresh = " << mode.vrefresh
                    << ", flags = " << mode.flags << ", type = " << mode.type
                    << ", name = " << mode.name << std::endl;
        }
        //						uint32_t *props; /**< List of property ids */
        //						uint64_t 	*prop_values; /**< List of property values
        //*/
        std::cout << "\tEncoders:" << std::endl;
        for (int k = 0; k < conn->count_encoders; ++k) {
          Drm::DrmModeEncoder encoder{fileDescr, conn->encoders[k]};
          std::cout << "\t\t" << "Encoder[" << k << "]:"
                    << " encoder_id = " << encoder->encoder_id
                    << " encoder_type = " << encoder->encoder_type
                    << " crtc_id = " << encoder->crtc_id
                    << " possible_crtcs = " << encoder->possible_crtcs << std::endl;
        }
        std::cout << std::endl;
      }

      std::cout << "FD " << fileDescr->get() << " of file " << fileDescr->getFileName()
                << " supports Kernel Mode Switching: "
                << (drmIsKMS(fileDescr->get()) ? "Yes" : "No") << std::endl;

    } catch (std::exception const &exp) {
      std::cerr << exp.what();
    }

  } // for (int i = 0 ; i < numFiles; ++i) {

  drmFreeDevices(drmDevicePtrs.data(), drmDevicePtrs.size());

  return 0;
}