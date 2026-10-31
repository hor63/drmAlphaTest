
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <ios>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <iostream>
#include <string>
#include <vector>



int
main()
{

  std::cout << "Hello" << std::endl;

  auto numFiles = drmGetDevices2(0, NULL, 0);
  std::cout << "Number of drm devices = " << numFiles << std::endl;

  std::vector<drmDevicePtr> drmDevicePtrs;

  if (numFiles > 0) {
    drmDevicePtrs = std::vector<drmDevicePtr>{static_cast<size_t>(numFiles)};
      
    numFiles = drmGetDevices2(0U, drmDevicePtrs.data(), numFiles);
    std::cout << "Number of drm devices = " << numFiles << std::endl;
    std::cout << "drmDevicePtrs.size() = " << drmDevicePtrs.size() << std::endl;
    
    if (numFiles != drmDevicePtrs.size()) {
      std::cout << "Error: drmGetDevices2 returned " << numFiles
        << ". Exiting" << std::endl;
    exit(1);
    }
  } // if (numFiles > 0) {
    
  for (int i = 0; i < numFiles; ++i) {
    std::cout << "Device no. " << i << ": " << std::endl;
    
    std::cout << "\tbustype = " << drmDevicePtrs[i]->bustype << std::endl;
    std::cout << "\tavailable_nodes = " 
      << std::hex << drmDevicePtrs[i]->available_nodes << std::dec
      << std::endl;
      
      std::cout << "\tDRM_NODE_PRIMARY = " 
        << ((drmDevicePtrs[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) ?
        drmDevicePtrs[i]->nodes[DRM_NODE_PRIMARY] : "<NULL>") << std::endl;
      std::cout << "\tDRM_NODE_CONTROL = " 
        << ((drmDevicePtrs[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) ?
        drmDevicePtrs[i]->nodes[DRM_NODE_CONTROL] : "<NULL>") << std::endl;
      std::cout << "\tDRM_NODE_RENDER = " 
        << ((drmDevicePtrs[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) ?
        drmDevicePtrs[i]->nodes[DRM_NODE_RENDER] : "<NULL>") << std::endl;

        if (!(drmDevicePtrs[i]->available_nodes & (1 << DRM_NODE_PRIMARY))) {
          // Huu? Does not have a primary node???
          continue;
        }
    std::string primaryNodeFileName = drmDevicePtrs[i]->nodes[DRM_NODE_PRIMARY];
    auto fd = ::open(primaryNodeFileName.c_str(), O_RDWR, 0666);
    if (fd < 0) {
      auto err = errno;
      std::cerr << "Could not open " << primaryNodeFileName << ". Error is "
                << strerrorname_np(err) << std::endl;
    } else {
      std::cout << "fd for " << primaryNodeFileName << " = " << fd << std::endl;

      auto drmRes = ::drmModeGetResources(fd);
      if (drmRes == nullptr) {
        std::cerr << " Could not obtain DRM mode resources" << std::endl;
      } else {
        std::cout << "drmModeGetResources successful with"
                  << " count_fbs = " << drmRes->count_fbs
                  << " count_crtcs = " << drmRes->count_crtcs
                  << " count_connectors = " << drmRes->count_connectors
                  << " count_encoders = " << drmRes->count_encoders
                  << " min_width = " << drmRes->min_width
                  << " max_width = " << drmRes->max_width
                  << " min_height = " << drmRes->min_height
                  << " max_height = " << drmRes->max_height << std::endl;

        for (int j = 0; j < drmRes->count_crtcs; ++j) {
          auto crtc = drmModeGetCrtc(fd, drmRes->crtcs[j]);
          if (crtc != nullptr) {
            std::cout << "Crtc [" << j << "]:"
                      << " crtc_id = " << crtc->crtc_id
                      << " buffer_id = " << crtc->buffer_id
                      << " x = " << crtc->x << " y = " << crtc->y
                      << " width = " << crtc->width
                      << " height = " << crtc->height
                      << " mode_valid = " << crtc->mode_valid
                      << " mode.name = " << crtc->mode.name
                      << " gamma_size = " << crtc->gamma_size << std::endl;

            drmModeFreeCrtc(crtc);
          }
        }

        for (int j = 0; j < drmRes->count_connectors; ++j) {

          auto conn = drmModeGetConnector(
            fd, drmRes->connectors[j]);
          if (conn != nullptr) {
            std::cout << "Connector [" << j << "]:"
                      << " connector_id = " << conn->connector_id
                      << ", encoder_id = " << conn->encoder_id
                      << ", connector_type = " << conn->connector_type
                      << ", connector_type_id = " << conn->connector_type_id
                      << ", connection = " << conn->connection
                      << ", mmWidth = " << conn->mmWidth
                      << ", mmHeight = " << conn->mmHeight
                      << ", subpixel = " << conn->subpixel
                      << ", count_modes = " << conn->count_modes
                      << ", count_props = " << conn->count_props
                      << ", count_encoders = " << conn->count_encoders
                      << std::endl;
            for (int k = 0; k < conn->count_modes; ++k) {
              auto &mode = conn->modes[k];
              std::cout << "\tmodes[" << k << "]:"
                        << " clock = " << mode.clock
                        << ", hdisplay = " << mode.hdisplay
                        << ", vdisplay = " << mode.vdisplay
                        << ", vrefresh = " << mode.vrefresh
                        << ", flags = " << mode.flags
                        << ", type = " << mode.type << ", name = " << mode.name
                        << std::endl;
            }
            //						uint32_t *props; /**< List of property ids */
            //						uint64_t 	*prop_values; /**< List of property values
            //*/
            std::cout << "\tEncoders = ";
            for (int k = 0; k < conn->count_encoders; ++k) {
              std::cout << conn->encoders[k] << ' ';
            }
            std::cout << std::endl;
            drmModeFreeConnector(conn);
          }
        }

        ::drmModeFreeResources(drmRes);
      } // if (drmRes == nullptr) {

      ::close(fd);
    } // if (fd < 0) {

  } // for (int i = 0 ; i < numFiles; ++i) {

  drmFreeDevices(drmDevicePtrs.data(), drmDevicePtrs.size());

  return 0;
}