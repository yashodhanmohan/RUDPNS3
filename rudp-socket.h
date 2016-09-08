/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 *               2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: 
 */

#ifndef RUDP_SOCKET_H
#define RUDP_SOCKET_H

#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"

namespace ns3 {

class Node;
class Packet;

/**
 * \ingroup socket
 *
 * \brief (abstract) base class of all UdpSockets
 *
 * This class exists solely for hosting UdpSocket attributes that can
 * be reused across different implementations, and for declaring
 * UDP-specific multicast API.
 */
class RudpSocket : public Socket
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
 
  RudpSocket (void);
  virtual ~RudpSocket (void);

private:
  // Indirect the attribute setting and getting through private virtual methods
  /**
   * \brief Set the receiving buffer size
   * \param size the buffer size
   */
  virtual void SetRcvBufSize (uint32_t size) = 0;
  /**
   * \brief Get the receiving buffer size
   * \returns the buffer size
   */
  virtual uint32_t GetRcvBufSize (void) const = 0;
  /**
   * \brief Set the MTU discover capability
   *
   * \param discover the MTU discover capability
   */
  virtual void SetMtuDiscover (bool discover) = 0;
  /**
   * \brief Get the MTU discover capability
   *
   * \returns the MTU discover capability
   */
  virtual bool GetMtuDiscover (void) const = 0;
};

} // namespace ns3

#endif /* UDP_SOCKET_H */


