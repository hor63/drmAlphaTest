
#include <cstring>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <iostream>
#include <string>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

int main () {
	int numFiles = 0;
	
	std::cout << "Hello" << std::endl;
	
	struct dirent64** direntList = nullptr;
	
	numFiles = scandir64("/dev/dri",&direntList,
		[](const struct dirent64 *dirent) {
			if (dirent->d_type == DT_CHR){
			return 1;
			} else {
				return 0;
			}
		},
		[](const struct dirent64 **, const struct dirent64 **) {
			return 1;
		}
	);

	std::cout << "scandir64 returned = " << numFiles << std::endl;
	
	for (int i = 0 ; i < numFiles; ++i) {
		
		std::cout << "directory entry #" << i << " = " 
			<< direntList[i]->d_name
			<< ", type = " << static_cast<int>(direntList[i]->d_type)
			<< std::endl;
		
		std::string absFileName = "/dev/dri/";
		absFileName.append (direntList[i]->d_name);
		
		auto fd = ::open(absFileName.c_str(),O_RDWR,0666);
		if (fd < 0) {
			auto err = errno;
			std::cerr << "Could not open " 
				<< absFileName.c_str() 
				<< ". Error is " << strerrorname_np(err)
				<< std::endl;
		} else {
			std::cout << "fd for " << absFileName 
				<< " = " << fd << std::endl;
				
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
				<< " max_height = " << drmRes->max_height
				<< std:: endl;

				for (int j = 0; j < drmRes->count_crtcs; ++j) {
					auto crtc = drmModeGetCrtc(fd,drmRes->crtcs[j]);
					if (crtc != nullptr) {
						std::cout << "Crtc ["<<j << "]:"
						<< " crtc_id = " << crtc->crtc_id
						<< " buffer_id = " << crtc->buffer_id
						<< " x = " << crtc->x
						<< " y = " << crtc->y
						<< " width = " << crtc->width
						<< " height = " << crtc->height
						<< " mode_valid = " << crtc->mode_valid
						<< " mode.name = " << crtc->mode.name
						<< " gamma_size = " << crtc->gamma_size
						<< std::endl;
						
						drmModeFreeCrtc(crtc);
					}
				}	

				for (int j = 0; j<drmRes->count_connectors ; ++j) {
					
					auto conn = drmModeGetConnector(fd,drmRes->connectors[j]);
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
						for (int k = 0 ; k<conn->count_modes ;++k) {
							auto &mode = conn->modes[k];
							std::cout << "\tmodes["<<k<<"]:"
							<< " clock = " << mode.clock
							<< ", hdisplay = " << mode.hdisplay
							<< ", vdisplay = " << mode.vdisplay
							<< ", vrefresh = " << mode.vrefresh
							<< ", flags = " << mode.flags
							<< ", type = " << mode.type
							<< ", name = " << mode.name
							<< std::endl;
						}
//						uint32_t *props; /**< List of property ids */
//						uint64_t 	*prop_values; /**< List of property values */
						std::cout << "\tEncoders = ";
						for (int k = 0 ; k<conn->count_encoders ;++k) {
							std::cout << conn->encoders[k] << ' ';
						}
						std::cout << std::endl;
						drmModeFreeConnector(conn);
					}
					
				}
				
				::drmModeFreeResources(drmRes);
			} //if (drmRes == nullptr) {
				
			::close(fd);
		} // if (fd < 0) {

			
	} // for (int i = 0 ; i < numFiles; ++i) {
	
	
	if (direntList != nullptr) {
		::free (direntList);
	}
	
}