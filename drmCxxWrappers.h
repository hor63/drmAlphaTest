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

template <typename T> class DrmResourceException : public ExceptionBase
{
public:
  DrmResourceException(std::string const &descr)
      : ExceptionBase{[](std::string const &descr) -> std::string {
          std::string typeDescr{"Error occurred for type "};
          typeDescr.append(typeid(T).name()).append(": ").append(descr);
          return typeDescr;
        }(descr)}
  {
  }
}; // template class DrmResourceException

template <typename T, typename IdT> class DrmResource
{
public:
  using TPtr               = T *;
  using TResourceException = DrmResourceException<T>;

private:
  FileDescrShPtr fileDescr;
  IdT resourceID = 0;
  TPtr resource  = nullptr;

  /** \brief Obtain the pointer to the drm resource.
   *
   * This method requires template specialization.
   * C has no polymorphism. but exilicitly named different functions.
   */
  static TPtr getResource(FileDescrShPtr fileDescr, IdT id) noexcept;

  /** \brief Free the drm resource.
   *
   * This method requires template specialization.
   * C has no polymorphism. but explicitly named different functions.
   */
  static void freeResource(TPtr resourcePtr) noexcept;

public:
  DrmResource(FileDescrShPtr const &fileDescrShPtr, IdT resourceID)
      : fileDescr{fileDescrShPtr}, resourceID{resourceID},
        resource{getResource(fileDescrShPtr, resourceID)}
  {
    if (resource == nullptr) {
      std::string descr{"Cannot obtain the resource for ID "};
      descr.append(std::to_string(resourceID));
      throw TResourceException(descr);
    }
  }
  ~DrmResource()
  {
    if (resource != nullptr) {
      freeResource(resource);
    }
  }

  DrmResource(DrmResource const &)            = delete;
  DrmResource &operator=(DrmResource const &) = delete;

  DrmResource(DrmResource &&source) noexcept
  {
    if (resource != nullptr) {
      freeResource(resource);
    }
    resource          = source.resource;
    source.resource   = nullptr;
    fileDescr         = std::move(source.fileDescr);
    resourceID        = source.resourceID;
    source.resourceID = 0;
  }

  DrmResource &operator=(DrmResource &&source) noexcept
  {
    if (resource != nullptr) {
      freeResource(resource);
    }
    resource          = source.resource;
    source.resource   = nullptr;
    fileDescr         = std::move(source.fileDescr);
    resourceID        = source.resourceID;
    source.resourceID = 0;

    return *this;
  }

  TPtr const get() const noexcept
  {
    return resource;
  }

  TPtr const operator->() const
  {

    if (resource == nullptr) {
      std::string descr{"Trying de-referencing a nullptr of a "};
      descr.append(typeid(T).name()).append(" object.");
      throw TResourceException(descr);
    }

    return resource;
  }
};

using DrmModeResource = DrmResource<::drmModeRes, uint32_t>;
/** \brief Required specialization of template class method.
 *
 * \param fileDescr File descriptor object
 * \param id Ignored dummy. Set 0.
 */
template <>
inline ::drmModeResPtr
DrmResource<drmModeRes, uint32_t>::getResource(FileDescrShPtr fileDescr, uint32_t id) noexcept
{
  return ::drmModeGetResources(fileDescr->get());
};

/** \brief Required specialization of template class method.
 */
template <>
inline void
DrmResource<drmModeRes, uint32_t>::freeResource(::drmModeResPtr resourcePtr) noexcept
{
  ::drmModeFreeResources(resourcePtr);
}

using DrmModeCrtc = DrmResource<::drmModeCrtc, uint32_t>;
/** \brief Required specialization of template class method.
 *
 * \param fileDescr File descriptor object
 * \param id Ignored dummy. Set 0.
 */
template <>
inline ::drmModeCrtcPtr
DrmResource<::drmModeCrtc, uint32_t>::getResource(FileDescrShPtr fileDescr, uint32_t id) noexcept
{
  return ::drmModeGetCrtc(fileDescr->get(), id);
};

/** \brief Required specialization of template class method.
 */
template <>
inline void
DrmResource<::drmModeCrtc, uint32_t>::freeResource(::drmModeCrtcPtr resourcePtr) noexcept
{
  ::drmModeFreeCrtc(resourcePtr);
}

using DrmModeConnector = DrmResource<::drmModeConnector, uint32_t>;
/** \brief Required specialization of template class method.
 *
 * \param fileDescr File descriptor object
 * \param id Ignored dummy. Set 0.
 */
template <>
inline ::drmModeConnectorPtr
DrmResource<::drmModeConnector, uint32_t>::getResource(FileDescrShPtr fileDescr,
                                                       uint32_t id) noexcept
{
  return ::drmModeGetConnector(fileDescr->get(), id);
};

/** \brief Required specialization of template class method.
 */
template <>
inline void
DrmResource<::drmModeConnector, uint32_t>::freeResource(::drmModeConnectorPtr resourcePtr) noexcept
{
  ::drmModeFreeConnector(resourcePtr);
}

using DrmModeEncoder = DrmResource<::drmModeEncoder, uint32_t>;
/** \brief Required specialization of template class method.
 *
 * \param fileDescr File descriptor object
 * \param id Ignored dummy. Set 0.
 */
template <>
inline ::drmModeEncoderPtr
DrmResource<::drmModeEncoder, uint32_t>::getResource(FileDescrShPtr fileDescr, uint32_t id) noexcept
{
  return ::drmModeGetEncoder(fileDescr->get(), id);
};

/** \brief Required specialization of template class method.
 */
template <>
inline void
DrmResource<::drmModeEncoder, uint32_t>::freeResource(::drmModeEncoderPtr resourcePtr) noexcept
{
  ::drmModeFreeEncoder(resourcePtr);
}

}; // namespace Drm
#endif /* DRMCXXWRAPPERS_H_ */
