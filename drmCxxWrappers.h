/*
 * drmCxxWrappers.h
 *
 *  Created on: Nov 1, 2026
 *      Author: hor
 *
 * C++ Wrappers around drm objects and structs
 * These make the resource management nightmare a lot easier.
 */

#ifndef DRMCXXWRAPPERS_H_
#define DRMCXXWRAPPERS_H_

#include <cerrno>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <ios>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Drm {
class ExceptionBase : public std::exception
{
private:
  std::string exceptionText;

public:
  ExceptionBase(std::string &&str) : exceptionText{std::move(str)} {}
  ExceptionBase(std::string const &str) : exceptionText{str} {}

  virtual char const *what() const noexcept override
  {
    return exceptionText.c_str();
  }
};

class ExceptionFileDescr : public ExceptionBase
{
public:
  ExceptionFileDescr(std::string &&descr, int errn)
      : ExceptionBase{std::move([](std::string &&descr, int errn) {
          std::string str{descr};
          str.append(". errno = ").append(strerrorname_np(errn));
          str.append(". errno means: ").append(strerror(errn));
          return str;
        }(std::move(descr), errn))}
  {
  }
}; // class ExceptionFileDescr

class FileDescr final
{
private:
  int fd               = -1;
  std::string fileName = "<None>";

public:
  FileDescr() noexcept {}
  FileDescr(const char *fileName, int flags, int fileAccessRights = 0666)
      : fd{::open(fileName, flags, fileAccessRights)}, fileName{fileName}
  {

    if (fd < 0) {
      auto errDescr = std::string("::open ").append(fileName);
      throw ExceptionFileDescr(std::move(errDescr), errno);
    }
  }
  ~FileDescr()
  {
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
  }

  /// \brief Copying FileDescr is prohibited. Use \ref FileDescrShPtr to share one file
  /// descriptor
  FileDescr(FileDescr const &) = delete;
  /// \brief Copying FileDescr is prohibited. Use \ref FileDescrShPtr to share one file
  /// descriptor
  FileDescr &operator=(FileDescr const &) = delete;

  /// \brief You can move a file descriptor.
  FileDescr(FileDescr &&source) noexcept
  {
    if (fd >= 0) {
      ::close(fd);
    }
    fd              = source.fd;
    fileName        = source.fileName;
    source.fd       = -1;
    source.fileName = "<None>";
  }
  /// \brief You can move a file descriptor.
  FileDescr &operator=(FileDescr &&source) noexcept
  {
    if (fd >= 0) {
      ::close(fd);
    }
    fd              = source.fd;
    fileName        = source.fileName;
    source.fd       = -1;
    source.fileName = "<None>";

    return *this;
  }

  /// \brief Return the file descriptor.
  int get() const noexcept
  {
    return fd;
  }
  std::string const &getFileName() const noexcept
  {
    return fileName;
  }
}; // class FileDescr

using FileDescrShPtr = std::shared_ptr<FileDescr>;

class ExceptionDrmModeResource final : public ExceptionBase
{
public:
  ExceptionDrmModeResource(FileDescrShPtr const &fileDescr)
      : ExceptionBase{std::move([](FileDescrShPtr const &fileDescr) {
          std::string str{"::drmModeGetResources returned NULL for file \""};
          str.append(fileDescr->getFileName()).append("\".");
          return str;
        }(fileDescr))}
  {
  }
  ExceptionDrmModeResource(std::string reason) : ExceptionBase{reason} {}
}; // class ExceptionDrmModeResource

/// \brief Wrapper around ::drmModeRes
class DrmModeResource final
{
  FileDescrShPtr fileDescr;
  ::drmModeResPtr modeResources = nullptr;

public:
  DrmModeResource(FileDescrShPtr const &fileDescrShPtr)
      : fileDescr{fileDescrShPtr}, modeResources{::drmModeGetResources(fileDescr->get())}
  {
    if (modeResources == nullptr) {
      throw DrmModeResource(fileDescr);
    }
  }
  ~DrmModeResource()
  {
    if (modeResources) {
      drmModeFreeResources(modeResources);
    }
  }

  DrmModeResource(DrmModeResource const &)            = delete;
  DrmModeResource &operator=(DrmModeResource const &) = delete;

  DrmModeResource(DrmModeResource &&source) noexcept
  {
    if (modeResources != nullptr) {
      drmModeFreeResources(modeResources);
    }
    modeResources        = source.modeResources;
    source.modeResources = nullptr;
    fileDescr            = std::move(source.fileDescr);
  }

  DrmModeResource &operator=(DrmModeResource &&source) noexcept
  {
    if (modeResources != nullptr) {
      drmModeFreeResources(modeResources);
    }
    modeResources        = source.modeResources;
    source.modeResources = nullptr;
    fileDescr            = std::move(source.fileDescr);

    return *this;
  }

  ::drmModeResPtr const get() const noexcept
  {
    return modeResources;
  }

  ::drmModeResPtr const operator->() const
  {

    if (modeResources == nullptr) {
      throw ExceptionDrmModeResource(
        "Trying de-referencing a nullptr of a Drm::DrmModeResource object");
    }

    return modeResources;
  }
}; // class DrmModeRes

class ExceptionDrmModeCrtc final : public ExceptionBase
{
public:
  ExceptionDrmModeCrtc(FileDescrShPtr const &fileDescr, uint32_t crtcID)
      : ExceptionBase{std::move([](FileDescrShPtr const &fileDescr, uint32_t crtcID) {
          std::string str{"::drmModeGetCrtc returned NULL for file \""};
          str.append(fileDescr->getFileName())
            .append("\" and crtc_id = ")
            .append(std::to_string(crtcID));
          return str;
        }(fileDescr, crtcID))}
  {
  }
  ExceptionDrmModeCrtc(std::string reason) : ExceptionBase{reason} {}
}; // class ExceptionDrmModeResource

class DrmModeCrtc final
{
private:
  FileDescrShPtr fileDescr;
  ::drmModeCrtcPtr modeCrtc = nullptr;

public:
  DrmModeCrtc(FileDescrShPtr const &fileDescrShPtr, uint32_t crtcId)
      : fileDescr{fileDescrShPtr}, modeCrtc{::drmModeGetCrtc(fileDescrShPtr->get(), crtcId)}
  {
    if (modeCrtc == nullptr) {
      throw ExceptionDrmModeCrtc(fileDescrShPtr, crtcId);
    }
  }
  ~DrmModeCrtc()
  {
    if (modeCrtc != nullptr) {
      ::drmModeFreeCrtc(modeCrtc);
    }
  }

  DrmModeCrtc(DrmModeCrtc const &)            = delete;
  DrmModeCrtc &operator=(DrmModeCrtc const &) = delete;

  DrmModeCrtc(DrmModeCrtc &&source) noexcept
  {
    if (modeCrtc != nullptr) {
      ::drmModeFreeCrtc(modeCrtc);
    }
    modeCrtc        = source.modeCrtc;
    source.modeCrtc = nullptr;
    fileDescr       = std::move(source.fileDescr);
  }
  DrmModeCrtc &operator=(DrmModeCrtc &&source) noexcept
  {
    if (modeCrtc != nullptr) {
      ::drmModeFreeCrtc(modeCrtc);
    }
    modeCrtc        = source.modeCrtc;
    source.modeCrtc = nullptr;
    fileDescr       = std::move(source.fileDescr);

    return *this;
  }

  ::drmModeCrtcPtr const get() const noexcept
  {
    return modeCrtc;
  }
  ::drmModeCrtcPtr const operator->() const
  {
    if (modeCrtc == nullptr) {
      throw ExceptionDrmModeCrtc("Trying de-referencing a nullptr of a Drm::DrmModeCrtc object");
    }
    return modeCrtc;
  }

}; // class DrmModeCrtc

class ExceptionDrmModeConnector final : public ExceptionBase
{
public:
  ExceptionDrmModeConnector(FileDescrShPtr const &fileDescr, uint32_t connectorID)
      : ExceptionBase{std::move([](FileDescrShPtr const &fileDescr, uint32_t connectorID) {
          std::string str{"::drmModeGetConnector returned NULL for file \""};
          str.append(fileDescr->getFileName())
            .append("\" and crtc_id = ")
            .append(std::to_string(connectorID));
          return str;
        }(fileDescr, connectorID))}
  {
  }
  ExceptionDrmModeConnector(std::string reason) : ExceptionBase{reason} {}
}; // class ExceptionDrmModeResource

class DrmModeConnector final
{
private:
  FileDescrShPtr fileDescr;
  ::drmModeConnectorPtr modeConnector = nullptr;

public:
  DrmModeConnector(FileDescrShPtr const &fileDescrShPtr, uint32_t connectorId)
      : fileDescr{fileDescrShPtr},
        modeConnector{::drmModeGetConnector(fileDescrShPtr->get(), connectorId)}
  {
    if (modeConnector == nullptr) {
      throw ExceptionDrmModeConnector(fileDescrShPtr, connectorId);
    }
  }
  ~DrmModeConnector()
  {
    if (modeConnector != nullptr) {
      ::drmModeFreeConnector(modeConnector);
    }
  }

  DrmModeConnector(DrmModeConnector const &)            = delete;
  DrmModeConnector &operator=(DrmModeConnector const &) = delete;

  DrmModeConnector(DrmModeConnector &&source) noexcept
  {
    if (modeConnector != nullptr) {
      ::drmModeFreeConnector(modeConnector);
    }
    modeConnector        = source.modeConnector;
    source.modeConnector = nullptr;
    fileDescr            = std::move(source.fileDescr);
  }
  DrmModeConnector &operator=(DrmModeConnector &&source) noexcept
  {
    if (modeConnector != nullptr) {
      ::drmModeFreeConnector(modeConnector);
    }
    modeConnector        = source.modeConnector;
    source.modeConnector = nullptr;
    fileDescr            = std::move(source.fileDescr);

    return *this;
  }

  ::drmModeConnectorPtr const get() const noexcept
  {
    return modeConnector;
  }
  ::drmModeConnectorPtr const operator->() const
  {
    if (modeConnector == nullptr) {
      throw ExceptionDrmModeConnector(
        "Trying de-referencing a nullptr of a Drm::DrmModeConnector object");
    }
    return modeConnector;
  }

}; // class DrmModeConnector

}; // namespace Drm
#endif /* DRMCXXWRAPPERS_H_ */
