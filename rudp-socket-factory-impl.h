/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Author:
 */
#ifndef RUDP_SOCKET_FACTORY_IMPL_H
#define RUDP_SOCKET_FACTORY_IMPL_H

#include "ns3/rudp-socket-factory.h"
#include "ns3/ptr.h"

namespace ns3 {

class RudpL4Protocol;

/**
 * \ingroup internet
 * \defgroup rudp Rudp
 *
 * Put description here
 */

/**
 * \ingroup rudp
 * \brief Object to create RUDP socket instances 
 *
 * This class implements the API for creating RUDP sockets.
 * It is a socket factory (deriving from class SocketFactory).
 */
class RudpSocketFactoryImpl : public RudpSocketFactory
{
public:
  RudpSocketFactoryImpl ();
  virtual ~RudpSocketFactoryImpl ();

  /**
   * \brief Set the associated UDP L4 protocol.
   * \param udp the UDP L4 protocol
   */
  void SetRudp (Ptr<RudpL4Protocol> rudp);

  /**
   * \brief Implements a method to create a Rudp-based socket and return
   * a base class smart pointer to the socket.
   *
   * \return smart pointer to Socket
   */
  virtual Ptr<Socket> CreateSocket (void);

protected:
  virtual void DoDispose (void);
private:
  Ptr<RudpL4Protocol> m_rudp; //!< the associated RUDP L4 protocol
};

} // namespace ns3

#endif /* RUDP_SOCKET_FACTORY_IMPL_H */
